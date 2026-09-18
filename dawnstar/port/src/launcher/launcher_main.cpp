// The launcher -- what someone who downloaded a zip actually opens.
//
// Ported from shadowkey-decomp's port/src/launcher/launcher_main.cpp,
// same architecture throughout (owner-drawn buttons, an Impl-in-
// GWLP_USERDATA idiom, an artwork-sized window, a detached worker thread
// posting WM_APP_* messages back for the update check/install, IFileOpenDialog
// pickers, hidden-while-playing + MsgWaitForMultipleObjects). Two real
// differences from the source this was ported from: there is no font row
// (Dawnstar's renderer draws its own embedded bitmap font -- see
// graphics/bitmap_font.h/.cpp -- and reads no external file, unlike
// Shadowkey's Nokia ROM font requirement), and "Choose file..." asks for
// a single .jar rather than a folder to search (see launcher/install.h).
//
// It draws its own text and its own buttons rather than using a dialog
// template: the artwork is a dark, mostly-black wordmark, and the default
// Win32 controls on top of it look like a bug. Only the buttons are real
// windows (they need focus, keyboard activation and the accessibility
// that comes with them); every label is drawn in WM_PAINT.

#include <windows.h>

#include <commctrl.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "launcher/banner.h"
#include "launcher/install.h"
#include "launcher/launcher_config.h"
#include "launcher/resource.h"
#include "launcher/update_check.h"
#include "launcher/updater.h"
#include "launcher/version.h"
#include "platform/win32/exe_dir.h"

namespace {

namespace fs = std::filesystem;
using dawnstar::launcher::LauncherConfig;

// ---------------------------------------------------------------------
// Palette. The banner is a mostly-black background behind gold lettering
// ("The Elder Scrolls Travels: Dawnstar"), so a dark panel that reads as a
// continuation of that background -- rather than an approximation of some
// other art style -- is the natural fit; SeamColorFor below samples the
// artwork's own last visible row so the panel actually blends into it.
// ---------------------------------------------------------------------
constexpr COLORREF kBackground = RGB(10, 9, 14);
constexpr COLORREF kPanel = RGB(16, 14, 22);
constexpr COLORREF kPanelHigh = RGB(30, 26, 42);
constexpr COLORREF kRule = RGB(120, 98, 46);   // the art's gold underline
constexpr COLORREF kText = RGB(230, 224, 210);
constexpr COLORREF kCaption = RGB(196, 164, 92);
constexpr COLORREF kMuted = RGB(140, 128, 108);
constexpr COLORREF kGold = RGB(224, 178, 88);
constexpr COLORREF kBad = RGB(224, 120, 120);

// Layout in logical (96-dpi) pixels; every use goes through Impl::S().
//
// The header's height is derived from the artwork's own proportions (like
// shadowkey-decomp's), not hardcoded, so a differently-proportioned banner
// re-lays the window instead of slicing the wordmark off.
constexpr int kClientWidth = 760;
constexpr int kMinClientWidth = 520;
constexpr int kPanelHeight = 250;
constexpr int kFallbackHeaderHeight = 200;  // only if the artwork fails to decode
constexpr int kBannerVisiblePermille = 1000;  // this banner has no lower "cropped" region to trim

constexpr int kMargin = 36;
constexpr int kButtonWidth = 160;
constexpr int kButtonHeight = 30;
constexpr int kButtonRight = kMargin + kButtonWidth;  // from the window's right edge

// All offsets below are measured from the top of the panel, not the top of
// the window, since the header above them moves with the artwork.
constexpr int kRow1Caption = 20;
constexpr int kRow1Value = 40;
constexpr int kRow1Button = 32;
constexpr int kRow2Caption = 82;
constexpr int kScaleY = 102;
constexpr int kScaleWidth = 44;
constexpr int kScaleHeight = 28;
constexpr int kPlayY = 96;
constexpr int kPlayHeight = 44;
constexpr int kStatusY = 152;
constexpr int kVersionY = 196;
constexpr int kUpdateY = 218;

enum ControlId : int {
    IDC_CHOOSE_DATA = 1001,
    IDC_PLAY = 1002,
    IDC_UPDATE = 1003,
    IDC_SCALE_FIRST = 1010,  // +0 => 2x, +1 => 3x, +2 => 4x
};

constexpr int kScaleChoices[] = {2, 3, 4};

// What the update worker thread sends back. The thread never touches the
// launcher's state directly -- it posts a heap-allocated result the window
// procedure takes ownership of. That is what makes the worker safe to
// detach: if the window has gone, PostMessage fails and the worker frees
// its own result, and nothing is left pointing at a dead stack frame.
constexpr UINT WM_APP_UPDATE_CHECKED = WM_APP + 1;
constexpr UINT WM_APP_UPDATE_PROGRESS = WM_APP + 2;
constexpr UINT WM_APP_UPDATE_DONE = WM_APP + 3;

struct UpdateCheckResult {
    bool ok = false;
    dawnstar::launcher::ReleaseInfo release;
    std::string error;
};

struct UpdateProgressMessage {
    dawnstar::launcher::UpdateProgress progress;
};

struct UpdateDoneMessage {
    bool ok = false;
    std::string error;
    std::string tag;
};

enum class UpdateState {
    Disabled,   // a -dev build: never offers to replace a build tree
    Checking,
    UpToDate,
    Available,
    Installing,
    Failed,
};

std::wstring Widen(const std::string& text) {
    if (text.empty()) return std::wstring();
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                                         nullptr, 0);
    std::wstring wide(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wide.data(), size);
    return wide;
}

std::string Narrow(const std::wstring& text) {
    if (text.empty()) return std::string();
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                                         nullptr, 0, nullptr, nullptr);
    std::string narrow(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), narrow.data(),
                        size, nullptr, nullptr);
    return narrow;
}

// Long absolute paths are the normal case here (people keep downloads deep
// in Downloads), and the value row is only so wide. Middle-elision keeps
// both the drive and the leaf visible, which is what tells someone at a
// glance whether they picked the right thing.
std::wstring ElidePath(const std::wstring& path, size_t limit) {
    if (path.size() <= limit || limit < 12) return path;
    const size_t tail = limit * 2 / 3;
    const size_t head = limit - tail - 3;
    return path.substr(0, head) + L"..." + path.substr(path.size() - tail);
}

// ---------------------------------------------------------------------
// Owner-drawn buttons.
// ---------------------------------------------------------------------
struct ButtonStyle {
    bool primary = false;
    bool selected = false;
};

// Hover is not something a plain BS_OWNERDRAW button reports, so each
// button is subclassed for the two messages that track it.
LRESULT CALLBACK ButtonSubclass(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
                                UINT_PTR idSubclass, DWORD_PTR refData) {
    (void)idSubclass;
    bool* hovered = reinterpret_cast<bool*>(refData);
    switch (message) {
        case WM_MOUSEMOVE:
            if (hovered && !*hovered) {
                *hovered = true;
                TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hwnd, 0};
                TrackMouseEvent(&track);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            break;
        case WM_MOUSELEAVE:
            if (hovered && *hovered) {
                *hovered = false;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            break;
        default:
            break;
    }
    return DefSubclassProc(hwnd, message, wParam, lParam);
}

struct Button {
    HWND hwnd = nullptr;
    bool hovered = false;
    ButtonStyle style;
};

// ---------------------------------------------------------------------

struct Impl {
    HWND hwnd = nullptr;
    int dpi = 96;

    dawnstar::launcher::Banner banner;
    std::string installRoot;   // where Dawnstar.exe lives
    std::string configPath;
    LauncherConfig config;

    // Resolved absolute path and whether it currently checks out. Recomputed
    // by Refresh() rather than cached at pick time, so an install whose
    // data folder was deleted behind our back reports that on the next
    // launch instead of failing at Play.
    std::string dataPath;
    bool dataOk = false;

    std::wstring status;
    COLORREF statusColor = kMuted;
    bool busy = false;

    Button chooseData;
    Button play;
    Button update;
    Button scale[3];

    UpdateState updateState = UpdateState::Checking;
    dawnstar::launcher::ReleaseInfo availableRelease;
    std::wstring updateNote;
    // Set while an install is in flight. The window refuses to close and
    // the game refuses to start during it -- replacing dawnstar_port.exe
    // out from under a running game would be a genuinely bad time.
    bool installing = false;

    HFONT uiFont = nullptr;
    HFONT captionFont = nullptr;
    HFONT valueFont = nullptr;
    HFONT playFont = nullptr;

    // Both in device pixels, settled once at startup by SizeToArtwork().
    int clientWidth = 0;
    int panelTop = 0;
    // The artwork's own colour where the panel cuts it off, darkened. Falls
    // back to kPanel.
    COLORREF seamColor = kPanel;

    int S(int logical) const { return MulDiv(logical, dpi, 96); }
    // Panel-relative y, in device pixels.
    int P(int logical) const { return panelTop + S(logical); }
    int ButtonX() const { return clientWidth - S(kButtonRight); }
};

// How tall the header is when the artwork is drawn across `width`: the
// image's own aspect, cropped to the visible fraction.
int HeaderHeightFor(const dawnstar::launcher::Banner& banner, int width, int fallback) {
    if (!banner.valid()) return fallback;
    const int drawnHeight = MulDiv(banner.height, width, banner.width);
    return MulDiv(drawnHeight, kBannerVisiblePermille, 1000);
}

// The inverse: the width at which the header comes out `header` tall. Used
// only when the preferred window would not fit on the screen.
int WidthForHeaderHeight(const dawnstar::launcher::Banner& banner, int header) {
    if (!banner.valid() || header <= 0) return 0;
    const int drawnHeight = MulDiv(header, 1000, kBannerVisiblePermille);
    return MulDiv(banner.width, drawnHeight, banner.height);
}

COLORREF Blend(COLORREF from, COLORREF to, int amount /* 0..255 */) {
    const int inverse = 255 - amount;
    return RGB((GetRValue(from) * inverse + GetRValue(to) * amount) / 255,
               (GetGValue(from) * inverse + GetGValue(to) * amount) / 255,
               (GetBValue(from) * inverse + GetBValue(to) * amount) / 255);
}

// The average colour of the artwork's last visible row, darkened so the
// panel still reads as a panel.
COLORREF SeamColorFor(const dawnstar::launcher::Banner& banner) {
    if (!banner.valid()) return kPanel;
    int row = MulDiv(banner.height, kBannerVisiblePermille, 1000);
    if (row >= banner.height) row = banner.height - 1;
    if (row < 0) return kPanel;

    unsigned long long red = 0, green = 0, blue = 0;
    const uint32_t* pixels = banner.pixels.data() + static_cast<size_t>(row) * banner.width;
    for (int x = 0; x < banner.width; ++x) {
        const unsigned char* bgra = reinterpret_cast<const unsigned char*>(&pixels[x]);
        blue += bgra[0];
        green += bgra[1];
        red += bgra[2];
    }
    const int count = banner.width;
    const COLORREF average = RGB(static_cast<int>(red / count), static_cast<int>(green / count),
                                 static_cast<int>(blue / count));
    return Blend(average, kPanel, 150);
}

Impl* Self(HWND hwnd) {
    return reinterpret_cast<Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

// GetDpiForWindow is Win10 1607+; the manifest claims back to 7, so it is
// resolved at runtime with the classic device-caps reading as the fallback.
int QueryDpi(HWND hwnd) {
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
    if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        auto getDpi =
            reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(user32, "GetDpiForWindow"));
        if (getDpi) {
            const UINT dpi = getDpi(hwnd);
            if (dpi >= 72) return static_cast<int>(dpi);
        }
    }
    HDC dc = GetDC(hwnd);
    const int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSX) : 96;
    if (dc) ReleaseDC(hwnd, dc);
    return dpi >= 72 ? dpi : 96;
}

HFONT MakeFont(int dpi, int pointSize, int weight) {
    return CreateFontW(-MulDiv(pointSize, dpi, 72), 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                       VARIABLE_PITCH | FF_SWISS, L"Segoe UI");
}

void SetStatus(Impl& impl, const std::wstring& text, COLORREF color) {
    impl.status = text;
    impl.statusColor = color;
    InvalidateRect(impl.hwnd, nullptr, FALSE);
}

// ---------------------------------------------------------------------
// State
// ---------------------------------------------------------------------

void Refresh(Impl& impl) {
    impl.dataPath = dawnstar::launcher::ResolveAgainst(impl.installRoot, impl.config.gameData);
    impl.dataOk = dawnstar::launcher::IsGameDataRoot(impl.dataPath);

    const bool busy = impl.busy || impl.installing;
    EnableWindow(impl.play.hwnd, impl.dataOk && !busy);
    EnableWindow(impl.chooseData.hwnd, !busy);
    for (int i = 0; i < 3; ++i) {
        impl.scale[i].style.selected = kScaleChoices[i] == impl.config.scale;
        EnableWindow(impl.scale[i].hwnd, !busy);
    }

    // The update button exists only when there is actually an update: a
    // permanently visible "check for updates" that usually says "no" is
    // noise, and this checks on its own at startup anyway.
    const bool offerUpdate = impl.updateState == UpdateState::Available && !busy;
    ShowWindow(impl.update.hwnd, offerUpdate ? SW_SHOW : SW_HIDE);
    EnableWindow(impl.update.hwnd, offerUpdate);

    InvalidateRect(impl.hwnd, nullptr, FALSE);
}

void Save(Impl& impl) { dawnstar::launcher::SaveLauncherConfig(impl.configPath, impl.config); }

// ---------------------------------------------------------------------
// Updates
// ---------------------------------------------------------------------

std::wstring FormatSize(unsigned long long bytes) {
    if (bytes >= 1024ull * 1024) {
        wchar_t text[32];
        swprintf(text, 32, L"%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
        return text;
    }
    return std::to_wstring(bytes / 1024) + L" KB";
}

// Both workers below are detached and own everything they touch. Nothing
// here reads or writes `Impl` -- see WM_APP_UPDATE_* for why.
void StartUpdateCheck(HWND hwnd) {
    std::thread([hwnd]() {
        auto* result = new UpdateCheckResult();
        result->ok = dawnstar::launcher::FetchLatestRelease(
            DAWNSTAR_UPDATE_OWNER, DAWNSTAR_UPDATE_REPO, DAWNSTAR_UPDATE_TAG_PREFIX,
            result->release, result->error);
        if (!PostMessageW(hwnd, WM_APP_UPDATE_CHECKED, 0,
                          reinterpret_cast<LPARAM>(result))) {
            delete result;  // the window has gone; nobody will take it
        }
    }).detach();
}

void ForwardProgress(const dawnstar::launcher::UpdateProgress& progress, void* context) {
    HWND hwnd = static_cast<HWND>(context);
    auto* message = new UpdateProgressMessage{progress};
    if (!PostMessageW(hwnd, WM_APP_UPDATE_PROGRESS, 0, reinterpret_cast<LPARAM>(message))) {
        delete message;
    }
}

void StartInstall(HWND hwnd, const std::string& installRoot,
                  const dawnstar::launcher::ReleaseInfo& release) {
    std::thread([hwnd, installRoot, release]() {
        auto* done = new UpdateDoneMessage();
        done->tag = release.tag;
        done->ok = dawnstar::launcher::InstallUpdate(installRoot, release, ForwardProgress, hwnd,
                                                      done->error);
        if (!PostMessageW(hwnd, WM_APP_UPDATE_DONE, 0, reinterpret_cast<LPARAM>(done))) {
            delete done;
        }
    }).detach();
}

// ---------------------------------------------------------------------
// Pickers
// ---------------------------------------------------------------------

// IFileOpenDialog rather than the classic common dialog: this launcher
// already needs COM for it, and it has a modern address bar (pasting a
// path -- how most people navigate to a download -- works in it).
std::wstring PickPath(HWND owner, bool folders, const wchar_t* title,
                      const wchar_t* filterName, const wchar_t* filterSpec) {
    IFileOpenDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&dialog)))) {
        return std::wstring();
    }
    DWORD options = 0;
    if (SUCCEEDED(dialog->GetOptions(&options))) {
        dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR |
                           (folders ? FOS_PICKFOLDERS : 0));
    }
    dialog->SetTitle(title);
    if (!folders && filterName && filterSpec) {
        const COMDLG_FILTERSPEC filter[] = {{filterName, filterSpec}, {L"All files", L"*.*"}};
        dialog->SetFileTypes(2, filter);
    }

    std::wstring chosen;
    if (SUCCEEDED(dialog->Show(owner))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item)) && item) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path) {
                chosen = path;
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }
    dialog->Release();
    return chosen;
}

// Repaints and yields once, so "Unpacking the game files..." is actually on
// screen before the unzip starts rather than after it finishes.
void PumpOnce(Impl& impl) {
    UpdateWindow(impl.hwnd);
    MSG message;
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

void ChooseGameData(Impl& impl) {
    const std::wstring picked =
        PickPath(impl.hwnd, false, L"Select your Dawnstar .jar", L"Dawnstar game (*.jar)",
                 L"*.jar");
    if (picked.empty()) return;

    const std::string source = Narrow(picked);
    const std::string destination = (fs::path(impl.installRoot) / "data").string();
    impl.busy = true;
    Refresh(impl);
    SetStatus(impl, L"Unpacking the game files…", kCaption);
    PumpOnce(impl);

    std::string error;
    const bool installed = dawnstar::launcher::InstallGameJar(source, destination, error);
    impl.busy = false;
    if (!installed) {
        Refresh(impl);
        SetStatus(impl, Widen(error), kBad);
        return;
    }

    impl.config.gameData = dawnstar::launcher::RelativeToIfInside(impl.installRoot, destination);
    Save(impl);
    Refresh(impl);
    SetStatus(impl, L"Game files installed. Ready to play.", kGold);
}

// ---------------------------------------------------------------------
// Play
// ---------------------------------------------------------------------

// `bin/` is the shipped layout; the bare name is where a developer's own
// build puts it, right beside the launcher in port/build/. Supporting both
// means this can be tested straight out of a normal build.
std::string FindGameExecutable(const std::string& installRoot) {
    const fs::path candidates[] = {
        fs::path(installRoot) / "bin" / "dawnstar_port.exe",
        fs::path(installRoot) / "dawnstar_port.exe",
    };
    std::error_code error;
    for (const fs::path& candidate : candidates) {
        if (fs::is_regular_file(candidate, error) && !error) return candidate.string();
    }
    return std::string();
}

void Play(Impl& impl) {
    const std::string exe = FindGameExecutable(impl.installRoot);
    if (exe.empty()) {
        SetStatus(impl, L"dawnstar_port.exe is missing from this folder — the install "
                        L"looks incomplete. Re-extract the download.",
                  kBad);
        return;
    }

    const std::string userDir = (fs::path(impl.installRoot) / "user").string();
    std::error_code error;
    fs::create_directories(fs::path(userDir), error);

    // The game reads both of these; see main.cpp. DAWNSTAR_USER_DIR keeps
    // saves and the log out of bin/ and in one folder a player can find,
    // and DAWNSTAR_SCALE replaces what used to be a hardcoded 2x window.
    SetEnvironmentVariableW(L"DAWNSTAR_USER_DIR", Widen(userDir).c_str());
    SetEnvironmentVariableW(L"DAWNSTAR_SCALE", std::to_wstring(impl.config.scale).c_str());

    // argv[1] is the asset root -- quoted because it routinely contains
    // spaces.
    std::wstring command = L"\"" + Widen(exe) + L"\" \"" + Widen(impl.dataPath) + L"\"";

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');

    // CREATE_NO_WINDOW because the game is a WINAPI-subsystem binary with
    // its own log file (see main.cpp's OpenLogFile) -- no console to flash
    // up alongside it.
    const std::wstring workingDirectory = Widen(impl.installRoot);
    if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                        nullptr, workingDirectory.c_str(), &startup, &process)) {
        SetStatus(impl, L"Couldn't start the game. See user\\dawnstar_port.log.", kBad);
        return;
    }
    CloseHandle(process.hThread);

    // Out of the way while the game runs, back when it exits -- and the
    // wait pumps messages so the hidden window still answers the system
    // rather than going "not responding".
    ShowWindow(impl.hwnd, SW_HIDE);
    for (;;) {
        const DWORD result =
            MsgWaitForMultipleObjects(1, &process.hProcess, FALSE, INFINITE, QS_ALLINPUT);
        if (result == WAIT_OBJECT_0) break;
        MSG message;
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    CloseHandle(process.hProcess);

    ShowWindow(impl.hwnd, SW_SHOW);
    SetForegroundWindow(impl.hwnd);
    Refresh(impl);
    SetStatus(impl, L"Welcome back.", kMuted);
}

// ---------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------

void FillRectColor(HDC dc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

void DrawTextLine(HDC dc, HFONT font, COLORREF color, int x, int y, int width, int height,
                  const std::wstring& text, UINT format = DT_LEFT | DT_SINGLELINE | DT_VCENTER) {
    HGDIOBJ previous = SelectObject(dc, font);
    SetTextColor(dc, color);
    RECT rect{x, y, x + width, y + height};
    DrawTextW(dc, text.c_str(), -1, &rect, format | DT_NOPREFIX);
    SelectObject(dc, previous);
}

void PaintButton(Impl& impl, const Button& button, DRAWITEMSTRUCT* item) {
    const bool disabled = (item->itemState & ODS_DISABLED) != 0;
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;
    const bool focused = (item->itemState & ODS_FOCUS) != 0;

    COLORREF face = kPanelHigh;
    COLORREF border = RGB(70, 58, 38);
    COLORREF label = kText;
    if (button.style.primary) {
        face = pressed ? RGB(168, 128, 56) : (button.hovered ? RGB(240, 190, 100) : kGold);
        border = RGB(250, 210, 140);
        label = RGB(28, 20, 8);
    } else if (button.style.selected) {
        face = pressed ? RGB(78, 60, 32) : RGB(64, 48, 24);
        border = kGold;
        label = kGold;
    } else if (pressed) {
        face = RGB(48, 40, 28);
    } else if (button.hovered) {
        face = RGB(48, 40, 28);
        border = kRule;
    }
    if (disabled) {
        face = RGB(30, 26, 20);
        border = RGB(48, 40, 30);
        label = RGB(100, 92, 78);
    }

    HBRUSH faceBrush = CreateSolidBrush(face);
    HPEN borderPen = CreatePen(PS_SOLID, impl.S(1), border);
    HGDIOBJ oldBrush = SelectObject(item->hDC, faceBrush);
    HGDIOBJ oldPen = SelectObject(item->hDC, borderPen);
    const int radius = impl.S(6);
    RoundRect(item->hDC, item->rcItem.left, item->rcItem.top, item->rcItem.right,
              item->rcItem.bottom, radius, radius);
    SelectObject(item->hDC, oldBrush);
    SelectObject(item->hDC, oldPen);
    DeleteObject(faceBrush);
    DeleteObject(borderPen);

    wchar_t caption[64] = {};
    GetWindowTextW(item->hwndItem, caption, 63);
    HFONT font = button.style.primary ? impl.playFont : impl.uiFont;
    HGDIOBJ oldFont = SelectObject(item->hDC, font);
    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, label);
    RECT textRect = item->rcItem;
    DrawTextW(item->hDC, caption, -1, &textRect,
              DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    SelectObject(item->hDC, oldFont);

    if (focused && !disabled) {
        RECT focusRect = item->rcItem;
        InflateRect(&focusRect, -impl.S(4), -impl.S(4));
        DrawFocusRect(item->hDC, &focusRect);
    }
}

void PaintWindow(Impl& impl, HDC target) {
    RECT client{};
    GetClientRect(impl.hwnd, &client);
    const int width = client.right;
    const int height = client.bottom;

    // Composed off-screen and blitted once: the header is a large stretched
    // bitmap, and painting it straight to the window flickers visibly every
    // time a button repaints.
    HDC dc = CreateCompatibleDC(target);
    HBITMAP surface = CreateCompatibleBitmap(target, width, height);
    HGDIOBJ oldBitmap = SelectObject(dc, surface);

    const int headerHeight = impl.panelTop;

    RECT headerRect{0, 0, width, headerHeight};
    FillRectColor(dc, headerRect, kBackground);
    if (impl.banner.valid()) {
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth = impl.banner.width;
        info.bmiHeader.biHeight = -impl.banner.height;  // top-down
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;

        const int drawnHeight = MulDiv(impl.banner.height, width, impl.banner.width);
        SetStretchBltMode(dc, HALFTONE);
        SetBrushOrgEx(dc, 0, 0, nullptr);
        IntersectClipRect(dc, 0, 0, width, headerHeight);
        StretchDIBits(dc, 0, 0, width, drawnHeight, 0, 0, impl.banner.width, impl.banner.height,
                      impl.banner.pixels.data(), &info, DIB_RGB_COLORS, SRCCOPY);
        SelectClipRgn(dc, nullptr);
    }

    RECT panelRect{0, headerHeight, width, height};
    FillRectColor(dc, panelRect, kPanel);

    // Fade the top of the panel out of the artwork's own colour instead of
    // cutting to full contrast.
    const int fade = impl.S(48);
    for (int i = 0; i < fade && headerHeight + i < height; ++i) {
        const COLORREF row = Blend(impl.seamColor, kPanel, i * 255 / fade);
        RECT rowRect{0, headerHeight + i, width, headerHeight + i + 1};
        FillRectColor(dc, rowRect, row);
    }

    RECT ruleRect{0, headerHeight, width, headerHeight + impl.S(1)};
    FillRectColor(dc, ruleRect, kRule);

    SetBkMode(dc, TRANSPARENT);
    const int margin = impl.S(kMargin);
    const int valueWidth = impl.ButtonX() - margin - impl.S(20);
    const int valueChars = (std::max)(24, valueWidth / (std::max)(1, impl.S(7)));

    // Row 1 -- the game files.
    DrawTextLine(dc, impl.captionFont, kCaption, margin, impl.P(kRow1Caption), impl.S(400),
                 impl.S(18), L"GAME FILE");
    if (impl.dataOk) {
        DrawTextLine(dc, impl.valueFont, kText, margin, impl.P(kRow1Value), valueWidth,
                     impl.S(22), ElidePath(Widen(impl.dataPath), valueChars));
    } else {
        DrawTextLine(dc, impl.valueFont, kBad, margin, impl.P(kRow1Value), valueWidth, impl.S(22),
                     L"Not set — choose your Dawnstar .jar to begin.");
    }

    // Row 2 -- the window size.
    DrawTextLine(dc, impl.captionFont, kCaption, margin, impl.P(kRow2Caption), impl.S(400),
                 impl.S(18), L"WINDOW SIZE");

    // The status line, and the version in the corner.
    if (!impl.status.empty()) {
        DrawTextLine(dc, impl.uiFont, impl.statusColor, margin, impl.P(kStatusY),
                     width - 2 * margin, impl.S(38), impl.status,
                     DT_LEFT | DT_WORDBREAK | DT_EDITCONTROL);
    }
    DrawTextLine(dc, impl.captionFont, RGB(96, 84, 60), margin, impl.P(kVersionY),
                 width - 2 * margin, impl.S(18), L"Dawnstar Remastered " DAWNSTAR_LAUNCHER_VERSION_W,
                 DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    // The update note sits beside the version, left of the button. Only the
    // Available and Installing states have anything to say here; a
    // successful "you are up to date" is not news and stays quiet.
    if (!impl.updateNote.empty()) {
        const int noteWidth = impl.ButtonX() - margin - impl.S(20);
        const COLORREF noteColor =
            impl.updateState == UpdateState::Failed ? kMuted : kGold;
        DrawTextLine(dc, impl.uiFont, noteColor, margin, impl.P(kUpdateY), noteWidth,
                     impl.S(28), impl.updateNote, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    }
    (void)height;

    BitBlt(target, 0, 0, width, height, dc, 0, 0, SRCCOPY);
    SelectObject(dc, oldBitmap);
    DeleteObject(surface);
    DeleteDC(dc);
}

// ---------------------------------------------------------------------

constexpr DWORD kWindowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

// Settles the window's size, and with it impl.clientWidth/panelTop. See
// shadowkey-decomp's own SizeToArtwork for the full reasoning (unchanged
// here): the preferred width is kClientWidth and the header height follows
// the artwork's proportions, shrinking both together if the screen is too
// short, cropping the artwork only below kMinClientWidth.
void SizeToArtwork(Impl& impl, DWORD style) {
    RECT work{0, 0, 0, 0};
    if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0)) {
        work.right = GetSystemMetrics(SM_CXSCREEN);
        work.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    RECT frame{0, 0, 0, 0};
    AdjustWindowRect(&frame, style, FALSE);
    const int frameWidth = (frame.right - frame.left);
    const int frameHeight = (frame.bottom - frame.top);

    const int panelHeight = impl.S(kPanelHeight);
    const int maxClientWidth = (work.right - work.left) - frameWidth;
    const int maxClientHeight = (work.bottom - work.top) - frameHeight;

    int width = (std::min)(impl.S(kClientWidth), (std::max)(impl.S(kMinClientWidth),
                                                            maxClientWidth));
    int header = HeaderHeightFor(impl.banner, width, impl.S(kFallbackHeaderHeight));

    if (header + panelHeight > maxClientHeight) {
        const int allowedHeader = (std::max)(impl.S(80), maxClientHeight - panelHeight);
        const int narrower = WidthForHeaderHeight(impl.banner, allowedHeader);
        if (narrower >= impl.S(kMinClientWidth) && narrower < width) {
            width = narrower;
            header = HeaderHeightFor(impl.banner, width, impl.S(kFallbackHeaderHeight));
        } else {
            header = allowedHeader;
        }
    }

    impl.clientWidth = width;
    impl.panelTop = header;

    RECT desired{0, 0, width, header + panelHeight};
    AdjustWindowRect(&desired, style, FALSE);
    SetWindowPos(impl.hwnd, nullptr, 0, 0, desired.right - desired.left,
                 desired.bottom - desired.top, SWP_NOMOVE | SWP_NOZORDER);
}

// `x` and `y` are device pixels (y already panel-relative via Impl::P);
// `w` and `h` are logical.
HWND MakeButton(Impl& impl, Button& button, const wchar_t* caption, int id, int x, int y, int w,
                int h, ButtonStyle style) {
    button.style = style;
    button.hwnd = CreateWindowExW(0, L"BUTTON", caption,
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, x, y,
                                  impl.S(w), impl.S(h), impl.hwnd,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                  GetModuleHandleW(nullptr), nullptr);
    SetWindowSubclass(button.hwnd, ButtonSubclass, static_cast<UINT_PTR>(id),
                      reinterpret_cast<DWORD_PTR>(&button.hovered));
    return button.hwnd;
}

void CreateControls(Impl& impl) {
    const int buttonX = impl.ButtonX();
    MakeButton(impl, impl.chooseData, L"Choose file…", IDC_CHOOSE_DATA, buttonX,
               impl.P(kRow1Button), kButtonWidth, kButtonHeight, ButtonStyle{});
    MakeButton(impl, impl.play, L"Play", IDC_PLAY, buttonX, impl.P(kPlayY), kButtonWidth,
               kPlayHeight, ButtonStyle{true, false});
    MakeButton(impl, impl.update, L"Update", IDC_UPDATE, buttonX, impl.P(kUpdateY),
               kButtonWidth, kButtonHeight, ButtonStyle{});
    ShowWindow(impl.update.hwnd, SW_HIDE);  // only shown when one exists
    for (int i = 0; i < 3; ++i) {
        const std::wstring caption = std::to_wstring(kScaleChoices[i]) + L"×";
        MakeButton(impl, impl.scale[i], caption.c_str(), IDC_SCALE_FIRST + i,
                   impl.S(kMargin + i * (kScaleWidth + 8)), impl.P(kScaleY), kScaleWidth,
                   kScaleHeight, ButtonStyle{});
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    Impl* impl = Self(hwnd);
    if (!impl) return DefWindowProcW(hwnd, message, wParam, lParam);

    switch (message) {
        case WM_ERASEBKGND:
            return 1;  // PaintWindow covers every pixel; erasing first only flickers
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(hwnd, &paint);
            PaintWindow(*impl, dc);
            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_DRAWITEM: {
            auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            const Button* button = nullptr;
            switch (static_cast<int>(wParam)) {
                case IDC_CHOOSE_DATA: button = &impl->chooseData; break;
                case IDC_PLAY: button = &impl->play; break;
                case IDC_UPDATE: button = &impl->update; break;
                default:
                    if (static_cast<int>(wParam) >= IDC_SCALE_FIRST &&
                        static_cast<int>(wParam) < IDC_SCALE_FIRST + 3) {
                        button = &impl->scale[static_cast<int>(wParam) - IDC_SCALE_FIRST];
                    }
                    break;
            }
            if (!button) return FALSE;
            PaintButton(*impl, *button, item);
            return TRUE;
        }
        case WM_COMMAND: {
            const int id = LOWORD(wParam);
            if (HIWORD(wParam) != BN_CLICKED) break;
            if (impl->installing) return 0;  // nothing is safe to start mid-swap
            if (id == IDC_CHOOSE_DATA) ChooseGameData(*impl);
            else if (id == IDC_PLAY) Play(*impl);
            else if (id == IDC_UPDATE) {
                impl->installing = true;
                impl->updateState = UpdateState::Installing;
                impl->updateNote = L"Starting…";
                Refresh(*impl);
                SetStatus(*impl, L"Updating to " + Widen(impl->availableRelease.tag) +
                                     L". Please don't close this window.",
                          kCaption);
                StartInstall(hwnd, impl->installRoot, impl->availableRelease);
            } else if (id >= IDC_SCALE_FIRST && id < IDC_SCALE_FIRST + 3) {
                impl->config.scale = kScaleChoices[id - IDC_SCALE_FIRST];
                Save(*impl);
                Refresh(*impl);
            }
            return 0;
        }

        // --- the update worker's three messages -------------------------
        //
        // Each arrives with a heap-allocated payload this handler owns and
        // frees. The worker is detached and never touches `Impl`, so a
        // check still in flight when the window closes simply fails to post
        // and cleans up after itself.
        case WM_APP_UPDATE_CHECKED: {
            std::unique_ptr<UpdateCheckResult> result(
                reinterpret_cast<UpdateCheckResult*>(lParam));
            if (!result->ok) {
                // Not being able to reach GitHub is not an error worth
                // shouting about -- the launcher's job is to start a game.
                impl->updateState = UpdateState::Failed;
                impl->updateNote.clear();
                Refresh(*impl);
                return 0;
            }
            const int comparison = dawnstar::launcher::CompareVersions(DAWNSTAR_LAUNCHER_VERSION,
                                                                        result->release.version);
            if (comparison < 0) {
                impl->updateState = UpdateState::Available;
                impl->availableRelease = result->release;
                impl->updateNote = L"Version " + Widen(result->release.version) + L" is available";
            } else {
                impl->updateState = UpdateState::UpToDate;
                impl->updateNote.clear();
            }
            Refresh(*impl);
            return 0;
        }
        case WM_APP_UPDATE_PROGRESS: {
            std::unique_ptr<UpdateProgressMessage> message(
                reinterpret_cast<UpdateProgressMessage*>(lParam));
            const dawnstar::launcher::UpdateProgress& progress = message->progress;
            if (progress.stage == dawnstar::launcher::UpdateStage::Downloading &&
                progress.bytesTotal > 0) {
                impl->updateNote = L"Downloading " + FormatSize(progress.bytesSoFar) + L" of " +
                                   FormatSize(progress.bytesTotal);
            } else if (!progress.message.empty()) {
                impl->updateNote = Widen(progress.message);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_APP_UPDATE_DONE: {
            std::unique_ptr<UpdateDoneMessage> done(
                reinterpret_cast<UpdateDoneMessage*>(lParam));
            impl->installing = false;
            if (!done->ok) {
                impl->updateState = UpdateState::Failed;
                impl->updateNote.clear();
                Refresh(*impl);
                SetStatus(*impl, L"The update failed: " + Widen(done->error), kBad);
                return 0;
            }
            // This executable has just been renamed aside and replaced, so
            // the new one has to take over from here.
            if (dawnstar::launcher::RelaunchLauncher(impl->installRoot)) {
                DestroyWindow(hwnd);
            } else {
                impl->updateState = UpdateState::UpToDate;
                impl->updateNote.clear();
                Refresh(*impl);
                SetStatus(*impl,
                          L"Updated to " + Widen(done->tag) +
                              L". Close and reopen Dawnstar to use it.",
                          kGold);
            }
            return 0;
        }

        case WM_CLOSE:
            if (impl->installing) {
                // Half-replaced installs are how people end up with a
                // folder that no longer starts.
                SetStatus(*impl, L"Please wait — files are being replaced right now.",
                          kBad);
                return 0;
            }
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    // APARTMENTTHREADED because IFileOpenDialog requires an STA.
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);

    Impl impl;
    impl.installRoot = dawnstar::ExecutableDirectory();
    impl.configPath = (fs::path(impl.installRoot) / LauncherConfig::kFileName).string();
    impl.banner = dawnstar::launcher::LoadEmbeddedBanner();
    dawnstar::launcher::LoadLauncherConfig(impl.configPath, impl.config);

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;  // WM_ERASEBKGND is handled
    windowClass.lpszClassName = L"DawnstarLauncherWindow";
    windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APPICON));
    windowClass.hIconSm = windowClass.hIcon;
    RegisterClassExW(&windowClass);

    // Created hidden at a nominal size first, because the DPI that decides
    // the real size is only knowable once there is a window to ask about.
    impl.hwnd = CreateWindowExW(0, windowClass.lpszClassName,
                                L"Dawnstar Remastered",
                                kWindowStyle, CW_USEDEFAULT, CW_USEDEFAULT, kClientWidth,
                                kClientWidth, nullptr, nullptr, instance, &impl);
    if (!impl.hwnd) {
        if (SUCCEEDED(comResult)) CoUninitialize();
        return 1;
    }

    impl.dpi = QueryDpi(impl.hwnd);
    impl.uiFont = MakeFont(impl.dpi, 10, FW_NORMAL);
    impl.captionFont = MakeFont(impl.dpi, 8, FW_SEMIBOLD);
    impl.valueFont = MakeFont(impl.dpi, 10, FW_NORMAL);
    impl.playFont = MakeFont(impl.dpi, 13, FW_SEMIBOLD);

    SizeToArtwork(impl, kWindowStyle);
    impl.seamColor = SeamColorFor(impl.banner);

    CreateControls(impl);
    Refresh(impl);

    // Clears out whatever the last update left behind, then asks GitHub
    // whether there is a newer Dawnstar build -- on a worker thread,
    // because a slow or unreachable network must never delay the window
    // appearing.
    if (dawnstar::launcher::UpdatesEnabledForThisBuild(DAWNSTAR_LAUNCHER_VERSION)) {
        dawnstar::launcher::CleanUpPreviousUpdate(impl.installRoot);
        impl.updateState = UpdateState::Checking;
        StartUpdateCheck(impl.hwnd);
    } else {
        impl.updateState = UpdateState::Disabled;
    }

    if (impl.dataOk) {
        SetStatus(impl, L"Ready to play.", kMuted);
    } else {
        SetStatus(impl,
                  L"Dawnstar Remastered doesn't include the game itself. Point it at your "
                  L"own copy of TEST-Dawnstar.jar to get started.",
                  kMuted);
    }

    ShowWindow(impl.hwnd, showCommand);
    UpdateWindow(impl.hwnd);

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(impl.hwnd, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    if (impl.uiFont) DeleteObject(impl.uiFont);
    if (impl.captionFont) DeleteObject(impl.captionFont);
    if (impl.valueFont) DeleteObject(impl.valueFont);
    if (impl.playFont) DeleteObject(impl.playFont);
    if (SUCCEEDED(comResult)) CoUninitialize();
    return 0;
}
