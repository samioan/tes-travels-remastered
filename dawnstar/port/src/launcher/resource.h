#pragma once

// Resource ids for port/src/launcher/launcher.rc. Kept in its own header
// because both the .rc and banner.cpp need IDR_BANNER, and an .rc cannot
// include a C++ header that pulls in <vector>.

#define IDR_BANNER 101
#define IDI_APPICON 102
