#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <future>
#include <chrono>
#include <sstream>
#include <cctype>
#include <curl/curl.h>
#include <atomic>
#include <memory>
#include <thread>

// core data structure
struct Package {
    std::string id;             // unique lookup key
    std::string name;           // ui title
    std::string description;    // short description
    std::string download_url;   // location of download
    uint64_t size_in_bytes;     // real allocation metrics
    bool is_selected = false;   //globally tracked state

    std::string target_filename = "";

    std::shared_ptr<std::atomic<double>> download_progress = std::make_shared<std::atomic<double>>(0.0);
    std::shared_ptr<std::atomic<bool>> is_downloading = std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<std::atomic<bool>> is_completed = std::make_shared<std::atomic<bool>>(false);
};

// preconfig data struct
struct PreconfigProfile {
    std::string name;
    std::string description;
    std::vector<std::string> package_ids;
};



class PackageManager {
private:
    std::map<std::string, Package> master_db;
    std::vector<PreconfigProfile> profiles;
    std::atomic<bool> sizing_finished{false};

public:
    PackageManager() {
        InitializeMasterDatabase();
        InitializeProfiles();
    }

    bool IsSizingFinished() const { return sizing_finished.load(); }

    std::map<std::string, Package>& GetMasterDB() { return master_db; }
    std::vector<PreconfigProfile>& GetProfiles() { return profiles; }

    // callback function to read incoming HTTP header strings line by line
    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
        size_t num_bytes = size * nitems;
        std::string header_line(buffer, num_bytes);
        uint64_t* final_size = static_cast<uint64_t*>(userdata);

        // parse out the content-length line
        if (header_line.rfind("Content-Length:", 0) == 0 || header_line.rfind("content-length:", 0) == 0) {
            std::stringstream ss;
            for (char c : header_line) {
                if (std::isdigit(c)) ss << c;
            }
            ss >> *final_size;
        }
        return num_bytes;
    }

    // memory safe live size fetcher using libcurl
    static uint64_t FetchRemoteSize(const std::string& url, const std::string& id) {
        if (url.empty()) return 0;

        CURL* curl = curl_easy_init();
        if (!curl) return 0;

        uint64_t fetched_size = 0;

        // configure the libcurl ez handle
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);          // headers only, do not download the body
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback); // assign header loop callback
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &fetched_size);      // pass reference pointer to store data
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);        // 10-second timeout window
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow redirects

        // execute the native network request inside our own thread context
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "failed to ping " << id << " -> " << curl_easy_strerror(res) << std::endl;
            // fallback value if server doesn't respond during launch
            fetched_size = 1ULL * 1024 * 1024 * 1024; // 1gb
        }

        curl_easy_cleanup(curl);
        return fetched_size;
    }

    void UpdateManifestSizesAsync() {
        sizing_finished.store(false);
        std::cout << "initializing libcurl file size framework..." << std::endl;

        // isolate map iterators safely from the sequential loop positions
        std::vector<std::string> all_keys;
        for (const auto& pair : master_db) {
            all_keys.push_back(pair.first);
        }

        // vector to keep track of concurrent worker threads
        std::vector<std::thread> workers;

        for (const std::string& key : all_keys) {
            // spawn a dedicated thread for each lookup
            workers.push_back(std::thread([this, key]() {
                std::string  url = this->master_db[key].download_url;

                // fetch the live size from the server
                uint64_t fetched_size = FetchRemoteSize(url, key);
                // write the result
                this->master_db[key].size_in_bytes = fetched_size;

                // detect file extension by parsing the URL string backwards to get the file name and extension
                size_t last_slash = url.rfind("/");
                if (last_slash != std::string::npos) {
                    std::string raw_filename = url.substr(last_slash + 1);

                    // clean out query strings (if they even exist)
                    size_t query_idx = raw_filename.rfind("?");
                    if (query_idx != std::string::npos) {
                        raw_filename = raw_filename.substr(0, query_idx);
                    }

                    this->master_db[key].target_filename = raw_filename;
                } else {
                    // safety fallback for if the URL does not contain a valid path slash
                    this->master_db[key].target_filename = key + ".zim";
                }
            }));
        }

        // wait for all threads to finish
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        sizing_finished.store(true);
        std::cout << "finished file size synchronization!" << std::endl;
    }

private:
    void InitializeMasterDatabase() {
        // readers
        master_db["tool_reader_linux"]    = {"tool_reader_linux", "kiwix reader for linux (.AppImage)", "portable self-contained runtime file. can be used to read all the .zim files here fully offline on linux", "https://download.kiwix.org/release/kiwix-desktop/kiwix-desktop_x86_64_2.4.1.appimage"};
        master_db["tool_reader_win"]      = {"tool_reader_win", "kiwix reader for Windows (.exe ZIP)", "standalone .zim reader tool. runs out of the box on Windows devices without setup", "https://download.kiwix.org/release/kiwix-desktop/kiwix-desktop_windows_x64.zip"};
        master_db["tool_reader_mac"]      = {"tool_reader_mac", "kiwix reader for macOS (.dmg)", "apple disk image bundle containing an offline .zim reading application", "https://download.kiwix.org/release/kiwix-macos/kiwix-macos.dmg"};
        // master dataset list
        master_db["wiki_en_all"]          = {"wiki_en_all", "wikipedia english (full)", "the entire english wikipedia with images and tables.", "https://download.kiwix.org/zim/wikipedia/wikipedia_en_all_maxi_2026-02.zim"};
        master_db["wiki_en_nopic"]        = {"wiki_en_nopic", "wikipedia english (text only)", "complete text articles with 0 images for space efficiency.", "https://download.kiwix.org/zim/wikipedia/wikipedia_en_all_nopic_2026-03.zim"};
        master_db["wiki_en_mini"]         = {"wiki_en_mini", "wikipedia english (intros only)", "wikipedia but only the headers and introductory paragraphs to save a large amount of space", "https://download.kiwix.org/zim/wikipedia/wikipedia_en_all_mini_2026-03.zim"};
        master_db["stackoverflow_full"]   = {"stackoverflow_full", "stackoverflow archive (full)", "complete searchable stackoverflow archive with lots of programming knowledge", "https://download.kiwix.org/zim/stack_exchange/stackoverflow.com_en_all_2023-11.zim"};
        master_db["stackexchange_3dp"]    = {"stackexchange_3dp", "stackexchange 3d printing archive", "complete stackexchange 3d printing archive for 3d printer people", "https://download.kiwix.org/zim/stack_exchange/3dprinting.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_academ"] = {"stackexchange_academ", "stackexchange academia archive", "complete stackexchange academia archive for hardcore academics", "https://download.kiwix.org/zim/stack_exchange/academia.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_ai"]     = {"stackexchange_ai", "stackexchange AI archive", "complete stackexchange archive on all things artificial intelligence", "https://download.kiwix.org/zim/stack_exchange/ai.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_andr"]   = {"stackexchange_andr", "stackexchange android archive", "complete stackexchange archive on android stuff", "https://download.kiwix.org/zim/stack_exchange/android.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_anime"]  = {"stackexchange_anime", "stackexchange anime archive", "complete stackexchange weeb archive", "https://download.kiwix.org/zim/stack_exchange/anime.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_apple"]  = {"stackexchange_apple", "stackexchange apple archive", "complete stackexchange archive on apple (tech company, not fruit)", "https://download.kiwix.org/zim/stack_exchange/apple.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_arduin"] = {"stackexchange_arduin", "stackexchange arduino archive", "complete stackexchange arduino archive for your arduinoing", "https://download.kiwix.org/zim/stack_exchange/arduino.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_ubuntu"] = {"stackexchange_ubuntu", "stackexchange askubuntu archive", "complete stackexchange archive of ubuntu knowledge", "https://download.kiwix.org/zim/stack_exchange/askubuntu.com_en_all_2025-12.zim"};
        master_db["stackexchange_stars"]  = {"stackexchange_stars", "stackexchange astronomy archive", "complete stackexchange archive of knowledge of the stars", "https://download.kiwix.org/zim/stack_exchange/astronomy.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_planes"] = {"stackexchange_planes", "stackexchange aviation archive", "complete stackexchange archive of aviation stuff", "https://download.kiwix.org/zim/stack_exchange/aviation.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_bio"]    = {"stackexchange_bio", "stackexchange biology archive", "complete stackexchange archive of biology information", "https://download.kiwix.org/zim/stack_exchange/biology.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_blend"]  = {"stackexchange_blend", "stackexchange blender archive", "complete stackexchange archive of the 3d modeling program blender", "https://download.kiwix.org/zim/stack_exchange/blender.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_chem"]   = {"stackexchange_chem", "stackexchange chemistry archive", "complete stackexchange archive of chemistry knowledge", "https://download.kiwix.org/zim/stack_exchange/chemistry.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_compgr"] = {"stackexchange_compgr", "stackexchange computer graphics archive", "complete stackexchange computer graphics knowledge archive", "https://download.kiwix.org/zim/stack_exchange/computergraphics.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_cookin"] = {"stackexchange_cookin", "stackexchange cooking archive", "complete stackexchange archive of cooking knowledge", "https://download.kiwix.org/zim/stack_exchange/cooking.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_cs"]     = {"stackexchange_cs", "stackexchange computer science archive", "complete stackexchange computer science knowledge archive", "https://download.kiwix.org/zim/stack_exchange/cs.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_data"]   = {"stackexchange_data", "stackexchange data science archive", "complete stackexchange data science archive", "https://download.kiwix.org/zim/stack_exchange/datascience.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_earth"]  = {"stackexchange_earth", "stackexchange earth science archive", "complete stackexchange earth science archive", "https://download.kiwix.org/zim/stack_exchange/earthscience.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_econ"]   = {"stackexchange_econ", "stackexchange economics archive", "complete stackexchange economics knowledge archive", "https://download.kiwix.org/zim/stack_exchange/economics.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_elect"]  = {"stackexchange_elect", "stackexchange electronics archive", "complete stackexchange electronics information archive", "https://download.kiwix.org/zim/stack_exchange/electronics.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_fit"]    = {"stackexchange_fit", "stackexchange fitness archive", "complete stackexchange fitness information archive", "https://download.kiwix.org/zim/stack_exchange/fitness.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_gdev"]   = {"stackexchange_gdev", "stackexchange gamedev archive", "complete stackexchange game development archive", "https://download.kiwix.org/zim/stack_exchange/gamedev.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_garden"] = {"stackexchange_garden", "stackexchange gardening archive", "complete stackexchange gardening knowledge archive", "https://download.kiwix.org/zim/stack_exchange/gardening.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_gdesgn"] = {"stackexchange_gdesgn", "stackexchange graphic design archive", "complete stackexchange graphic design information archive", "https://download.kiwix.org/zim/stack_exchange/graphicdesign.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_law"]    = {"stackexchange_law", "stackexchange law archive", "complete stackexchange law archive", "https://download.kiwix.org/zim/stack_exchange/law.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_liter"]  = {"stackexchange_liter", "stackexchange literature archive", "complete stackexchange literature knowledge archives", "https://download.kiwix.org/zim/stack_exchange/literature.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_math"]   = {"stackexchange_math", "stackexchange math archive", "complete stackexchange math knowledge archive", "https://download.kiwix.org/zim/stack_exchange/math.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_mechan"] = {"stackexchange_mechan", "stackexchange mechanics archive", "complete stackexchange mechanics information archive", "https://download.kiwix.org/zim/stack_exchange/mechanics.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_mytho"]  = {"stackexchange_mytho", "stackexchange mythology archive", "complete stackexchange mythology knowledge archive", "https://download.kiwix.org/zim/stack_exchange/mythology.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_parent"] = {"stackexchange_parent", "stackexchange parenting archive", "complete stackexchange parenting archive to raise kids", "https://download.kiwix.org/zim/stack_exchange/parenting.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_physi"]  = {"stackexchange_physi", "stackexchange physics archive", "conplete stackexchange physics knowledge archive", "https://download.kiwix.org/zim/stack_exchange/physics.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_psycho"] = {"stackexchange_psycho", "stackexchange psychology archive", "complete stackexchange psychology information archive", "https://download.kiwix.org/zim/stack_exchange/psychology.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_quant"]  = {"stackexchange_quant", "stackexchange quantum computing archive", "complete stackexchange quantum computing knowledge archive", "https://download.kiwix.org/zim/stack_exchange/quantumcomputing.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_raspb"]  = {"stackexchange_raspb", "stackexchange raspberry pi archive", "complete stackexchange raspberry pi information archive", "https://download.kiwix.org/zim/stack_exchange/raspberrypi.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_robo"]   = {"stackexchange_robo", "stackexchange robotics archive", "complete stackexchange robotics knowledge archive", "https://download.kiwix.org/zim/stack_exchange/robotics.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_sftwen"] = {"stackexchange_sftwen", "stackexchange software engineering archive", "complete stackexchange software engineering knowledge archive", "https://download.kiwix.org/zim/stack_exchange/softwareengineering.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_space"]  = {"stackexchange_space", "stackexchange space archive", "complete stackexchange space information archive", "https://download.kiwix.org/zim/stack_exchange/space.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_unix"]   = {"stackexchange_unix", "stackexchange unix archive", "complete stackexchange  unix information archive", "https://download.kiwix.org/zim/stack_exchange/unix.stackexchange.com_en_all_2026-02.zim"};
        master_db["stackexchange_writin"] = {"stackexchange_writin", "stackexchange writing archive", "complete stackexchange writing knowledge archive", "https://download.kiwix.org/zim/stack_exchange/writing.stackexchange.com_en_all_2026-02.zim"};
        master_db["gutenberg"]            = {"gutenberg", "english gutenberg library archive (full)", "collection of thousands of public domain ebooks", "https://download.kiwix.org/zim/gutenberg/gutenberg_en_all_2023-08.zim"};
        master_db["gutenberg_medi"]       = {"gutenberg_medi", "gutenberg library medical collection", "large collection of public domain medical ebooks", "https://download.kiwix.org/zim/gutenberg/gutenberg_en_lcc-r_2025-12.zim"};
        master_db["gutenberg_sci"]        = {"gutenberg_sci", "gutenberg library science collection", "large collection of public domain science ebooks", "https://download.kiwix.org/zim/gutenberg/gutenberg_en_lcc-q_2026-03.zim"};
        master_db["gutenberg_tech"]       = {"gutenberg_tech", "gutenberg library technology collection", "large collection of public domain technology ebooks", "https://download.kiwix.org/zim/gutenberg/gutenberg_en_lcc-t_2026-03.zim"};
        master_db["ifixit"]               = {"ifixit", "ifixit information archive (full", "full ifixit archive with guides to fix almost any devices", "https://download.kiwix.org/zim/ifixit/ifixit_en_all_2025-12.zim"};
        master_db["devdocs_async"]        = {"devdocs_async", "developer documentation for async", "documentation archive for developing with async", "https://download.kiwix.org/zim/devdocs/devdocs_en_async_2026-05.zim"};
        master_db["devdocs_bash"]         = {"devdocs_bash", "bash developer documentation", "documentation archive for coding with bash", "https://download.kiwix.org/zim/devdocs/devdocs_en_bash_2026-04.zim"};
        master_db["devdocs_bootstrap"]    = {"devdocs_bootstrap", "bootstrap developer documentation", "documentation archive for developing with bootstrap", "https://download.kiwix.org/zim/devdocs/devdocs_en_bootstrap_2026-04.zim"};
        master_db["devdocs_c"]            = {"devdocs_c", "developer documentation for C", "documentation archive useful for coding in C", "https://download.kiwix.org/zim/devdocs/devdocs_en_c_2026-04.zim"};
        master_db["devdocs_cmake"]        = {"devdocs_cmake", "CMake developer documentation", "documentation archive for using CMake", "https://download.kiwix.org/zim/devdocs/devdocs_en_cmake_2026-05.zim"};
        master_db["devdocs_cpp"]          = {"devdocs_cpp", "C++ developer documentation", "documentation archive for coding in C++", "https://download.kiwix.org/zim/devdocs/devdocs_en_cpp_2026-04.zim"};
        master_db["devdocs_css"]          = {"devdocs_css", "css developer documentation", "documentation archive for coding in css", "https://download.kiwix.org/zim/devdocs/devdocs_en_css_2026-04.zim"};
        master_db["devdocs_docker"]       = {"devdocs_docker", "docker developer documentation", "documentation archive for using docker", "https://download.kiwix.org/zim/devdocs/devdocs_en_docker_2026-04.zim"};
        master_db["devdocs_gcc"]          = {"devdocs_gcc", "gcc developer documentation", "documentation archive for using gcc", "https://download.kiwix.org/zim/devdocs/devdocs_en_gcc_2026-05.zim"};
        master_db["devdocs_git"]          = {"devdocs_git", "git developer documentation", "documentation archive for using git", "https://download.kiwix.org/zim/devdocs/devdocs_en_git_2026-04.zim"};
        master_db["devdocs_gnumake"]      = {"devdocs_gnumake", "GNU-make developer documentation", "documentation archive for using GNU-make", "https://download.kiwix.org/zim/devdocs/devdocs_en_gnu-make_2026-04.zim"};
        master_db["devdocs_go"]           = {"devdocs_go", "Go developer documentation", "documentation archive for coding with golang", "https://download.kiwix.org/zim/devdocs/devdocs_en_go_2026-04.zim"};
        master_db["devdocs_godot"]        = {"devdocs_godot", "Godot developer documentation", "documentation archive for using Godot", "https://download.kiwix.org/zim/devdocs/devdocs_en_godot_2026-04.zim"};
        master_db["devdocs_haxe"]         = {"devdocs_haxe", "haxe developer documentation", "documentation archive for coding with haxe", "https://download.kiwix.org/zim/devdocs/devdocs_en_haxe_2026-04.zim"};
        master_db["devdocs_homebrew"]     = {"devdocs_homebrew", "homebrew developer documentation", "documentation archive for using homebrew", "https://download.kiwix.org/zim/devdocs/devdocs_en_homebrew_2026-04.zim"};
        master_db["devdocs_html"]         = {"devdocs_html", "html developer documentation", "documentation archive for coding with html", "https://download.kiwix.org/zim/devdocs/devdocs_en_html_2026-04.zim"};
        master_db["devdocs_http"]         = {"devdocs_http", "http developer documentation", "documentation archive for using http", "https://download.kiwix.org/zim/devdocs/devdocs_en_http_2026-04.zim"};
        master_db["devdocs_js"]           = {"devdocs_js", "javascript developer documentation", "documentation archive for coding with javascript", "https://download.kiwix.org/zim/devdocs/devdocs_en_javascript_2026-04.zim"};
        master_db["devdocs_jekyll"]       = {"devdocs_jekyll", "jekyll developer documentation", "documentation archive for using jekyll", "https://download.kiwix.org/zim/devdocs/devdocs_en_jekyll_2026-04.zim"};
        master_db["devdocs_kotlin"]       = {"devdocs_kotlin", "kotlin developer documentation", "documentation archive for coding with kotlin", "https://download.kiwix.org/zim/devdocs/devdocs_en_kotlin_2026-04.zim"};
        master_db["devdocs_lua"]          = {"devdocs_lua", "lua developer documentation", "documentation archive for coding with lua", "https://download.kiwix.org/zim/devdocs/devdocs_en_lua_2026-05.zim"};
        master_db["devdocs_markdown"]     = {"devdocs_markdown", "markdown developer documentation", "documentation archive for writing in markdown", "https://download.kiwix.org/zim/devdocs/devdocs_en_markdown_2026-04.zim"};
        master_db["devdocs_meteor"]       = {"devdocs_meteor", "meteorJS developer documentation", "documentation archive for using meteorJS", "https://download.kiwix.org/zim/devdocs/devdocs_en_meteor_2026-04.zim"};
        master_db["devdocs_node"]         = {"devdocs_node", "nodeJS developer documentation", "documentation archive for using nodeJS", "https://download.kiwix.org/zim/devdocs/devdocs_en_node_2026-05.zim"};
        master_db["devdocs_numpy"]        = {"devdocs_numpy", "numPy developer documentation", "documentation archive for using numPy", "https://download.kiwix.org/zim/devdocs/devdocs_en_numpy_2026-04.zim"};
        master_db["devdocs_opengl"]       = {"devdocs_opengl", "openGL developer documentation", "documentation archive for using openGL", "https://download.kiwix.org/zim/devdocs/devdocs_en_opengl_2026-04.zim"};
        master_db["devdocs_openjdk"]      = {"devdocs_openjdk", "openJDK developer documentation", "documentation archive for using openJDK", "https://download.kiwix.org/zim/devdocs/devdocs_en_openjdk_2026-05.zim"};
        master_db["devdocs_php"]          = {"devdocs_php", "php developer documentation", "documentation archjive for using php", "https://download.kiwix.org/zim/devdocs/devdocs_en_php_2026-05.zim"};
        master_db["devdocs_pygame"]       = {"devdocs_pygame", "pygame developer documentation", "documentation archive for using pygame", "https://download.kiwix.org/zim/devdocs/devdocs_en_pygame_2026-04.zim"};
        master_db["devdocs_python"]       = {"devdocs_python", "python developer documentation", "documentation archive for coding with python", "https://download.kiwix.org/zim/devdocs/devdocs_en_python_2026-05.zim"};
        master_db["devdocs_pytorch"]      = {"devdocs_pytorch", "pytorch developer documentation", "documentation archive for using pytorch", "https://download.kiwix.org/zim/devdocs/devdocs_en_pytorch_2026-04.zim"};
        master_db["devdocs_qt"]           = {"devdocs_qt", "qt developer documentation", "documentation archive for using qt", "https://download.kiwix.org/zim/devdocs/devdocs_en_qt_2026-04.zim"};
        master_db["devdocs_redux"]        = {"devdocs_redux", "redux developer documentation", "documentation archive for using redux", "https://download.kiwix.org/zim/devdocs/devdocs_en_redux_2026-04.zim"};
        master_db["devdocs_ruby"]         = {"devdocs_ruby", "ruby developer documentation", "documentation archive for coding in ruby", "https://download.kiwix.org/zim/devdocs/devdocs_en_ruby_2026-04.zim"};
        master_db["devdocs_rust"]         = {"devdocs_rust", "rust developer documentation", "documentation archive for coding in rust", "https://download.kiwix.org/zim/devdocs/devdocs_en_rust_2026-04.zim"};
        master_db["devdocs_sqlite"]       = {"devdocs_sqlite", "sqlite developer documentation", "documentation archive for using sqlite", "https://download.kiwix.org/zim/devdocs/devdocs_en_sqlite_2026-04.zim"};
        master_db["devdocs_threejs"]      = {"devdocs_threejs", "threeJS developer documentation", "documentation archive for using threeJS", "https://download.kiwix.org/zim/devdocs/devdocs_en_threejs_2026-03.zim"};
        master_db["devdocs_typescript"]   = {"devdocs_typescript", "typescript developer documentation", "documentation archive for using typescript", "https://download.kiwix.org/zim/devdocs/devdocs_en_typescript_2026-04.zim"};
        master_db["devdocs_vulkan"]       = {"devdocs_vulkan", "vulkan developer documentation", "documentation archive for using vulkan", "https://download.kiwix.org/zim/devdocs/devdocs_en_vulkan_2026-04.zim"};
        master_db["devdocs_wordpress"]    = {"devdocs_wordpress", "wordpress developer documentation", "documentation archive for using wordpress", "https://download.kiwix.org/zim/devdocs/devdocs_en_wordpress_2026-04.zim"};
        master_db["devdocs_zsh"]          = {"devdocs_zsh", "z-shell developer documentation", "documentation archive for coding with z-shell", "https://download.kiwix.org/zim/devdocs/devdocs_en_zsh_2026-04.zim"};
        master_db["freecodecamp"]         = {"freecodecamp", "all of the freecodecamp curriculum", "learn to code offline from anywhere", "https://download.kiwix.org/zim/freecodecamp/freecodecamp_en_all_2026-05.zim"};
        master_db["libretexts_bio"]       = {"libretexts_bio", "libretexts biology collection", "large collection of biology textbooks and educational material", "https://download.kiwix.org/zim/libretexts/libretexts.org_en_bio_2025-01.zim"};
        master_db["libretexts_biz"]       = {"libretexts_biz", "libretexts business collection", "large collection of business textbooks and educational material", "https://download.kiwix.org/zim/libretexts/libretexts.org_en_biz_2026-01.zim"};
        master_db["libretexts_chem"]      = {"libretexts_chem", "libretexts chemistry collection", "large collection of chemistry textbooks and educational material", "https://download.kiwix.org/zim/libretexts/libretexts.org_en_chem_2025-01.zim"};
        master_db["libretexts_eng"]       = {"libretexts_eng", "libretexts engineering collection", "large collection of engineering textbooks and educational material", "https://download.kiwix.org/zim/libretexts/libretexts.org_en_eng_2025-01.zim"};
        master_db["libretexts_geo"]       = {"libretexts_geo", "libretexts geosciences collection", "large collection of geoscience textbooks and educational material", "https://download.kiwix.org/zim/libretexts/libretexts.org_en_geo_2026-01.zim"};
        master_db["libretexts_human"]     = {"libretexts_human", "libretexts humanities collection", "large collection of humanities textsbooks and educational material", "https://download.kiwix.org/zim/libretexts/libretexts.org_en_human_2025-01.zim"};
        master_db["libretexts_math"]      = {"libretexts_math", "libretexts math collection", "large collection of math textbooks and educational material", "https://download.kiwix.org/zim/libretexts/libretexts.org_en_math_2026-01.zim"};
        master_db["libretexts_med"]       = {"libretexts_med", "libretexts medical collection", "large collection of medical textbooks and educational material", "https://download.kiwix.org/zim/libretexts/libretexts.org_en_med_2025-01.zim"};
        master_db["libretexts_phys"]      = {"libretexts_phys", "libretexts physics collection", "large collection of physics textbooks and educational material", "https://download.kiwix.org/zim/libretexts/libretexts.org_en_phys_2026-01.zim"};
        master_db["libretexts_socialsci"] = {"libretexts_socialsci", "libretexts social science collection", "large collection of social science textbooks and educational material", "https://download.kiwix.org/zim/libretexts/libretexts.org_en_socialsci_2025-01.zim"};
        master_db["libretexts_stats"]     = {"libretexts_stats", "libretexts statistics collection", "large collection of statistics textbooks and educational material"};
        master_db["khanacademy"]          = {"khanacademy", "khan academy (full)", "working archive of all of khan academy", "https://download.kiwix.org/zim/other/khanacademy_en_all_2023-03.zim"};
        master_db["crashcourse"]          = {"crashcourse", "crashcourse (full)", "full archive of all of crashcourse", "https://download.kiwix.org/zim/other/crashcourse_en_all_2026-05.zim"};
        master_db["minecraft"]            = {"minecraft", "minecraft wiki (full)", "archive of the full minecraft wiki", "https://download.kiwix.org/zim/other/minecraftwiki_en_all_maxi_2026-05.zim"};
        master_db["wiktionary"]           = {"wiktionary", "wiktionary (full)", "full archive of wikitionary", "https://download.kiwix.org/zim/wiktionary/wiktionary_en_all_nopic_2026-05.zim"};
        master_db["nasa_apod"]            = {"nasa_apod", "NASA astronomy picture of the day", "offline archive of NASA's astronomy pictures of the day (1995-2026)", "https://download.kiwix.org/zim/zimit/apod.nasa.gov_en_all_2026-05.zim"};
        master_db["medlineplus"]          = {"medlineplus", "medlineplus library", "archive of the NIH's medlineplus library, with tons of health information", "https://download.kiwix.org/zim/zimit/medlineplus.gov_en_all_2025-01.zim"};
    }

    void InitializeProfiles() {
        // preconfigured profiles setup here
        profiles.push_back({"general information: 16gb", "perfect for smaller (~16gb) drives includes basic information", {
            "tool_reader_linux", "tool_reader_win", "tool_reader_mac",
            "wiki_en_mini",
            "devdocs_c", "devdocs_cpp", "devdocs_cmake", "devdocs_git", "devdocs_bash", "devdocs_zsh",
            "devdocs_html", "devdocs_css", "devdocs_js",
            "gutenberg_medi",
            "medlineplus"
        }});
        profiles.push_back({"general information: 32gb", "perfect for average-sized drives (32gb)", {
            "tool_reader_linux", "tool_reader_win", "tool_reader_mac",
            "wiki_en_mini",
            "wiktionary",
            "ifixit",
            "freecodecamp",
            "devdocs_c", "devdocs_cpp", "devdocs_cmake", "devdocs_git", "devdocs_bash", "devdocs_zsh",
            "devdocs_docker", "devdocs_gcc", "devdocs_gnumake", "devdocs_linux", "devdocs_python",
            "devdocs_go", "devdocs_rust", "devdocs_sqlite", "devdocs_html", "devdocs_css", "devdocs_js",
            "stackexchange_cs", "stackexchange_ubuntu", "stackexchange_unix", "stackexchange_raspb",
            "gutenberg_medi"
        }});
        profiles.push_back({"general information: 64gb", "contains a medium amount of info, better for ~64gb", {
            "tool_reader_linux", "tool_reader_win", "tool_reader_mac",
            "wiki_en_nopic",
            "wiktionary",
            "ifixit",
            "freecodecamp",
            "gutenberg_medi",
            "libretexts_eng",
            "devdocs_c", "devdocs_cpp", "devdocs_cmake", "devdocs_git", "devdocs_bash", "devdocs_zsh",
            "devdocs_docker", "devdocs_gcc", "devdocs_gnumake", "devdocs_python", "devdocs_go",
            "devdocs_rust", "devdocs_sqlite", "devdocs_html", "devdocs_css", "devdocs_js", "devdocs_node",
            "stackexchange_cs",
        }});
        profiles.push_back({"general information: 128gb", "for drives more than 128gb. will have lots of info.", {
            "tool_reader_linux", "tool_reader_win", "tool_reader_mac",
            "wiki_en_nopic",
            "wiktionary",
            "ifixit",
            "crashcourse",
            "medlineplus",
            "nasa_apod",
            "minecraft",
            "libretexts_bio", "libretexts_biz", "libretexts_chem", "libretexts_eng",
            "libretexts_geo", "libretexts_human", "libretexts_math", "libretexts_med",
            "libretexts_phys", "libretexts_socialsci", "libretexts_stats",
            "stackexchange_elect", "stackexchange_math", "stackexchange_physi"
        }});
        profiles.push_back({"general information CHUNGUS", "contains probably all the information you will ever need", {"tool_reader_linux", "tool_reader_win", "tool_reader_mac", "wiki_en_all", "wiki_en_nopic", "wiki_en_mini", "stackoverflow_full", "stackexchange_3dp", "stackexchange_academ", "stackexchange_ai", "stackexchange_andr", "stackexchange_anime", "stackexchange_apple", "stackexchange_arduin", "stackexchange_ubuntu", "stackexchange_stars", "stackexchange_planes", "stackexchange_bio", "stackexchange_blend", "stackexchange_chem", "stackexchange_compgr", "stackexchange_cookin", "stackexchange_cs", "stackexchange_data", "stackexchange_earth", "stackexchange_econ", "stackexchange_elect", "stackexchange_fit", "stackexchange_gdev", "stackexchange_garden", "stackexchange_gdesgn", "stackexchange_law", "stackexchange_liter", "stackexchange_math", "stackexchange_mechan", "stackexchange_mytho", "stackexchange_parent", "stackexchange_physi", "stackexchange_psycho", "stackexchange_quant", "stackexchange_raspb", "stackexchange_robo", "stackexchange_sftwen", "stackexchange_space", "stackexchange_unix", "stackexchange_writin", "gutenberg", "gutenberg_medi", "gutenberg_sci", "gutenberg_tech", "ifixit", "devdocs_async", "devdocs_bash", "devdocs_bootstrap", "devdocs_c", "devdocs_cmake", "devdocs_cpp", "devdocs_css", "devdocs_docker", "devdocs_gcc", "devdocs_git", "devdocs_gnumake", "devdocs_go", "devdocs_godot", "devdocs_haxe", "devdocs_homebrew", "devdocs_html", "devdocs_http", "devdocs_js", "devdocs_jekyll", "devdocs_kotlin", "devdocs_lua", "devdocs_markdown", "devdocs_meteor", "devdocs_node", "devdocs_numpy", "devdocs_opengl", "devdocs_openjdk", "devdocs_php", "devdocs_pygame", "devdocs_python", "devdocs_pytorch", "devdocs_qt", "devdocs_redux", "devdocs_ruby", "devdocs_rust", "devdocs_sqlite", "devdocs_threejs", "devdocs_typescript", "devdocs_vulkan", "devdocs_wordpress", "devdocs_zsh", "freecodecamp", "libretexts_bio", "libretexts_biz", "libretexts_chem", "libretexts_eng", "libretexts_geo", "libretexts_human", "libretexts_math", "libretexts_med", "libretexts_phys", "libretexts_socialsci", "libretexts_stats", "khanacademy", "crashcourse", "minecraft", "wiktionary", "nasa_apod", "medlineplus"}});

        // focused presets
        profiles.push_back({"coding and software engineering", "for the coders", {
            "tool_reader_linux", "tool_reader_win", "tool_reader_mac",
            "stackoverflow_full",
            "freecodecamp",
            "devdocs_c", "devdocs_cpp", "devdocs_cmake", "devdocs_gcc", "devdocs_gnumake", "devdocs_asm",
            "devdocs_rust", "devdocs_go", "devdocs_python", "devdocs_node", "devdocs_kotlin", "devdocs_ruby",
            "devdocs_docker", "devdocs_git", "devdocs_bash", "devdocs_zsh", "devdocs_homebrew", "devdocs_jekyll",
            "devdocs_html", "devdocs_css", "devdocs_js", "devdocs_typescript", "devdocs_bootstrap", "devdocs_sqlite"
        }});
        profiles.push_back({"computer science & graphics", "computer nerd preset", {
            "tool_reader_linux", "tool_reader_win", "tool_reader_mac",
            "stackexchange_cs", "stackexchange_ai", "stackexchange_math", "stackexchange_quant",
            "libretexts_math", "libretexts_stats",
            "devdocs_opengl", "devdocs_vulkan", "devdocs_threejs", "devdocs_compgr",
            "devdocs_godot", "devdocs_pygame", "devdocs_lua", "devdocs_haxe", "devdocs_numpy", "devdocs_pytorch",
            "gutenberg_tech", "gutenberg_sci"
        }});
        profiles.push_back({"the scholar", "smartie pants profile", {
            "tool_reader_linux", "tool_reader_win", "tool_reader_mac",
            "wiki_en_nopic",
            "gutenberg",
            "crashcourse",
            "medlineplus",
            "nasa_apod",
            "libretexts_bio", "libretexts_biz", "libretexts_chem", "libretexts_eng",
            "libretexts_geo", "libretexts_human", "libretexts_math", "libretexts_med",
            "libretexts_phys", "libretexts_socialsci", "libretexts_stats",
            "stackexchange_academ", "stackexchange_liter", "stackexchange_mytho", "stackexchange_econ"
        }});
        profiles.push_back({"the stargazer", "ooh sparkly stars", {
            "tool_reader_linux", "tool_reader_win", "tool_reader_mac",
            "wiki_en_mini",
            "nasa_apod",
            "libretexts_phys", "libretexts_math", "libretexts_eng", "libretexts_geo",
            "gutenberg_sci", "gutenberg_tech",
            "stackexchange_space",
            "stackexchange_stars",
            "stackexchange_planes",
            "stackexchange_physi",
            "stackexchange_math",
            "devdocs_c", "devdocs_cpp", "devdocs_python", "devdocs_numpy", "devdocs_vulkan",
        }});
    }
};