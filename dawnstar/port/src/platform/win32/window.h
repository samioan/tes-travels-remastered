#pragma once
#include <functional>
#include <string>

#include "graphics/backbuffer.h"

namespace dawnstar {

// Minimal Win32 window + message pump, presenting a Backbuffer via GDI
// (StretchDIBits) each frame. No SDL2/OpenGL -- see docs/PORT_ROADMAP.md.
class Window {
public:
    // Called once per message-pump iteration where no window messages were
    // pending -- the host drives ticking/presenting from here.
    using IdleCallback = std::function<void()>;

    Window(int clientWidth, int clientHeight, const std::wstring& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Runs the message pump until the window is closed. Calls onIdle every
    // time there are no pending messages (i.e. every "frame").
    void RunMessageLoop(const IdleCallback& onIdle);

    // Presents the backbuffer, nearest-neighbor scaled to the client area.
    void Present(const Backbuffer& backbuffer);

    // Requests the window close itself (posts WM_CLOSE, the same real
    // message the OS sends for the title bar's own close button) --
    // M38's own real quit-confirmation flow needs to close the window
    // programmatically from inside the idle callback, not just react to
    // the user clicking the OS close button.
    void Close();

    bool ShouldClose() const { return shouldClose_; }

    // Opaque pimpl -- public only so window.cpp's free-function WndProc
    // (not a Window member) can name/define it; callers outside window.cpp
    // still can't do anything with an incomplete type.
    struct Impl;

private:
    Impl* impl_;
    bool shouldClose_ = false;
};

}  // namespace dawnstar
