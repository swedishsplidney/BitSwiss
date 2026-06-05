#include "StorageWriter.hpp"
#include <fstream>
#include <iostream>

WriteResult StorageWriter::WritePackagesToDestination(const std::string& target_path, const std::vector<Package*>& selected_packages) {
    WriteResult result = { false, "", 0 };

    // verify the target path
    if (!fs::exists(target_path)) {
        result.error_message = "target file destination point does not exist!";
        return result;
    }

    try {
        // create standardized folder
        fs::path bitswiss_root = fs::path(target_path) / "BitSwiss_archive";
        if (!fs::exists(bitswiss_root)) {
            fs::create_directories(bitswiss_root);
        }

        // loop through the selected packages
        for (const auto& pkg : selected_packages) {
            if (!pkg) continue; // safety check

            // debug: see what the writer is receiving
            std::cout << "debug: writer processing id: " << pkg->id << " | target filename value: " << pkg->target_filename << std::endl;

            // use dynamically discovered file name with .zim as fallback
            std::string filename = pkg->target_filename.empty() ? (pkg->id + ".zim") : pkg->target_filename;
            fs::path final_output_file = bitswiss_root / filename;

            // open high-speed output file stream in strict binary mode
            std::ofstream disk_stream(final_output_file, std::ios::binary | std::ios::out | std::ios::trunc);

            if (!disk_stream.is_open()) {
                result.error_message = "OS permission failure. could not write to: " + pkg->name;
                return result;
            }

            // write an initial small, uncorrupted identifier block
            // this is to ensure the stream handles data well before adding actual network data
            std::string header_signature = "BITSWISS-V1\n";
            std::string meta_data = "PKG_ID: " + pkg->id + "\nSIZE_EXPECTED: " + std::to_string(pkg->size_in_bytes) + "\n---\n";

            disk_stream.write(header_signature.c_str(), header_signature.size());
            disk_stream.write(meta_data.c_str(), meta_data.size());

            result.bytes_written += (header_signature.size() + meta_data.size());

            // flush buffers and close cleanly
            disk_stream.flush();
            disk_stream.close();
        }

        result.success = true;
    }
    catch (const fs::filesystem_error& e) {
        result.error_message = std::string("critical OS drive failure: ") + e.what();
        return result;
    }

    return result;
}