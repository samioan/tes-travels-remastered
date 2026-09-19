#include "platform/win32/exe_dir.h"

#include <windows.h>

#include <filesystem>
#include <vector>

namespace stormhold {

std::string ExecutableDirectory() {
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD written =
            GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (written == 0) return std::string();
        if (written < buffer.size()) break;
        buffer.resize(buffer.size() * 2);  // truncated; try again with more room
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, buffer.data(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return std::string();
    std::string narrow(static_cast<size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, buffer.data(), -1, narrow.data(), size, nullptr, nullptr);
    return std::filesystem::path(narrow).parent_path().string();
}

}  // namespace stormhold
