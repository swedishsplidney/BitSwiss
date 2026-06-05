#pragma once
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <curl/curl.h>
#include "PackageManager.hpp"

class DownloadManager {
private:
    std::vector<std::thread> worker_threads;
    std::mutex queue_mutex;
    std::atomic<bool> is_running{false};
    std::atomic<bool> cancel_requested{false};

    // the actual raw work function that gets executed inside worker threads
    static void DownloadWorkerTask(Package* pkg, std::string destination_directory, std::atomic<bool>* global_cancel);

    // libcurl callback function to handle incoming data streams
    static size_t WriteDataCallback(void* ptr, size_t size, size_t nmemb, void* stream);

    // libcurl progress callback function
    static int ProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);

public:
    DownloadManager() { curl_global_init(CURL_GLOBAL_ALL); }
    ~DownloadManager() {
        CancelPool(); // hard abort streams
        curl_global_cleanup();
    }

    // spawns threads to process sellected allocations in parallel
    void StartDownloadPool(const std::vector<Package*>& selected_packages, const std::string& target_mount_path);

    // joins threads to clean up memory
    void JoinAll();

    void CancelPool();
    bool IsRunning() const { return is_running.load(); }
};