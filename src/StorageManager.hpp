#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstdint>

struct DriveInfo {
    std::string device_path; // e.g. '/dev/sdb' or 'E:\'
    std::string model_name; // e.g. 'SanDisk Ultra'
    uint64_t total_bytes; // e.g. 16000000000 (16gb)
    bool is_usb;
};

class StorageManager {
public:
    // returns a list of safely targetable storage devices
    static std::vector<DriveInfo> GetTargetDrives();
};