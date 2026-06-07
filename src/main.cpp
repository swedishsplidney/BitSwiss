#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <GLFW/glfw3.h>

#include "DownloadManager.hpp"
#include "StorageManager.hpp"
#include "PackageManager.hpp"
#include "ThemeManager.hpp"

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    // decide gl/glsl versions
    // gl 3.3 core is OS universal
#if defined(__APPLE__)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    // create window w/ graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "BitSwiss launcher", nullptr, nullptr);
    if (window == nullptr) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    // setup imgui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // kb controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // docking

    // setup imgui style (dark mode, obv)
    ImGui::StyleColorsDark();

    // setup platform/renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // our state
    ImVec4 clear_color = ImVec4(0.15f, 0.16f, 0.21f, 1.00f); // bg

    // main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // start the imgui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // custom app window
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

            // center and size window
            ImVec2 target_size = ImVec2(950.0f, 650.0f);
            ImVec2 center_pos = ImVec2((1280.0f - target_size.x) * 0.5f, (720.0f - target_size.y ) * 0.5f);

            ImGui::SetNextWindowPos(center_pos, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(target_size, ImGuiCond_FirstUseEver);

            ImGui::Begin("BitSwiss generator control panel");

            ImGui::PopStyleColor();

            static int current_theme_idx = THEME_DARK_WIN95;
            static bool initialization_frame = true;
            static PackageManager pm;
            static DownloadManager dm;
            static int selected_drive_idx = 0;
            static bool sizes_synced = false;

            // apply defaults on start
            if (initialization_frame) {
                ApplyTheme(current_theme_idx);
                initialization_frame = false;
            }

            // settings panel
            if (ImGui::CollapsingHeader("settings")) {
                ImGui::Indent();
                ImGui::Spacing();

                // theme toggle dropdown
                const char* preview_name = GetThemeName(current_theme_idx);
                if (ImGui::BeginCombo("theme", preview_name)) {
                    for (int i = 0; i < THEME_COUNT; i++) {
                        bool is_selected = (current_theme_idx == i);
                        if (ImGui::Selectable(GetThemeName(i), is_selected)) {
                            current_theme_idx = i;
                            ApplyTheme(current_theme_idx); // instantly apply theme
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::TextDisabled("changes the style and theme");
                ImGui::Spacing();
                ImGui::Unindent();
            }

            ImGui::Separator();

            // trigger the step 2 network background threads only one time
            if (!sizes_synced) {
                pm.UpdateManifestSizesAsync();
                sizes_synced = true;
            }

            // fetch live hardware device list
            std::vector<DriveInfo> drives = StorageManager::GetTargetDrives();

            // section 1: storage vector target
            ImGui::Text("1. target destination vector");
            static std::vector<std::string> drive_previews;
            for (const auto& d : drives) {
                double gb = (double)d.total_bytes / (1024.0 * 1024.0 * 1024.0);
                drive_previews.push_back(d.device_path + " (" + d.model_name + ") [" + std::to_string((int)gb) + " GB]");
            }

            if (drives.empty()) {
                ImGui::BeginDisabled();
                ImGui::Combo("target disk", &selected_drive_idx, "searching for removable drives...");
                ImGui::EndDisabled();
            } else {
                if (selected_drive_idx >= (int)drives.size()) selected_drive_idx = 0;
                std::vector<const char*> combo_items;
                for  (const auto& s : drive_previews) combo_items.push_back(s.c_str());
                ImGui::Combo("target disk", &selected_drive_idx, combo_items.data(), (int)combo_items.size());
            }

            ImGui::Separator();

            // section 2: preconfigured profiles
            if (ImGui::CollapsingHeader("2. preconfigured profiles")) {
                ImGui::Spacing();

                // renders custom layout select blocks
                for (const auto& profile : pm.GetProfiles()) {
                    if (ImGui::Button(profile.name.c_str(), ImVec2(240.0f, 0.0f))) {
                        // drop current select array modifications
                        for (auto& [id, pkg] : pm.GetMasterDB()) pkg.is_selected = false;

                        // map the configuration profile items back to the master definitions
                        for (const auto& id : profile.package_ids) {
                            if (pm.GetMasterDB().find(id) != pm.GetMasterDB().end()) {
                                pm.GetMasterDB()[id].is_selected = true;
                            }
                        }
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("- %s", profile.description.c_str());
                }
            }

            ImGui::Separator();

            // section 3: master data list
            ImGui::Text("3. master data list");
            ImGui::Spacing();

            // creates a smooth scrolling panel
            if (ImGui::BeginChild("MasterScrollingList", ImVec2(-1.0f, 300.0f), true, ImGuiWindowFlags_HorizontalScrollbar)) {
                for (auto& [id, pkg] : pm.GetMasterDB()) {
                    double pkg_gb = (double)pkg.size_in_bytes / (1024.0 * 1024.0 * 1024.0);

                    char label_buf[256];
                    snprintf(label_buf, sizeof(label_buf), "%s (%.2f GB)##%s", pkg.name.c_str(), pkg_gb, pkg.id.c_str());

                    ImGui::Checkbox(label_buf, &pkg.is_selected);
                    ImGui::SameLine();
                    ImGui::TextDisabled("| %s", pkg.description.c_str());

                    // dl progress bar
                    if (pkg.is_downloading->load()) {
                        double current_progress = pkg.download_progress->load();

                        // format text inside loading bar
                        char progress_text[32];
                        snprintf(progress_text, sizeof(progress_text), "downloading... %.1f%%", current_progress);

                        // render the progress bar
                        ImGui::ProgressBar(static_cast<float>(current_progress / 100.0), ImVec2(-1.0f, 0.0f), progress_text);
                    }
                    else if (pkg.is_completed->load()) {
                        // renders a solid green "finished" indicator
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 0.8f, 0.3f, 1.0f));
                        ImGui::ProgressBar(1.0f, ImVec2(-1.0f, 0.0f), "completed!");
                        ImGui::PopStyleColor();
                    }
                    ImGui::Separator();
                }
                ImGui::EndChild();
            }

            ImGui::Separator();

            // tools category
            ImGui::Text("4. include portable readers");
            ImGui::Spacing();

            static bool include_win_reader = false;
            static bool include_linux_reader = false;
            static bool include_mac_reader = false;

            ImGui::Checkbox("windows reader (.exe ZIP)", &include_win_reader); ImGui::SameLine(320.0f);
            ImGui::Checkbox("linux reader (.AppImage)", &include_linux_reader); ImGui::SameLine(640.0f);
            ImGui::Checkbox("mac reader (.dmg)", &include_mac_reader);

            ImGui::Spacing();
            ImGui::Separator();

            // section 4: real time capacity

            // pre inject tools so the capacity bar calculates them
            pm.GetMasterDB()["tool_reader_win"].is_selected = include_win_reader;
            pm.GetMasterDB()["tool_reader_linux"].is_selected = include_linux_reader;
            pm.GetMasterDB()["tool_reader_mac"].is_selected = include_mac_reader;

            uint64_t total_selected_bytes = 0;
            for (const auto& [id, pkg] : pm.GetMasterDB()) {
                if (pkg.is_selected) total_selected_bytes += pkg.size_in_bytes;
            }

            uint64_t drive_capacity_bytes = drives.empty() ? 0 : drives[selected_drive_idx].total_bytes;
            float allocation_fraction = drive_capacity_bytes == 0 ? 0.0f : (float)total_selected_bytes / (float)drive_capacity_bytes;

            char progress_buf[128];
            snprintf(progress_buf, sizeof(progress_buf), "%.2f GB / %.2f GB allocated",
                (double)total_selected_bytes / (1024.0 * 1024.0 * 1024.0),
                (double)drive_capacity_bytes / (1024.0 * 1024.0 * 1024.0));

            bool is_overloaded = total_selected_bytes > drive_capacity_bytes && drive_capacity_bytes > 0;
            ImVec4 final_bar_color;

            if (is_overloaded) {
                final_bar_color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
            } else {
                final_bar_color = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
            }

            // determine contrast text color only when bar underneath
            ImVec4 text_over_bar_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // white

            if (is_overloaded) {
                text_over_bar_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                // when overloaded red, white is good for visibility
            }
            else if (current_theme_idx == THEME_DARK_WIN95) {
                text_over_bar_color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                // yellow
            }
            else if (current_theme_idx == THEME_CLASSIC_WIN95) {
                text_over_bar_color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                // yellow
            }
            else if (current_theme_idx == THEME_FALLOUT_GREEN) {
                text_over_bar_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                // black
            }
            else if (current_theme_idx == THEME_FALLOUT_PURPLE) {
                text_over_bar_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                // black
            }

            // get layout coordinates and sizes
            ImVec2 bar_pos = ImGui::GetCursorScreenPos();
            ImVec2 bar_size = ImVec2(ImGui::GetContentRegionAvail().x, 22.0f);

            // render progress bar empty
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, final_bar_color);
            ImGui::ProgressBar(allocation_fraction, bar_size, "");
            ImGui::PopStyleColor(1);

            // calculate pixel split boundary
            float split_x = bar_pos.x + (bar_size.x * allocation_fraction);

            // dynamic left alignment math
            float padding_x = ImGui::GetStyle().ItemSpacing.x;
            float text_left_default = bar_pos.x + padding_x;

            ImVec2 text_size = ImGui::CalcTextSize(progress_buf);
            float max_text_x = (bar_pos.x + bar_size.x) - text_size.x - padding_x;

            float cushion = 6.0f;

            float text_x = text_left_default;
            if (split_x > text_left_default - cushion) {
                text_x = split_x + cushion;
                if (text_x > max_text_x) {
                    text_x = max_text_x; // lock to max
                }
            }

            float text_y = bar_pos.y + (bar_size.y - text_size.y) / 2.0f; // vertical center
            ImVec2 text_pos = ImVec2(text_x, text_y);

            // render on the filled side
            ImGui::PushClipRect(bar_pos, ImVec2(split_x, bar_pos.y + bar_size.y), true);
            ImGui::PushStyleColor(ImGuiCol_Text, text_over_bar_color);
            ImGui::SetCursorScreenPos(text_pos);
            ImGui::TextUnformatted(progress_buf);
            ImGui::PopStyleColor(1);
            ImGui::PopClipRect();

            // render on the un-filled side
            ImGui::PushClipRect(ImVec2(split_x, bar_pos.y), ImVec2(bar_pos.x + bar_size.x, bar_pos.y + bar_size.y), true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]); // normal theme color
            ImGui::SetCursorScreenPos(text_pos);
            ImGui::TextUnformatted(progress_buf);
            ImGui::PopStyleColor(1);
            ImGui::PopClipRect();

            // reset cursor layout padding
            ImGui::SetCursorScreenPos(ImVec2(bar_pos.x, bar_pos.y + bar_size.y + ImGui::GetStyle().ItemSpacing.y));

            // section 5: write to drive
            uint64_t selected_count = 0;
            uint64_t completed_count = 0;

            for (const auto& [id, pkg] : pm.GetMasterDB()) {
                if (pkg.is_selected) {
                    selected_count++;
                    if (pkg.is_completed->load()) {
                        completed_count++;
                    }
                }
            }

            // determine if completed or not
            bool batch_completed = false;
            static bool cancellation_requested = false;
            static uint64_t last_selected_count = 0;

            // reset state if selection changes
            if (selected_count != last_selected_count) {
                batch_completed = false;
                cancellation_requested = false;

                // clear other completion flags
                for (auto& [id, pkg] : pm.GetMasterDB()) {
                    pkg.is_completed->store(false);
                    pkg.download_progress->store(0.0);
                }

                last_selected_count = selected_count;
            }

            if (selected_count > 0 && completed_count == selected_count && dm.IsRunning()) {
                dm.CancelPool();
            }

            if (!dm.IsRunning()) {
                cancellation_requested = false;
            }

            // lock completed state when complete
            if (selected_count > 0 && completed_count == selected_count && !dm.IsRunning()) {
                // only mark true if something actually was downloaded
                if (total_selected_bytes > 0) {
                    batch_completed = true;
                }
            }

            if (batch_completed) {
                // render a non-clickable button
                ImGui::BeginDisabled();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.8f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

                ImGui::Button("completed!", ImVec2(-1.0f, 40.0f));

                ImGui::PopStyleColor(2);
                ImGui::EndDisabled();
            }
            else if (dm.IsRunning() && !cancellation_requested) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.10f, 0.15f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.20f, 0.25f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));

                if (ImGui::Button("cancel all downloads", ImVec2(-1.0f, 40.0f))) {
                    std::cout << "canceled: stopping all downloads..." << std::endl;
                    dm.CancelPool();
                    batch_completed = false;
                    cancellation_requested = true;
                }
                ImGui::PopStyleColor(3);
            }
            else {
                bool is_syncing = !pm.IsSizingFinished();
                bool disable_btn = drives.empty() || is_overloaded || total_selected_bytes == 0;

                if (disable_btn) ImGui::BeginDisabled();

                // dynamic button status indicator
                std::string button_text = is_syncing ? "syncing file extensions..." : "write to drive";

                if (ImGui::Button(button_text.c_str(), ImVec2(-1.0f, 40.0f))) {
                    cancellation_requested = false;
                    batch_completed = false;

                    std::vector<Package*> packages_to_deploy;

                    // add true filenames from URL
                    for (auto& [id, pkg] : pm.GetMasterDB()) {
                        if (pkg.is_selected) packages_to_deploy.push_back(&pkg);
                    }
                    std::string active_target_mount = drives[selected_drive_idx].device_path;
                    std::cout << "launching concurrent download pool to " << active_target_mount << std::endl;
                    dm.StartDownloadPool(packages_to_deploy, active_target_mount);
                }
                if (disable_btn) ImGui::EndDisabled();
            }

            ImGui::End();
        }

        // rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}