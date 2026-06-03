#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <GLFW/glfw3.h>
#include "StorageManager.hpp"
#include "PackageManager.hpp"

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
    bool show_demo_window = true;
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
            ImGui::Begin("BitSwiss generator control panel");

            static PackageManager pm;
            static int selected_drive_idx = 0;
            static bool sizes_synced = false;

            // trigger the step 2 network background threads only one time
            if (!sizes_synced) {
                pm.UpdateManifestSizesAsync();
                sizes_synced = true;
            }

            // fetch live hardware device list
            std::vector<DriveInfo> drives = StorageManager::GetTargetDrives();

            // section 1: storage vector target
            ImGui::Text("1. target destination vector");
            std::vector<std::string> drive_previews;
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
            ImGui::Text("2. preconfigured profiles");
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
                }
                ImGui::EndChild();
            }

            ImGui::Separator();

            // section 4: real time capacity
            uint64_t total_selected_bytes = 0;
            for (const auto& [id, pkg] : pm.GetMasterDB()) {
                if (pkg.is_selected) total_selected_bytes += pkg.size_in_bytes;
            }

            uint64_t drive_capacity_bytes = drives.empty() ? 0 : drives[selected_drive_idx].total_bytes;
            float allocation_fraction = drive_capacity_bytes == 0 ? 0.0f : (float)total_selected_bytes / (float)drive_capacity_bytes;

            bool is_overloaded =  total_selected_bytes > drive_capacity_bytes && drive_capacity_bytes > 0;
            if (is_overloaded) {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.2f, 0.2f, 1.0f)); // red
            } else {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 0.9f, 1.0f)); // blue
            }

            char progress_buf[128];
            snprintf(progress_buf, sizeof(progress_buf), "%.2f GB / %.2f GB allocated",
                (double)total_selected_bytes / (1024.0 * 1024.0 * 1024.0),
                (double)drive_capacity_bytes / (1024.0 * 1024.0 * 1024.0));

            ImGui::ProgressBar(allocation_fraction, ImVec2(-1.0f, 0.0f), progress_buf);
            ImGui::PopStyleColor();

            // section 5: write to drive
            bool disable_btn = drives.empty() || is_overloaded || total_selected_bytes == 0;
            if (disable_btn) ImGui::BeginDisabled();

            if (ImGui::Button("write to drive", ImVec2(-1.0f, 40.0f))) {
                // writing implementation goes here
            }

            if (disable_btn) ImGui::EndDisabled();

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