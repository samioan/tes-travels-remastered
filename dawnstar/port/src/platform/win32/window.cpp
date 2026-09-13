#include "platform/win32/window.h"

#include <windows.h>

namespace dawnstar {

namespace {

// BITMAPINFO with room for the 3 BI_BITFIELDS color masks (RGB565) right
// after the header, as GDI expects.
struct Rgb565BitmapInfo {
    BITMAPINFOHEADER header;
    DWORD masks[3];
};

}  // namespace

struct Window::Impl {
    HWND hwnd = nullptr;
    int clientWidth = 0;
    int clientHeight = 0;
    bool closed = false;
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
    impl_->clientWidth = clientWidth;
    impl_->clientHeight = clientHeight;

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

    RECT rect = {0, 0, clientWidth, clientHeight};
    DWORD style = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
    AdjustWindowRect(&rect, style, FALSE);

    impl_->hwnd = CreateWindowExW(
        0, kClassName, title.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr,
        GetModuleHandleW(nullptr), impl_);

    ShowWindow(impl_->hwnd, SW_SHOWNORMAL);
}

Window::~Window() {
    if (impl_->hwnd) DestroyWindow(impl_->hwnd);
    delete impl_;
}

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

void Window::Present(const Backbuffer& backbuffer) {
    HDC hdc = GetDC(impl_->hwnd);

    RECT client;
    GetClientRect(impl_->hwnd, &client);
    int w = client.right - client.left;
    int h = client.bottom - client.top;

    SetStretchBltMode(hdc, COLORONCOLOR);
    StretchDIBits(
        hdc, 0, 0, w, h, 0, 0, Backbuffer::kWidth, Backbuffer::kHeight,
        backbuffer.Data(), reinterpret_cast<const BITMAPINFO*>(&impl_->bmi),
        DIB_RGB_COLORS, SRCCOPY);

    ReleaseDC(impl_->hwnd, hdc);
}

}  // namespace dawnstar
