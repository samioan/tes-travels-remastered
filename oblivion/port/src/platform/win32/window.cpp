#include "platform/win32/window.h"

#include <windows.h>

namespace oblivion {

struct Window::Impl {
    HWND hwnd = nullptr;
    int clientWidth = 0;
    int clientHeight = 0;
    KeyCallback onKey;
    BITMAPINFO bmi{};
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
            if (impl && impl->onKey) impl->onKey(static_cast<unsigned>(wParam));
            return 0;
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
    impl_->clientWidth = clientWidth;
    impl_->clientHeight = clientHeight;

    BITMAPINFOHEADER& h = impl_->bmi.bmiHeader;
    h.biSize = sizeof(BITMAPINFOHEADER);
    h.biWidth = Backbuffer::kWidth;
    h.biHeight = -Backbuffer::kHeight;  // top-down
    h.biPlanes = 1;
    h.biBitCount = 32;
    h.biCompression = BI_RGB;

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

    RECT rect = {0, 0, clientWidth, clientHeight};
    DWORD style = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
    AdjustWindowRect(&rect, style, FALSE);
    impl_->hwnd = CreateWindowExW(0, kClassName, title.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT,
                                  rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr,
                                  GetModuleHandleW(nullptr), impl_);
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

void Window::Present(const Backbuffer& bb) {
    HDC dc = GetDC(impl_->hwnd);
    StretchDIBits(dc, 0, 0, impl_->clientWidth, impl_->clientHeight, 0, 0, Backbuffer::kWidth,
                  Backbuffer::kHeight, bb.Data(), &impl_->bmi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(impl_->hwnd, dc);
}

void Window::SetTitle(const std::wstring& title) { SetWindowTextW(impl_->hwnd, title.c_str()); }

void* Window::Handle() const { return impl_->hwnd; }

bool Window::HasFocus() const { return GetForegroundWindow() == impl_->hwnd; }

bool Window::CursorPos(int* x, int* y) const {
    POINT pt;
    if (!GetCursorPos(&pt) || !ScreenToClient(impl_->hwnd, &pt)) return false;
    RECT rc;
    GetClientRect(impl_->hwnd, &rc);
    if (pt.x < 0 || pt.y < 0 || pt.x >= rc.right || pt.y >= rc.bottom || rc.right <= 0 || rc.bottom <= 0) return false;
    *x = pt.x * Backbuffer::kWidth / rc.right;
    *y = pt.y * Backbuffer::kHeight / rc.bottom;
    return true;
}

void Window::RequestClose() { PostMessageW(impl_->hwnd, WM_CLOSE, 0, 0); }

}  // namespace oblivion
