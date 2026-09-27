#include "platform/win32/window.h"

#include <windows.h>

#include <algorithm>

namespace dawnstar {

namespace {

// BITMAPINFO with room for the 3 BI_BITFIELDS color masks (RGB565) right
// after the header, as GDI expects.
struct Rgb565BitmapInfo {
    BITMAPINFOHEADER header;
    DWORD masks[3];
};

constexpr DWORD kWindowedStyle = WS_OVERLAPPEDWINDOW;

}  // namespace

struct Window::Impl {
    HWND hwnd = nullptr;
    bool closed = false;
    bool fullscreen = false;
    DWORD windowedStyle = 0;
    WINDOWPLACEMENT windowedPlacement = {sizeof(WINDOWPLACEMENT)};
    KeyCallback onKey;
    Scaling scaling = Scaling::Fit;
    Rgb565BitmapInfo bmi{};
};

namespace {

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
            if (impl && impl->onKey && !(lParam & (1 << 30))) impl->onKey(static_cast<unsigned>(wParam));
            return 0;
        case WM_ERASEBKGND:
            return 1;  // Present paints everything, bars included
        case WM_CLOSE:
            if (impl) impl->closed = true;
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

    impl_->bmi.header.biSize = sizeof(BITMAPINFOHEADER);
    impl_->bmi.header.biWidth = Backbuffer::kWidth;
    impl_->bmi.header.biHeight = -Backbuffer::kHeight;  // top-down
    impl_->bmi.header.biPlanes = 1;
    impl_->bmi.header.biBitCount = 16;
    impl_->bmi.header.biCompression = BI_BITFIELDS;
    impl_->bmi.masks[0] = 0xF800;  // R
    impl_->bmi.masks[1] = 0x07E0;  // G
    impl_->bmi.masks[2] = 0x001F;  // B

    static const wchar_t* kClassName = L"DawnstarPortWindow";
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = kClassName;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassW(&wc);
        classRegistered = true;
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

    ShowWindow(impl_->hwnd, SW_SHOWNORMAL);
}

Window::~Window() {
    if (impl_->hwnd) DestroyWindow(impl_->hwnd);
    delete impl_;
}

void Window::SetKeyCallback(KeyCallback cb) { impl_->onKey = std::move(cb); }

void Window::RunMessageLoop(const IdleCallback& onIdle) {
    MSG msg;
    while (!impl_->closed) {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        } else {
            onIdle();
        }
    }
    shouldClose_ = true;
}

void Window::Close() { PostMessageW(impl_->hwnd, WM_CLOSE, 0, 0); }

void Window::SetScaling(Scaling scaling) {
    impl_->scaling = scaling;
    InvalidateRect(impl_->hwnd, nullptr, TRUE);
}

void Window::Present(const Backbuffer& backbuffer) {
    const Scaling scaling = impl_->scaling;
    HDC hdc = GetDC(impl_->hwnd);

    RECT client;
    GetClientRect(impl_->hwnd, &client);
    const int cw = client.right - client.left;
    const int ch = client.bottom - client.top;
    const int sw = backbuffer.RealWidth(), sh = Backbuffer::kHeight;
    impl_->bmi.header.biWidth = sw;

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

    HBRUSH black = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    const RECT bars[4] = {{0, 0, cw, y}, {0, y + h, cw, ch}, {0, y, x, y + h}, {x + w, y, cw, y + h}};
    for (const RECT& b : bars)
        if (b.right > b.left && b.bottom > b.top) FillRect(hdc, &b, black);

    SetStretchBltMode(hdc, COLORONCOLOR);
    StretchDIBits(hdc, x, y, w, h, 0, 0, sw, sh, backbuffer.Data(),
                  reinterpret_cast<const BITMAPINFO*>(&impl_->bmi), DIB_RGB_COLORS, SRCCOPY);

    ReleaseDC(impl_->hwnd, hdc);
}

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

void Window::ClientSize(int* w, int* h) const {
    RECT client = {};
    GetClientRect(impl_->hwnd, &client);
    if (w) *w = client.right - client.left;
    if (h) *h = client.bottom - client.top;
}

void Window::SetClientSize(int width, int height) {
    if (impl_->fullscreen) return;
    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, kWindowedStyle, FALSE);
    RECT work = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int w = rect.right - rect.left, h = rect.bottom - rect.top;
    if (work.right > work.left) {
        w = std::min<int>(w, work.right - work.left);
        h = std::min<int>(h, work.bottom - work.top);
    }
    if (IsZoomed(impl_->hwnd)) ShowWindow(impl_->hwnd, SW_RESTORE);
    SetWindowPos(impl_->hwnd, nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    InvalidateRect(impl_->hwnd, nullptr, TRUE);
}

}  // namespace dawnstar
