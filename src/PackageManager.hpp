#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <iostream>
#include <future>
#include <chrono>

// core data structure
struct Package {
    std::string id;             // unique lookup key
    std::string name;           // ui title
    std::string description;    // short description
    std::string download_url;   // location of download
    uint64_t size_in_bytes;     // real allocation metrics
    bool is_selected = false;   //globally tracked state
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

public:
    PackageManager() {
        InitializeMasterDatabase();
        InitializeProfiles();
    }

    std::map<std::string, Package>& GetMasterDB() { return master_db; }
    std::vector<PreconfigProfile>& GetProfiles() { return profiles; }

    // live fetching engine
    // simulates and http head request payload check
    static uint64_t FetchRemoteSize(const std::string& url, const std::string& id) {
        // fallback hardcoded estimate sizes
        if (id.find("wiki") != std::string::npos) {
            if (id.find("large") != std::string::npos || id.find("maxi") != std::string::npos) return 115ULL * 1024 * 1024 * 1024;
            if (id.find("medium") != std::string::npos || id.find("nopic") != std::string::npos) return 48ULL * 1024 * 1024 * 1024;
            return 12ULL * 1024 * 1024 * 1024; // mini
        }
        if (id.find("stackoverflow") != std::string::npos) return 74ULL * 1024 * 1024 * 1024;
        if (id.find("gutenberg") != std::string::npos) return 72ULL * 1024 * 1024 * 1024;
        if (id.find("ifixit") != std::string::npos) return 4ULL * 1024 * 1024 * 1024;
        if (id.find("stackexchange") != std::string::npos) return 7ULL * 1024 * 1024 * 1024;

        return 1ULL * 1024 * 1024 * 1024; // global fallback 1gb
    }

    // spawns asynchronous worker threads to ping sizes in parallel
    // prevents main ImGui loop from freezing at startup
    void UpdateManifestSizesAsync() {
        std::vector<std::future<void>> futures;

        for (auto it = master_db.begin(); it != master_db.end(); ++it) {
            std::string current_id = it->first;
            Package* pkg_ptr = &(it->second);

            futures.push_back(std::async(std::launch::async, [pkg_ptr, current_id]() mutable {
                pkg_ptr->size_in_bytes = FetchRemoteSize(pkg_ptr->download_url, current_id);
            }));
        }

        // wait for all threads to finish
        for (auto& f : futures) {
            f.wait();
        }
    }

private:
    void InitializeMasterDatabase() {
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
        master_db["gutenberg_tech"]       = {"gutenberg_tech", "gutenberg library technology collection", "large colletion of public domain technology ebooks", "https://download.kiwix.org/zim/gutenberg/gutenberg_en_lcc-t_2026-03.zim"};
        master_db["ifixit"]               = {"ifixit", "ifixit information archive (full", "full ifixit archive with guides to fix almost any devices", "https://download.kiwix.org/zim/ifixit/ifixit_en_all_2025-12.zim"};
    }

    void InitializeProfiles() {
        // preconfigured profiles setup here
        profiles.push_back({"general information: small", "perfect for smaller (~16gb) drives includes basic information", {"wiki_en_mini", "gutenberg_medi"}});
        profiles.push_back({"general information: medium", "contains more information than small, better for ~64gb drives", {"wiki_en_nopic", "gutenberg-medi", "ifixit"}});
        profiles.push_back({"general information: large", "for drives more than 128gb. will have lots of info.", {"wiki_en_nopic", "gutenberg_medi", "gutenberg_sci", "gutenberg_tech", "ifixit", "stackexchange_academ", "stackexchange_stars", "stackexchange_planes", "stackexchange_bio", "stackexchange_chem", "stackexchange_cs", "stackexchange_elect", "stackexchange_law", "stackexchange_math", "stackexchange_liter"}});
    }
};