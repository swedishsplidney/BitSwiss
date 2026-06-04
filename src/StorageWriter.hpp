#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>
#include "PackageManager.hpp"

namespace fs = std::filesystem;

// response to send back to ImGui
struct WriteResult {
    bool success;
    std::string error_message;
    uint64_t bytes_written;
};

class StorageWriter {
    // will handle opening the drive, creating directories, and processing packages
    static WriteResult WritePackagesToDestination(const std::string& target_path, const std::vector<Package>& selected_packages);
};