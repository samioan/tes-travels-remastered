#pragma once
#include <string>

namespace dawnstar {

// The directory the running executable lives in -- what the launcher (and,
// via DAWNSTAR_USER_DIR, the game itself) treats as the install root.
// Mirrors shadowkey-decomp's platform/win32/exe_dir.h.
std::string ExecutableDirectory();

}  // namespace dawnstar
