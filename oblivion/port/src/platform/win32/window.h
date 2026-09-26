#pragma once
#include <functional>
#include <string>

#include "graphics/backbuffer.h"

namespace oblivion {

// Minimal Win32 window + message pump presenting a Backbuffer through GDI
// (StretchDIBits), nearest-neighbour scaled to the client area. Same shape as
// the sibling stormhold/dawnstar ports platform layer; no SDL/OpenGL.
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

    void Present(const Backbuffer& backbuffer);
    void RequestClose();

    struct Impl;

private:
    Impl* impl_;
};

}  // namespace oblivion
