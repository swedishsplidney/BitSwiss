#include "StorageManager.hpp"
#ifdef _WIN32
#include "storageManager.hpp"
#include <windows.h>
#include <vector>

std::vector<DriveInfo> StorageManager::GetTargetDrives() {
    std::vector<DriveInfo> target_drives;
    wchar_t drive_buffer[256];
    DWORD buffer_length = GetLogicalDriveStringsW(254, drive_buffer);

    if (buffer_length == 0) return target_drives;

    wchar_t* drive = drive_buffer;
    while (*drive) {
        if (GetDriveTypeW(drive) == DRIVE_REMOVABLE) {
            std::wstring w_drive_path(drive);
            std::string drive_path(w_drive_path.begin(), w_drive_path.end());

            ULARGE_INTEGER free_bytes, total_bytes, total_free_bytes;
            uint64_t byte_size = 0;

            if (GetDiskFreeSpaceExW(drive, &free_bytes, &total_bytes, &total_free_bytes)) {
                byte_size = total_bytes.QuadPart;
            }

            DriveInfo info;
            info.device_path = drive_path;
            info.model_name = "Removable Storage Drive";
            info.total_bytes = byte_size;
            info.is_usb = true;

            target_drives.push_back(info);
        }
        drive += lstrlenW(drive) + 1;
    }

    return target_drives;
}
#endif