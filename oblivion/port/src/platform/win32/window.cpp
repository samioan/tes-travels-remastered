#include "platform/win32/window.h"

#include <windows.h>

#include <algorithm>

namespace oblivion {

struct Window::Impl {
    HWND hwnd = nullptr;
    KeyCallback onKey;
    bool fullscreen = false;
    DWORD windowedStyle = 0;
    WINDOWPLACEMENT windowedPlacement = {sizeof(WINDOWPLACEMENT)};
    // Where the last frame landed in the client area (for mouse aiming).
    int vpX = 0, vpY = 0, vpW = 1, vpH = 1;
    int srcW = Backbuffer::kWidth, srcH = Backbuffer::kHeight;
};

namespace {

constexpr DWORD kWindowedStyle = WS_OVERLAPPEDWINDOW;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window::Impl* impl = reinterpret_cast<Window::Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            // Alt+Enter is fullscreen too (reported as F11); other Alt combos stay with the system.
            if (msg == WM_SYSKEYDOWN && wParam == VK_RETURN) {
                if (impl && impl->onKey && !(lParam & (1 << 30))) impl->onKey(VK_F11);
                return 0;
            }
            if (msg == WM_SYSKEYDOWN && wParam != VK_F10) return DefWindowProcW(hwnd, msg, wParam, lParam);
            if (impl && impl->onKey) impl->onKey(static_cast<unsigned>(wParam));
            return 0;
        case WM_ERASEBKGND:
            return 1;  // Present paints everything, bars included
        case WM_SETCURSOR:
            if (impl && impl->fullscreen && LOWORD(lParam) == HTCLIENT) {
                SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)));
                return TRUE;
            }
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

}  // namespace

Window::Window(int clientWidth, int clientHeight, const std::wstring& title) {
    impl_ = new Impl();
    static const wchar_t* kClassName = L"OblivionPortWindow";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = kClassName;
        wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        RegisterClassW(&wc);
        registered = true;
    }

    // Never open bigger than the desktop's work area.
    RECT work = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    RECT rect = {0, 0, clientWidth, clientHeight};
    AdjustWindowRect(&rect, kWindowedStyle, FALSE);
    int w = rect.right - rect.left, h = rect.bottom - rect.top;
    if (work.right > work.left) {
        w = std::min<int>(w, work.right - work.left);
        h = std::min<int>(h, work.bottom - work.top);
    }
    impl_->hwnd = CreateWindowExW(0, kClassName, title.c_str(), kWindowedStyle, CW_USEDEFAULT, CW_USEDEFAULT, w, h,
                                  nullptr, nullptr, GetModuleHandleW(nullptr), impl_);
    ShowWindow(impl_->hwnd, SW_SHOW);
}

Window::~Window() { delete impl_; }

void Window::SetKeyCallback(KeyCallback cb) { impl_->onKey = std::move(cb); }

void Window::RunMessageLoop(const IdleCallback& onIdle) {
    MSG msg = {};
    for (;;) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        onIdle();
        Sleep(1);
    }
}

void Window::ClientSize(int* w, int* h) const {
    RECT rc;
    GetClientRect(impl_->hwnd, &rc);
    *w = rc.right - rc.left;
    *h = rc.bottom - rc.top;
}

void Window::Present(const Backbuffer& bb, Scaling scaling) {
    HDC dc = GetDC(impl_->hwnd);
    int cw, ch;
    ClientSize(&cw, &ch);
    const int sw = bb.RealWidth(), sh = Backbuffer::kHeight;
    int w = cw, h = ch;
    if (cw > 0 && ch > 0) {
        if (scaling == Scaling::Integer && cw >= sw && ch >= sh) {
            const int k = std::min(cw / sw, ch / sh);
            w = sw * k;
            h = sh * k;
        } else if (static_cast<long long>(cw) * sh >= static_cast<long long>(ch) * sw) {
            h = ch;  // pillarbox
            w = static_cast<int>(static_cast<long long>(ch) * sw / sh);
        } else {
            w = cw;  // letterbox
            h = static_cast<int>(static_cast<long long>(cw) * sh / sw);
        }
    }
    const int x = (cw - w) / 2, y = (ch - h) / 2;
    impl_->vpX = x;
    impl_->vpY = y;
    impl_->vpW = std::max(1, w);
    impl_->vpH = std::max(1, h);
    impl_->srcW = sw;
    impl_->srcH = sh;

    HBRUSH black = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    const RECT bars[4] = {{0, 0, cw, y}, {0, y + h, cw, ch}, {0, y, x, y + h}, {x + w, y, cw, y + h}};
    for (const RECT& b : bars)
        if (b.right > b.left && b.bottom > b.top) FillRect(dc, &b, black);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = sw;
    bmi.bmiHeader.biHeight = -sh;  // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    SetStretchBltMode(dc, COLORONCOLOR);  // nearest neighbour keeps the pixel art crisp
    StretchDIBits(dc, x, y, w, h, 0, 0, sw, sh, bb.Data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(impl_->hwnd, dc);
}

void Window::SetTitle(const std::wstring& title) { SetWindowTextW(impl_->hwnd, title.c_str()); }

void Window::SetFullscreen(bool on) {
    HWND hwnd = impl_->hwnd;
    if (on == impl_->fullscreen) return;
    if (on) {
        impl_->windowedStyle = static_cast<DWORD>(GetWindowLongW(hwnd, GWL_STYLE));
        GetWindowPlacement(hwnd, &impl_->windowedPlacement);
        MONITORINFO mi = {sizeof(mi)};
        if (!GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi)) return;
        SetWindowLongW(hwnd, GWL_STYLE, static_cast<LONG>((impl_->windowedStyle & ~WS_OVERLAPPEDWINDOW) | WS_POPUP));
        SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
    } else {
        SetWindowLongW(hwnd, GWL_STYLE, static_cast<LONG>(impl_->windowedStyle ? impl_->windowedStyle : kWindowedStyle));
        SetWindowPlacement(hwnd, &impl_->windowedPlacement);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
    impl_->fullscreen = on;
    InvalidateRect(hwnd, nullptr, TRUE);
}

bool Window::IsFullscreen() const { return impl_->fullscreen; }

void* Window::Handle() const { return impl_->hwnd; }

bool Window::HasFocus() const { return GetForegroundWindow() == impl_->hwnd; }

bool Window::CursorPos(int* x, int* y) const {
    POINT pt;
    if (!GetCursorPos(&pt) || !ScreenToClient(impl_->hwnd, &pt)) return false;
    int cw, ch;
    ClientSize(&cw, &ch);
    if (pt.x < 0 || pt.y < 0 || pt.x >= cw || pt.y >= ch) return false;
    // Through the letterboxing, into backbuffer pixels (bars map outside [0, width)).
    *x = (pt.x - impl_->vpX) * impl_->srcW / impl_->vpW;
    *y = (pt.y - impl_->vpY) * impl_->srcH / impl_->vpH;
    return true;
}

void Window::RequestClose() { PostMessageW(impl_->hwnd, WM_CLOSE, 0, 0); }

}  // namespace oblivion
