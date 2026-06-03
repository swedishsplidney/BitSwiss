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

            // persistent static manifest and state parameters
            static std::vector<Category> categories = PackageManager::GetDefaultManifest();
            static int selected_drive_idx = 0;

            // scan hardware devices
            std::vector<DriveInfo> drives = StorageManager::GetTargetDrives();

            // tracking drop-down/selection
            ImGui::Text("1. destination vector");
            std::vector<std::string> drive_previews;
            for (const auto& d : drives) {
                double gb = (double)d.total_bytes / (1024.0 * 1024.0 * 1024.0);
                drive_previews.push_back(d.device_path + " (" + d.model_name + ") [" + std::to_string((int)gb) + " GB]");
            }

            if (drives.empty()) {
                ImGui::BeginDisabled();
                std::string placeholder = "no compatible devices detected :(";
                ImGui::Combo("target disk", &selected_drive_idx, placeholder.c_str());
                ImGui::EndDisabled();
            } else {
                if (selected_drive_idx >= (int)drives.size()) selected_drive_idx = 0;

                // custom combo builder array processing loop
                std::vector<const char*> combo_items;
                for (const auto& s : drive_previews) combo_items.push_back(s.c_str());
                ImGui::Combo("target disk", &selected_drive_idx, combo_items.data(), (int)combo_items.size());
            }

            ImGui::Separator();

            // accumulative data size math
            uint64_t total_selected_bytes = 0;
            for (const auto& cat : categories) {
                for (const auto& pkg : cat.packages) {
                    if (pkg.is_selected) total_selected_bytes += pkg.size_in_bytes;
                }
            }

            uint64_t drive_capacity_bytes = drives.empty() ? 0 : drives[selected_drive_idx].total_bytes;
            float allocation_fraction = drive_capacity_bytes == 0 ? 0.0f : (float)total_selected_bytes / (float)drive_capacity_bytes;

            ImGui::Text("2. target allocation graph");

            // flash red if chosen data exceeds available storage
            bool is_overloaded = total_selected_bytes > drive_capacity_bytes && drive_capacity_bytes > 0;
            if (is_overloaded) {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 0.9f, 1.0f)); // cyan blue
            }

            char progress_buf[64];
            snprintf(progress_buf, sizeof(progress_buf), "%.1f MB / %.1f GB Allocated",
                (double)total_selected_bytes / (1024.0 * 1024.0),
                (double)drive_capacity_bytes / (1024.0 * 1024.0 * 1024.0));

            ImGui::ProgressBar(allocation_fraction, ImVec2(-1.0f, 0.0f), progress_buf);
            ImGui::PopStyleColor();

            ImGui::Separator();

            // render dynamic category blocks
            ImGui::Text("3. modular payloads");
            for (auto& cat : categories) {
                if (ImGui::CollapsingHeader(cat.name.c_str())) {
                    ImGui::Indent();
                    for (auto& pkg : cat.packages) {
                        double pkg_mb = (double)pkg.size_in_bytes / (1024.0 * 1024.0);

                        char label_buf[128];
                        snprintf(label_buf, sizeof(label_buf), "%s (%.1f MB)##%s",
                            pkg.name.c_str(),
                            pkg_mb,
                            pkg.name.c_str());

                        ImGui::Checkbox(label_buf, &pkg.is_selected);

                        ImGui::SameLine();
                        ImGui::TextDisabled("- %s", pkg.description.c_str());
                    }
                    ImGui::Unindent();
                }
            }

            ImGui::Separator();

            // safe provision activation lockouts
            bool disable_compile_btn = drives.empty() || is_overloaded || total_selected_bytes == 0;
            if (disable_compile_btn) ImGui::BeginDisabled();

            if (ImGui::Button("forge station payload", ImVec2(-1.0f, 40.0f))) {
                // this will be phase 3 target module thread
            }

            if (disable_compile_btn) ImGui::EndDisabled();

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