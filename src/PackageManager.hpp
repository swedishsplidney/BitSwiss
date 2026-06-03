#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Package {
    std::string name;
    std::string description;
    uint64_t size_in_bytes;
    bool is_selected = false;
};

struct Category {
    std::string name;
    std::vector<Package> packages;
};

class PackageManager {
public:
    static std::vector<Category> GetDefaultManifest() {
        std::vector<Category> manifest;

        // comp sci category
        Category cs;
        cs.name = "computer science & systems";
        cs.packages = {
            {"minimal GCC/Clang toolchain", "C/C++ core compilers and build tools.", 450ULL * 1024 *1024},
            {"rust language stack", "rustup installer, cargo, and standard library components.", 380ULL * 1024 * 1024},
            {"neovim IDE suite", "preconfigured lightweight terminal development environment.", 45ULL * 1024 * 1024}
        };
        manifest.push_back(cs);

        // circuitry and hardware
        Category hardware;
        hardware.name = "circuitry and robotics";
        hardware.packages = {
            {"kiCad EDA suite", "schematic capture and PCB layout software.", 1200ULL * 1024 * 1024},
            {"pulseView / sigrok", "logic analyzer frontend software for hardware signal debugging.", 85ULL * 1024 * 1024},
            {"avrDude compiler tool", "microcontroller code flashing binary for embedded chips.", 12ULL * 1024 * 1024}
        };
        manifest.push_back(hardware);

        // space and aerospace
        Category space;
        space.name = "aerospace & mission data";
        space.packages = {
            {"NASA exoplanet dataset", "offline archive of public telemetry and atmospheric charts.", 350ULL * 1024 * 1024},
            {"GNU radio runtime", "digital signal processing blocks for tracking real satellites.", 620ULL * 1024 * 1024},
            {"SGP4 orbit propagator", "calculates life coordinates for standard orbital bodies.", 18ULL * 1024 * 1024}
        };
        manifest.push_back(space);

        return manifest;
    }
};