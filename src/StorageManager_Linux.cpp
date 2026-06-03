#ifndef _WIN32
#ifndef __APPLE__

#include "StorageManager.hpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

std::vector<DriveInfo> StorageManager::GetTargetDrives() {
    std::vector<DriveInfo> target_drives;

    if (!fs::exists("/sys/class/block")) return target_drives;

    for (const auto& entry : fs::directory_iterator("/sys/class/block")) {
        std::string dev_name = entry.path().filename().string();

        // skip loop devices and virtual ram blocks
        if (dev_name.rfind("loop", 0) == 0 || dev_name.rfind("ram", 0) == 0) continue;

        // skip individual partitions
        if (fs::exists(entry.path() / "partition")) continue;

        std::string sys_path = entry.path().string();

        // 1 = removable drive, 0 = fixed (internal) drive
        std::ifstream removable_file(sys_path + "/removable");
        int removable = 0;
        if (!(removable_file >> removable) || removable != 1) continue;

        // get total size in 512 byte sectors
        std::ifstream size_file(sys_path + "/size");
        uint64_t sectors = 0;
        size_file >> sectors;
        uint64_t byte_size = sectors * 512;

        // fetch model name
        std::string model = "Unknown Removable Drive";
        std::ifstream model_file(sys_path + "/device/model");
        if (model_file) {
            std::getline(model_file, model);
        }

        DriveInfo info;
        info.device_path = "/dev/" + dev_name;
        info.model_name = model;
        info.total_bytes = byte_size;
        info.is_usb = true;

        target_drives.push_back(info);
    }

    return target_drives;
}

#endif
#endif