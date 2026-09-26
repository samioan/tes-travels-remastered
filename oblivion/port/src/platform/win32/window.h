#pragma once
#include <functional>
#include <string>

#include "graphics/backbuffer.h"

namespace oblivion {

// How the backbuffer is fitted into the window: the largest size keeping its
// aspect ratio (Fit), or only whole multiples of it (Integer, crisp pixels).
enum class Scaling { Fit, Integer };

// Win32 window + message pump presenting a Backbuffer through GDI
// (StretchDIBits, nearest neighbour), centred with black bars. The window is
// resizable and can go borderless fullscreen; the backbuffer's own width may
// change from frame to frame (widescreen).
class Window {
public:
    using IdleCallback = std::function<void()>;
    // Called with the Win32 virtual-key code on key down (auto-repeat included).
    using KeyCallback = std::function<void(unsigned vk)>;

    Window(int clientWidth, int clientHeight, const std::wstring& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void SetKeyCallback(KeyCallback cb);

    // Runs the message pump until the window closes; calls onIdle whenever no
    // messages are pending.
    void RunMessageLoop(const IdleCallback& onIdle);

    void Present(const Backbuffer& backbuffer, Scaling scaling = Scaling::Fit);
    void RequestClose();
    void SetTitle(const std::wstring& title);

    // Borderless fullscreen on the monitor the window is on; leaving restores the
    // previous position and size.
    void SetFullscreen(bool on);
    bool IsFullscreen() const;
    // The client area in pixels (what Auto resolution follows).
    void ClientSize(int* w, int* h) const;

    // The native handle (void* so this header stays free of windows.h).
    void* Handle() const;
    bool HasFocus() const;
    // The mouse cursor in backbuffer coordinates (of the last presented frame);
    // false when it is outside the window.
    bool CursorPos(int* x, int* y) const;

    struct Impl;

private:
    Impl* impl_;
};

}  // namespace oblivion
