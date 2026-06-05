#include "DownloadManager.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

// courier structs so curl can track both progress and cancellations at the same time
struct ProgressContext {
    std::shared_ptr<std::atomic<double>>* package_progress;
    std::atomic<bool>* global_cancel;
};

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
    if (!clientp) return 0;

    auto* ctx = static_cast<ProgressContext*>(clientp);

    // if canceled, return 1 to drop curl connection
    if (ctx->global_cancel && ctx->global_cancel->load()) {
        return 1;
    }

    if (dltotal > 0 && ctx->package_progress) {
        (**ctx->package_progress).store((dlnow / dltotal) * 100.0);
    }

    return 0;
}

// 3. isolated thread loop
void DownloadManager::DownloadWorkerTask(Package* pkg, std::string destination_directory, std::atomic<bool>* global_cancel) {
    pkg->is_downloading->store(true);

    // create the destination file path
    fs::path archive_dir = fs::path(destination_directory) / "BitSwiss_archive";

    try {
        // force create the dir inside thread
        if (!fs::exists(archive_dir)) {
            fs::create_directories(archive_dir);
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "thread OS error: cannot create directory: " << e.what() << std::endl;
        pkg->is_downloading->store(false);
        return;
    }

    // reconstruct final target path using dynamic filename
    std::string filename = pkg->target_filename.empty() ? (pkg->id + ".zim") : pkg->target_filename;
    fs::path target_file = archive_dir / filename;

    // open stream in binary append/out mode
    std::ofstream output_stream(target_file, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!output_stream.is_open()) {
        std::cerr << "OS permission error. cannot write to: " << target_file << std::endl;
        std::cerr << "check if your drive is mounted as read-only!" << std::endl;
        pkg->is_downloading->store(false);
        return;
    }

    CURL* curl_handle = curl_easy_init();
    bool was_aborted = false;

    if (curl_handle) {
        // live url
        std::string live_url = pkg->download_url;
        std::cout << "thread - streaming from URL: " << live_url << std::endl;

        curl_easy_setopt(curl_handle, CURLOPT_URL, live_url.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
        // pass custom filesystem stream pointer to callback function
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteDataCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &output_stream);
        // enable and add live structural progress hooks
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, ProgressCallback);

        // pass address of shared progress tracking pointer
        ProgressContext ctx = { &(pkg->download_progress), global_cancel };
        curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, &ctx);

        // execute block-blocking network retrieval
        CURLcode res = curl_easy_perform(curl_handle);

        if (res == CURLE_ABORTED_BY_CALLBACK || (global_cancel && global_cancel->load())) {
            was_aborted = true;
            std::cout << "safely aborted: " << pkg->id << std::endl;
        } else if (res != CURLE_OK) {
            std::cerr << "curl thread error " << curl_easy_strerror(res) << std::endl;
        } else {
            // check what HTTP code the server returned
            long response_code = 0;
            curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
            std::cout << "server Response Code for " << pkg->id
                      << ": HTTP " << response_code << std::endl;

            if (response_code >= 400) {
                std::cerr << "-> error: server rejected the request or file doesn't exist!" << std::endl;
            } else {
                pkg->is_completed->store(true);
            }
        }

        curl_easy_cleanup(curl_handle);
    }

    output_stream.flush();
    output_stream.close();
    pkg->is_downloading->store(false);

    // clean up broken file if canceled midway
    if (was_aborted && fs::exists(target_file)) {
        fs::remove(target_file);
    }
}

void DownloadManager::StartDownloadPool(const std::vector<Package*>& selected_packages, const std::string& target_mount_path) {
    JoinAll(); // ensure old pools are cleaned up
    cancel_requested.store(false);
    is_running.store(true);

    for (Package* pkg : selected_packages) {
        // spawn a brand new OS thread for each individual package download
        worker_threads.push_back(std::thread(DownloadWorkerTask, pkg, target_mount_path, &cancel_requested));
    }
}

void DownloadManager::CancelPool() {
    cancel_requested.store(true);
    JoinAll();
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