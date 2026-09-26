#include "platform/win32/display.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>

namespace dawnstar {

void Display::Attach(Window* window) {
    window_ = window;
    if (!window_) return;
    window_->SetClientSize(Backbuffer::kWidth * s_.scale, Backbuffer::kHeight * s_.scale);
    window_->SetScaling(s_.scaling);
    if (s_.fullscreen) window_->SetFullscreen(true);
}

void Display::Load() {
    if (cfgPath_.empty()) return;
    FILE* f = nullptr;
    if (fopen_s(&f, cfgPath_.c_str(), "r") != 0 || !f) return;
    int fs = 0, sc = 0, scale = s_.scale;
    if (fscanf_s(f, "%d %d %d", &fs, &sc, &scale) >= 2) {
        s_.fullscreen = fs != 0;
        s_.scaling = sc == 1 ? Scaling::Integer : Scaling::Fit;
        s_.scale = std::max(kMinScale, std::min(kMaxScale, scale));
    }
    std::fclose(f);
}

void Display::Save() const {
    if (cfgPath_.empty()) return;
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(cfgPath_).parent_path(), ec);
    FILE* f = nullptr;
    if (fopen_s(&f, cfgPath_.c_str(), "w") != 0 || !f) return;
    std::fprintf(f, "%d %d %d\n", s_.fullscreen ? 1 : 0, s_.scaling == Scaling::Integer ? 1 : 0, s_.scale);
    std::fclose(f);
}

void Display::CycleResolution(int dir) {
    const int n = kMaxScale - kMinScale + 1;
    s_.scale = kMinScale + (s_.scale - kMinScale + dir + n) % n;
    if (window_) window_->SetClientSize(Backbuffer::kWidth * s_.scale, Backbuffer::kHeight * s_.scale);
    Save();
}

void Display::ToggleFullscreen() {
    s_.fullscreen = !s_.fullscreen;
    if (window_) window_->SetFullscreen(s_.fullscreen);
    Save();
}

void Display::CycleScaling() {
    s_.scaling = s_.scaling == Scaling::Fit ? Scaling::Integer : Scaling::Fit;
    if (window_) window_->SetScaling(s_.scaling);
    Save();
}

}  // namespace dawnstar
