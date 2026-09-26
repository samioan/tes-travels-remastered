#pragma once
#include <functional>
#include <string>

#include "graphics/backbuffer.h"

namespace stormhold {

// How the 176x208 backbuffer is fitted into the window: the largest size that
// keeps its aspect ratio (Fit), or only whole multiples of it (Integer, crisp
// pixels). Either way the picture is centred with black bars, never stretched.
enum class Scaling { Fit, Integer };

// Minimal Win32 window + message pump, presenting a Backbuffer via GDI
// (StretchDIBits) each frame. No SDL2/OpenGL -- see docs/PORT_ROADMAP.md.
// Resizable, with borderless fullscreen.
class Window {
public:
    // Called once per message-pump iteration where no window messages were
    // pending -- the host drives ticking/presenting from here.
    using IdleCallback = std::function<void()>;
    // Called with the Win32 virtual-key code on key down (F11, Alt+Enter as F11).
    using KeyCallback = std::function<void(unsigned vk)>;

    Window(int clientWidth, int clientHeight, const std::wstring& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void SetKeyCallback(KeyCallback cb);

    // Runs the message pump until the window is closed. Calls onIdle every
    // time there are no pending messages (i.e. every "frame").
    void RunMessageLoop(const IdleCallback& onIdle);

    // Presents the backbuffer, centred and scaled per SetScaling (Fit by default).
    void Present(const Backbuffer& backbuffer);
    void SetScaling(Scaling scaling);

    // Borderless fullscreen on the monitor the window is on; leaving restores
    // the previous position and size.
    void SetFullscreen(bool on);
    bool IsFullscreen() const;
    // Resizes the windowed client area (ignored while fullscreen -- it applies
    // on leaving fullscreen).
    void SetClientSize(int width, int height);
    void ClientSize(int* w, int* h) const;  // the current client area

    // Lets the app itself end RunMessageLoop from inside its own onIdle
    // callback (same effect as the close button).
    void RequestClose();

    bool ShouldClose() const { return shouldClose_; }

    // Opaque pimpl -- public only so window.cpp's free-function WndProc
    // (not a Window member) can name/define it; callers outside window.cpp
    // still can't do anything with an incomplete type.
    struct Impl;

private:
    Impl* impl_;
    bool shouldClose_ = false;
};

}  // namespace stormhold
