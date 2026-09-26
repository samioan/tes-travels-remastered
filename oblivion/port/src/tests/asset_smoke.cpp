// Parses every extracted .scr/.jtm/.cml and checks the counts against what
// tools/parse_*.py --check reports (32 / 17 / 21 files, all parse to EOF).
#include <cstdio>
#include <filesystem>
#include <string>

#include "assets/asset_root.h"
#include "assets/cml.h"
#include "assets/image.h"
#include "assets/jtm.h"
#include "assets/scr.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    std::string dir = argc > 1 ? argv[1] : "../../extracted";
    oblivion::AssetRoot assets(dir);
    oblivion::ImageCache images(assets);
    int scr = 0, jtm = 0, cml = 0, failures = 0, insns = 0, layers = 0, groups = 0;
    for (const auto& e : fs::directory_iterator(dir)) {
        std::string ext = e.path().extension().string();
        std::string name = e.path().filename().string();
        try {
            if (ext == ".scr") {
                oblivion::Scr s = oblivion::ParseScr(assets.Read(name));
                for (int id = 0; id < 256; id++) insns += static_cast<int>(s.Script(id).size());
                scr++;
            } else if (ext == ".jtm") {
                oblivion::JtmMap m = oblivion::ParseJtm(assets.Read(name));
                layers += static_cast<int>(m.layers.size());
                jtm++;
            } else if (ext == ".cml") {
                oblivion::SpriteSet s = oblivion::ParseCml(assets.Read(name), images);
                groups += static_cast<int>(s.groups.size());
                cml++;
            }
        } catch (const std::exception& ex) {
            std::printf("FAIL %s: %s\n", name.c_str(), ex.what());
            failures++;
        }
    }
    std::printf("scr %d, jtm %d (%d layers), cml %d (%d groups), %d instructions, %d failures\n", scr, jtm,
                layers, cml, groups, insns, failures);
    bool ok = failures == 0 && scr == 32 && jtm == 17 && cml == 21;
    std::printf(ok ? "OK\n" : "MISMATCH\n");
    return ok ? 0 : 1;
}
