#include "platform/win32/display.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>

namespace dawnstar {

int Display::WidthFor(Aspect aspect, int clientW, int clientH) {
    const int H = Backbuffer::kHeight, base = Backbuffer::kWidth;
    auto ratio = [&](int num, int den) { return (H * num + den / 2) / den; };
    int w = base;
    switch (aspect) {
        case Aspect::R4_3: w = ratio(4, 3); break;
        case Aspect::R16_10: w = ratio(16, 10); break;
        case Aspect::R16_9: w = ratio(16, 9); break;
        case Aspect::R21_9: w = ratio(21, 9); break;
        case Aspect::Auto:
            if (clientW > 0 && clientH > 0)
                w = static_cast<int>((static_cast<long long>(H) * clientW + clientH / 2) / clientH);
            break;
        default: break;
    }
    w += w & 1;  // even: see Backbuffer::Resize
    return std::max(base, std::min(Backbuffer::kMaxWidth, w));
}

int Display::LogicalWidth() const {
    int cw = 0, ch = 0;
    if (window_) window_->ClientSize(&cw, &ch);
    return WidthFor(s_.aspect, cw, ch);
}

void Display::ApplySize() {
    if (!window_) return;
    // Auto opens in the original shape (it follows whatever the window becomes);
    // a fixed ratio opens in its own.
    const int lw = s_.aspect == Aspect::Auto ? Backbuffer::kWidth : WidthFor(s_.aspect, 0, 0);
    window_->SetClientSize(lw * s_.scale, Backbuffer::kHeight * s_.scale);
}

void Display::Attach(Window* window) {
    window_ = window;
    if (!window_) return;
    ApplySize();
    window_->SetScaling(s_.scaling);
    if (s_.fullscreen) window_->SetFullscreen(true);
}

void Display::Load() {
    if (cfgPath_.empty()) return;
    FILE* f = nullptr;
    if (fopen_s(&f, cfgPath_.c_str(), "r") != 0 || !f) return;
    int fs = 0, sc = 0, scale = s_.scale, asp = static_cast<int>(s_.aspect);
    if (fscanf_s(f, "%d %d %d %d", &fs, &sc, &scale, &asp) >= 2) {
        if (asp >= 0 && asp < static_cast<int>(Aspect::Count)) s_.aspect = static_cast<Aspect>(asp);
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
    ApplySize();
    Save();
}

std::string Display::WidescreenName() const {
    switch (s_.aspect) {
        case Aspect::Auto: return "Auto";
        case Aspect::R4_3: return "4:3";
        case Aspect::R16_10: return "16:10";
        case Aspect::R16_9: return "16:9";
        case Aspect::R21_9: return "21:9";
        default: return "Off";
    }
}

void Display::CycleWidescreen(int dir) {
    const int n = static_cast<int>(Aspect::Count);
    s_.aspect = static_cast<Aspect>((static_cast<int>(s_.aspect) + dir + n) % n);
    ApplySize();
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
