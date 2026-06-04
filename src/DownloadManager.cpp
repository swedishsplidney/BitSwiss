#include "DownloadManager.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

// 1. receive incoming chunks from interwebs
size_t DownloadManager::WriteDataCallback(void *ptr, size_t size, size_t nmemb, void *stream) {
    std::ofstream* out_stream =  static_cast<std::ofstream*>(stream);
    size_t total_bytes = size * nmemb;
    if (out_stream && out_stream->is_open()) {
        out_stream->write(static_cast<const char*>(ptr), total_bytes);
        return total_bytes;
    }
    return 0;
}

// 2. intercept download metrics from curl and update the progress value
int DownloadManager::ProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    if (clientp && dltotal > 0) {
        auto* progress = static_cast<std::shared_ptr<std::atomic<double>>*>(clientp);
        (**progress).store((dlnow / dltotal) * 100.0);
    }
    return 0;
}

// 3. isolated thread loop
void DownloadManager::DownloadWorkerTask(Package pkg, std::string destination_directory) {
    pkg.is_downloading->store(true);

    // create the destination file path
    fs::path target_file = fs::path(destination_directory) / "BitSwiss_archive" / (pkg.id + ".zim");

    // open stream in binary append/out mode
    std::ofstream output_stream(target_file, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!output_stream.is_open()) {
        std::cerr << "thread error: failed to open target file for writing: " << target_file << std::endl;
        pkg.is_downloading->store(false);
        return;
    }

    CURL* curl_handle = curl_easy_init();
    if (curl_handle) {
        // mock url endpoint for now, will link later
        std::string mock_url = "https://dumps.wikipedia.org/other/kiwix/zim/wikipedia/";

        curl_easy_setopt(curl_handle, CURLOPT_URL, mock_url.c_str());

        // pass custom filesystem stream pointer to callback function
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteDataCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &output_stream);

        // enable and add live structural progress hooks
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, ProgressCallback);

        // pass address of shared progress tracking pointer
        auto progress_ptr = &pkg.download_progress;
        curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, progress_ptr);

        // execute block-blocking network retrieval
        CURLcode res = curl_easy_perform(curl_handle);

        if (res != CURLE_OK) {
            std::cerr << "curl thread error " << curl_easy_strerror(res) << std::endl;
        } else {
            pkg.is_completed->store(true);
        }

        curl_easy_cleanup(curl_handle);
    }

    output_stream.flush();
    output_stream.close();
    pkg.is_downloading->store(false);
}

void DownloadManager::StartDownloadPool(const std::vector<Package>& selected_packages, const std::string& target_mount_path) {
    JoinAll(); // ensure old pools are cleaned up
    is_running = true;

    for (const auto& pkg : selected_packages) {
        // spawn a brand new OS thread for each individual package download
        worker_threads.push_back(std::thread(DownloadWorkerTask, pkg, target_mount_path));
    }
}

void DownloadManager::JoinAll() {
    for (auto& thread : worker_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads.clear();
    is_running = false;
}