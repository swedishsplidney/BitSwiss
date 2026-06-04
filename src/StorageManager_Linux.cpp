#ifndef _WIN32
#ifndef __APPLE__

#include "StorageManager.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

// helper function to resolve a base block device name (e.g. "sdb") to its active mount point
std::string FindMountPointForDevice(const std::string& dev_name) {
    std::ifstream mounts_file("/proc/mounts");
    if (!mounts_file.is_open()) return "";

    std::string line;
    while (std::getline(mounts_file, line)) {
        std::istringstream iss(line);
        std::string device, mount_point, fs_type;
        if (iss >> device >> mount_point >> fs_type) {
            // check if the mount matches either the base device (/dev/sdb) or a partition (/dev/sdb1)
            if (device == "/dev/" + dev_name || device.rfind("/dev/" + dev_name, 0) == 0) {
                return mount_point;
            }
        }
    }
    return "";
}

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

        // find where the OS has actually mounted this block hardware device
        std::string live_mount_path = FindMountPointForDevice(dev_name);

        // if the drive is plugged in but not currently mounted/opened by the OS file manager,
        // we skip it since standard file streams cannot write to an unmounted file table.
        if (live_mount_path.empty()) continue;

        // get total size in 512 byte sectors
        std::ifstream size_file(sys_path + "/size");
        uint64_t sectors = 0;
        size_file >> sectors;
        uint64_t byte_size = sectors * 512;

        // fetch model name
        std::string model = "unknown removable device";
        std::ifstream model_file(sys_path + "/device/model");
        if (model_file) {
            std::getline(model_file, model);
        }

        DriveInfo info;
        info.device_path = live_mount_path; // assign the folder destination path
        info.model_name = model;
        info.total_bytes = byte_size;
        info.is_usb = true;

        target_drives.push_back(info);
    }

    return target_drives;
}

#endif
#endif