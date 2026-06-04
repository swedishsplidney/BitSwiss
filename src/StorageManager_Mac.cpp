#ifdef __APPLE__
#include "StorageManager.hpp"
#include <sys/param.h>
#include <sys/mount.h>
#include <vector>

std::vector<DriveInfo> StorageManager::GetTargetDrives() {
    std::vector<DriveInfo> target_drives;
    struct statfs* mounts;
    int num_mounts = getmntinfo(&mounts, MNT_WAIT);

    for (int i = 0; i < num_mounts; i++) {
        std::string mount_on = mounts[i].f_mntonname;
        // string validations

        if (mount_on.rfind("/Volumes/", 0) == 0) {
            uint64_t byte_size = (uint64_t)mounts[i].f_blocks * mounts[i].f_bsize;

            DriveInfo info;
            info.device_path = mount_on;
            info.model_name = mount_on.substr(9);
            info.total_bytes = byte_size; // ensure that it matches the header parameter name
            info.is_usb = true;

            target_drives.push_back(info);
        }
    }
    return target_drives;
}
#endif