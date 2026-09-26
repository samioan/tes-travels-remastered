#include "platform/win32/display.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>

namespace oblivion {

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
            if (clientW > 0 && clientH > 0) w = static_cast<int>((static_cast<long long>(H) * clientW + clientH / 2) / clientH);
            break;
        default: break;
    }
    return std::max(base, std::min(Backbuffer::kMaxWidth, w));
}

void Display::Attach(Window* window) {
    window_ = window;
    if (window_ && s_.fullscreen) window_->SetFullscreen(true);
}

int Display::LogicalWidth() const {
    int cw = 0, ch = 0;
    if (window_) window_->ClientSize(&cw, &ch);
    return WidthFor(s_.aspect, cw, ch);
}

void Display::InitialClientSize(int scale, int* w, int* h) const {
    // Auto opens in the original shape; a fixed ratio opens in its own.
    const int lw = s_.aspect == Aspect::Auto ? Backbuffer::kWidth : WidthFor(s_.aspect, 0, 0);
    *w = lw * scale;
    *h = Backbuffer::kHeight * scale;
}

void Display::Load() {
    if (cfgPath_.empty()) return;
    FILE* f = std::fopen(cfgPath_.c_str(), "r");
    if (!f) return;
    int fs = 0, sc = 0, asp = static_cast<int>(Aspect::Auto);
    if (std::fscanf(f, "%d %d %d", &fs, &sc, &asp) >= 2) {
        s_.fullscreen = fs != 0;
        s_.scaling = sc == 1 ? Scaling::Integer : Scaling::Fit;
        if (asp >= 0 && asp < static_cast<int>(Aspect::Count)) s_.aspect = static_cast<Aspect>(asp);
    }
    std::fclose(f);
}

void Display::Save() const {
    if (cfgPath_.empty()) return;
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(cfgPath_).parent_path(), ec);
    FILE* f = std::fopen(cfgPath_.c_str(), "w");
    if (!f) return;
    std::fprintf(f, "%d %d %d\n", s_.fullscreen ? 1 : 0, s_.scaling == Scaling::Integer ? 1 : 0,
                 static_cast<int>(s_.aspect));
    std::fclose(f);
}

std::string Display::ResolutionName() const {
    switch (s_.aspect) {
        case Aspect::Original: return "Original";
        case Aspect::Auto: return "Auto";
        case Aspect::R4_3: return "4:3";
        case Aspect::R16_10: return "16:10";
        case Aspect::R16_9: return "16:9";
        case Aspect::R21_9: return "21:9";
        default: return "";
    }
}

void Display::CycleResolution(int dir) {
    const int n = static_cast<int>(Aspect::Count);
    s_.aspect = static_cast<Aspect>((static_cast<int>(s_.aspect) + dir + n) % n);
    Save();
}

void Display::ToggleFullscreen() {
    s_.fullscreen = !s_.fullscreen;
    if (window_) window_->SetFullscreen(s_.fullscreen);
    Save();
}

void Display::CycleScaling() {
    s_.scaling = s_.scaling == Scaling::Fit ? Scaling::Integer : Scaling::Fit;
    Save();
}

}  // namespace oblivion
