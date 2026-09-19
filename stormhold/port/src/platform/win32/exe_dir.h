#pragma once
#include <string>

namespace stormhold {

// The directory the running executable lives in -- what the launcher (and,
// via STORMHOLD_USER_DIR, the game itself) treats as the install root.
// Mirrors dawnstar's (and, before that, shadowkey-decomp's) identical
// platform/win32/exe_dir.h.
std::string ExecutableDirectory();

}  // namespace stormhold
