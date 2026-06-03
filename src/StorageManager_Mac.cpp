#ifdef __APPLE__
#include "StorageManager.hpp"
#include <sys/param.h>
#include <sys/mount.h>
#include <vector>

std::vector<DriveInfo> StorageManager::GetTargetDrives() {
    std::vector<DriveInfo> target_drives;
    struct statfs*  mounts;
    int num_mounts = getmntinfo(&mounts, MNT_WAIT);

    for (int i = 0; i < num_mounts; i++) {
        std::string mount_on = mounts[i].f_mntonname;
        std::string mnt_from = mounts[i].f_mntfromname;

        if (mount_on.rfind("/Volumes/", 0) == 0) {
            uint64_t byte_size = mounts[i].f_blocks * mounts[i].f_bsize;

            DriveInfo info;
            info.device_path = mnt_from;
            info.model_name = mount_on.substr(9); // strip prefix
            info.total_size = byte_size;
            info.is_usb = true;

            target_drives.push_back(info);
        }
    }
    return target_drives;
}
#endif