// Placeholder entry point -- opens a blank window to prove the build
// toolchain works. Replace with the real port once stormhold/decompiled/
// has been read through and renamed (see ../../docs/ROADMAP.md).
#include <windows.h>

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
  const wchar_t* kClassName = L"StormholdPortWindow";

  WNDCLASSW wc = {};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = kClassName;
  RegisterClassW(&wc);

  HWND hwnd = CreateWindowExW(
      0, kClassName, L"Stormhold Port", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 320, 240,
      nullptr, nullptr, hInstance, nullptr);
  if (!hwnd) return 0;

  ShowWindow(hwnd, nCmdShow);

  MSG msg = {};
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return 0;
}
