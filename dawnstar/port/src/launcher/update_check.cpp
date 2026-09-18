#include "launcher/update_check.h"

#include <windows.h>

#include <winhttp.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace dawnstar {
namespace launcher {

namespace {

std::wstring Widen(const std::string& text) {
    if (text.empty()) return std::wstring();
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                                         nullptr, 0);
    std::wstring wide(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &wide[0], size);
    return wide;
}

// --- version comparison ------------------------------------------------

struct ParsedVersion {
    int components[3] = {0, 0, 0};
    std::string suffix;  // whatever followed a '-', empty for a final release
};

ParsedVersion ParseVersion(const std::string& text) {
    ParsedVersion parsed;
    size_t at = 0;
    if (at < text.size() && (text[at] == 'v' || text[at] == 'V')) ++at;

    for (int component = 0; component < 3 && at < text.size(); ++component) {
        int value = 0;
        bool anyDigits = false;
        while (at < text.size() && text[at] >= '0' && text[at] <= '9') {
            // Saturate rather than overflow on an absurd version string.
            if (value < 100000000) value = value * 10 + (text[at] - '0');
            ++at;
            anyDigits = true;
        }
        if (anyDigits) parsed.components[component] = value;
        if (at < text.size() && text[at] == '.') {
            ++at;
        } else {
            break;
        }
    }
    const size_t dash = text.find('-', at ? at - 1 : 0);
    if (dash != std::string::npos && dash + 1 < text.size()) parsed.suffix = text.substr(dash + 1);
    return parsed;
}

// --- a very small JSON string reader -----------------------------------

// Reads the string value of `"key":"..."` starting the search at `from`.
// Handles the escapes GitHub actually emits; anything it does not
// understand is copied through, which is right for a URL.
bool ReadJsonString(const std::string& json, const std::string& key, size_t from,
                    std::string& value, size_t* valueEnd = nullptr) {
    const std::string needle = "\"" + key + "\"";
    size_t at = json.find(needle, from);
    if (at == std::string::npos) return false;
    at += needle.size();
    while (at < json.size() && (json[at] == ' ' || json[at] == ':' || json[at] == '\t')) ++at;
    if (at >= json.size() || json[at] != '"') return false;
    ++at;

    value.clear();
    while (at < json.size() && json[at] != '"') {
        if (json[at] == '\\' && at + 1 < json.size()) {
            const char escaped = json[at + 1];
            switch (escaped) {
                case 'n': value.push_back('\n'); break;
                case 't': value.push_back('\t'); break;
                case 'r': value.push_back('\r'); break;
                case 'b': value.push_back('\b'); break;
                case 'f': value.push_back('\f'); break;
                case 'u': {
                    // Only the BMP, only as UTF-8 -- enough for a release
                    // name, and none of the fields we use should contain one.
                    if (at + 5 < json.size()) {
                        const std::string hex = json.substr(at + 2, 4);
                        const unsigned code =
                            static_cast<unsigned>(std::strtoul(hex.c_str(), nullptr, 16));
                        if (code < 0x80) {
                            value.push_back(static_cast<char>(code));
                        } else if (code < 0x800) {
                            value.push_back(static_cast<char>(0xC0 | (code >> 6)));
                            value.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                        } else {
                            value.push_back(static_cast<char>(0xE0 | (code >> 12)));
                            value.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                            value.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                        }
                        at += 6;
                        continue;
                    }
                    break;
                }
                default: value.push_back(escaped); break;  // covers \" \\ \/
            }
            at += 2;
            continue;
        }
        value.push_back(json[at]);
        ++at;
    }
    if (at >= json.size()) return false;  // unterminated
    if (valueEnd) *valueEnd = at;
    return true;
}

bool ReadJsonBool(const std::string& json, const std::string& key, size_t from, bool& value) {
    const std::string needle = "\"" + key + "\"";
    size_t at = json.find(needle, from);
    if (at == std::string::npos) return false;
    at += needle.size();
    while (at < json.size() && (json[at] == ' ' || json[at] == ':' || json[at] == '\t')) ++at;
    if (json.compare(at, 4, "true") == 0) {
        value = true;
        return true;
    }
    if (json.compare(at, 5, "false") == 0) {
        value = false;
        return true;
    }
    return false;
}

bool StartsWith(const std::string& text, const std::string& prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

bool EndsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string DescribeWinHttpError(DWORD code) {
    switch (code) {
        case ERROR_WINHTTP_CANNOT_CONNECT: return "could not connect";
        case ERROR_WINHTTP_TIMEOUT: return "timed out";
        case ERROR_WINHTTP_NAME_NOT_RESOLVED: return "could not resolve github.com";
        case ERROR_WINHTTP_SECURE_FAILURE: return "the secure connection failed";
        case ERROR_WINHTTP_CONNECTION_ERROR: return "the connection was lost";
        default: return "network error " + std::to_string(code);
    }
}

// One WinHTTP request, start to finish. `onBody` is handed each chunk as it
// arrives; returning false from it aborts. Shared by the release check and
// the download, which differ only in what they do with the bytes.
bool HttpGet(const std::wstring& url, const wchar_t* accept,
             bool (*onBody)(const char*, size_t, void*), void* context,
             unsigned long long* contentLength, std::string& error) {
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    wchar_t host[256] = {};
    wchar_t path[2048] = {};
    parts.lpszHostName = host;
    parts.dwHostNameLength = ARRAYSIZE(host);
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = ARRAYSIZE(path);
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts)) {
        error = "malformed URL";
        return false;
    }

    // A User-Agent is not optional: the GitHub API rejects requests without
    // one. WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY picks up a system proxy,
    // which is the difference between working and not on a corporate
    // network.
    HINTERNET session = WinHttpOpen(L"DawnstarRemastered-Launcher/1.0",
                                    WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        error = DescribeWinHttpError(GetLastError());
        return false;
    }
    // Generous, but not unbounded: a hung connection must not leave the
    // launcher's update thread alive forever.
    WinHttpSetTimeouts(session, 15000, 15000, 30000, 30000);

    HINTERNET connection = WinHttpConnect(session, parts.lpszHostName, parts.nPort, 0);
    if (!connection) {
        error = DescribeWinHttpError(GetLastError());
        WinHttpCloseHandle(session);
        return false;
    }

    const DWORD flags = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connection, L"GET", parts.lpszUrlPath, nullptr,
                                            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            flags);
    if (!request) {
        error = DescribeWinHttpError(GetLastError());
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    bool ok = false;
    do {
        if (accept && !WinHttpAddRequestHeaders(request, accept, static_cast<DWORD>(-1),
                                                 WINHTTP_ADDREQ_FLAG_ADD)) {
            error = "could not set request headers";
            break;
        }
        if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA,
                                0, 0, 0) ||
            !WinHttpReceiveResponse(request, nullptr)) {
            error = DescribeWinHttpError(GetLastError());
            break;
        }

        DWORD status = 0, statusSize = sizeof(status);
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
                            WINHTTP_NO_HEADER_INDEX);
        if (status == 404) {
            error = "no release found (HTTP 404)";
            break;
        }
        if (status == 403) {
            // Anonymous GitHub API calls are rate-limited per IP; saying so
            // beats "network error 403".
            error = "GitHub is rate-limiting this connection -- try again later";
            break;
        }
        if (status < 200 || status >= 300) {
            error = "server returned HTTP " + std::to_string(status);
            break;
        }

        if (contentLength) {
            *contentLength = 0;
            wchar_t lengthText[32] = {};
            DWORD lengthSize = sizeof(lengthText);
            if (WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_LENGTH,
                                    WINHTTP_HEADER_NAME_BY_INDEX, lengthText, &lengthSize,
                                    WINHTTP_NO_HEADER_INDEX)) {
                *contentLength = _wcstoui64(lengthText, nullptr, 10);
            }
        }

        std::vector<char> buffer(64 * 1024);
        bool aborted = false;
        for (;;) {
            DWORD read = 0;
            if (!WinHttpReadData(request, buffer.data(), static_cast<DWORD>(buffer.size()),
                                 &read)) {
                error = DescribeWinHttpError(GetLastError());
                aborted = true;
                break;
            }
            if (read == 0) break;
            if (onBody && !onBody(buffer.data(), read, context)) {
                error = "the transfer was stopped";
                aborted = true;
                break;
            }
        }
        ok = !aborted;
    } while (false);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return ok;
}

bool AppendToString(const char* data, size_t size, void* context) {
    static_cast<std::string*>(context)->append(data, size);
    // The release list payload is tens of KB even with several releases in
    // it; a megabyte means something is very wrong and we should not keep
    // growing a string over it.
    return static_cast<std::string*>(context)->size() < 4u * 1024 * 1024;
}

struct DownloadSink {
    HANDLE file = INVALID_HANDLE_VALUE;
    unsigned long long written = 0;
    // Points at HttpGet's own content-length, which it fills from the
    // headers *before* the first body chunk -- so progress can report a
    // real total from the first callback rather than only at the end.
    const unsigned long long* total = nullptr;
    void (*onProgress)(unsigned long long, unsigned long long, void*) = nullptr;
    void* progressContext = nullptr;
    bool failed = false;
};

bool WriteToFile(const char* data, size_t size, void* context) {
    DownloadSink* sink = static_cast<DownloadSink*>(context);
    DWORD written = 0;
    if (!WriteFile(sink->file, data, static_cast<DWORD>(size), &written, nullptr) ||
        written != size) {
        sink->failed = true;
        return false;
    }
    sink->written += size;
    if (sink->onProgress) {
        sink->onProgress(sink->written, sink->total ? *sink->total : 0, sink->progressContext);
    }
    return true;
}

}  // namespace

int CompareVersions(const std::string& left, const std::string& right) {
    const ParsedVersion a = ParseVersion(left);
    const ParsedVersion b = ParseVersion(right);
    for (int i = 0; i < 3; ++i) {
        if (a.components[i] != b.components[i]) return a.components[i] < b.components[i] ? -1 : 1;
    }
    if (a.suffix.empty() != b.suffix.empty()) {
        // A pre-release sorts before the release it leads to.
        return a.suffix.empty() ? 1 : -1;
    }
    if (a.suffix == b.suffix) return 0;
    return a.suffix < b.suffix ? -1 : 1;
}

bool ParseReleaseList(const std::string& json, const std::string& tagPrefix, ReleaseInfo& out) {
    out = ReleaseInfo{};
    if (json.empty()) return false;

    // Walk every release object in the array (there may be several games'
    // releases interleaved), taking the first whose tag starts with
    // `tagPrefix`. Each object's own field search is bounded by the next
    // "tag_name" occurrence (or the end of the payload), so a field read
    // for one release can never wander into the next one's.
    size_t search = 0;
    for (;;) {
        size_t tagStart = json.find("\"tag_name\"", search);
        if (tagStart == std::string::npos) break;

        std::string tag;
        size_t tagValueEnd = 0;
        if (!ReadJsonString(json, "tag_name", tagStart, tag, &tagValueEnd)) break;

        const size_t nextTagStart = json.find("\"tag_name\"", tagValueEnd);
        const size_t objectEnd = nextTagStart == std::string::npos ? json.size() : nextTagStart;
        search = tagValueEnd;

        if (!StartsWith(tag, tagPrefix)) continue;

        ReleaseInfo candidate;
        candidate.tag = tag;
        candidate.version = tag.substr(tagPrefix.size());
        ReadJsonBool(json, "prerelease", tagStart, candidate.prerelease);

        // The first .zip asset URL within this object's own bounds.
        size_t urlSearch = tagStart;
        std::string url;
        size_t urlEnd = 0;
        while (ReadJsonString(json, "browser_download_url", urlSearch, url, &urlEnd) &&
               urlEnd < objectEnd) {
            if (EndsWith(url, ".zip")) {
                candidate.downloadUrl = url;
                const size_t slash = url.find_last_of('/');
                candidate.assetName = slash == std::string::npos ? url : url.substr(slash + 1);
                break;
            }
            urlSearch = urlEnd;
        }

        if (candidate.valid()) {
            out = candidate;
            return true;
        }
        // This release matched the prefix but has no .zip -- keep looking;
        // an older release for the same game might.
    }
    return false;
}

bool FetchLatestRelease(const std::string& owner, const std::string& repo,
                        const std::string& tagPrefix, ReleaseInfo& out, std::string& error) {
    out = ReleaseInfo{};
    error.clear();
    // per_page=20: enough to find this game's most recent release even
    // when other games in the same repo have published more recently --
    // see this header's own note on why /releases/latest can't be used.
    const std::wstring url = L"https://api.github.com/repos/" + Widen(owner) + L"/" +
                             Widen(repo) + L"/releases?per_page=20";
    std::string body;
    if (!HttpGet(url, L"Accept: application/vnd.github+json\r\n", AppendToString, &body, nullptr,
                 error)) {
        return false;
    }
    if (!ParseReleaseList(body, tagPrefix, out)) {
        error = "no \"" + tagPrefix + "\" release with a downloadable build was found";
        return false;
    }
    return true;
}

bool DownloadFile(const std::string& url, const std::string& destPath,
                  void (*onProgress)(unsigned long long, unsigned long long, void*),
                  void* progressContext, std::string& error) {
    error.clear();
    unsigned long long contentLength = 0;

    DownloadSink sink;
    sink.onProgress = onProgress;
    sink.progressContext = progressContext;
    sink.total = &contentLength;
    sink.file = CreateFileW(Widen(destPath).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
    if (sink.file == INVALID_HANDLE_VALUE) {
        error = "could not create " + destPath;
        return false;
    }

    const bool ok = HttpGet(Widen(url), nullptr, WriteToFile, &sink, &contentLength, error);
    CloseHandle(sink.file);

    if (!ok || sink.failed) {
        if (sink.failed && error.empty()) error = "could not write " + destPath;
        // No half-downloaded file left behind to be mistaken for a good one.
        DeleteFileW(Widen(destPath).c_str());
        return false;
    }
    return true;
}

}  // namespace launcher
}  // namespace dawnstar
