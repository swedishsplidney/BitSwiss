#pragma once
#include <imgui.h>

enum ThemeType {
    THEME_DARK_WIN95 = 0,
    THEME_CLASSIC_WIN95,
    THEME_FALLOUT_GREEN,
    THEME_MODERN_DARK,
    THEME_COUNT
};

inline const char* GetThemeName(int theme) {
    switch (theme) {
        case THEME_DARK_WIN95:    return "Windows 95 (dark)";
        case THEME_CLASSIC_WIN95: return "Windows 95 (classic)";
        case THEME_FALLOUT_GREEN: return "terminal green";
        case THEME_MODERN_DARK:   return "modern dark";
        default:                  return "unknown";
    }
}

inline void ResetToWin95Geometry(ImGuiStyle& style) {
    style.WindowPadding      = ImVec2(8.0f, 8.0f);
    style.FramePadding       = ImVec2(6.0f, 4.0f);
    style.ItemSpacing        = ImVec2(8.0f, 6.0f);
    style.WindowBorderSize   = 2.0f;
    style.FrameBorderSize    = 1.0f;
    style.PopupBorderSize    = 2.0f;

    // no rounding
    style.WindowRounding    = 0.0f;
    style.FrameRounding     = 0.0f;
    style.PopupRounding     = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding      = 0.0f;
    style.TabRounding       = 0.0f;
}

inline void ApplyTheme(int theme_type) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    switch (theme_type) {
        case THEME_DARK_WIN95: {
            ResetToWin95Geometry(style);
            colors[ImGuiCol_Text]          = ImVec4(0.85f, 0.85f, 0.88f, 1.0f);
            colors[ImGuiCol_TextDisabled]  = ImVec4(0.40f, 0.40f, 0.45f, 1.0f);
            colors[ImGuiCol_WindowBg]      = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
            colors[ImGuiCol_ChildBg]       = ImVec4(0.09f, 0.09f, 0.10f, 1.0f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.25f, 0.45f, 1.0f);
            colors[ImGuiCol_Button]        = ImVec4(0.20f, 0.20f, 0.24f, 1.0f);
            colors[ImGuiCol_FrameBg]       = ImVec4(0.07f, 0.07f, 0.08f, 1.0f);
            colors[ImGuiCol_CheckMark]     = ImVec4(0.30f, 0.65f, 1.00f, 1.0f);
            colors[ImGuiCol_PlotHistogram] = ImVec4(0.20f, 0.55f, 0.90f, 1.0f);
            break;
        }
        case THEME_CLASSIC_WIN95: {
            ResetToWin95Geometry(style);
            colors[ImGuiCol_Text]           = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
            colors[ImGuiCol_TextDisabled]   = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
            colors[ImGuiCol_WindowBg]       = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
            colors[ImGuiCol_ChildBg]        = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
            colors[ImGuiCol_TitleBgActive]  = ImVec4(0.00f, 0.00f, 0.50f, 1.0f);
            colors[ImGuiCol_Button]         = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
            colors[ImGuiCol_FrameBg]        = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
            colors[ImGuiCol_CheckMark]      = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
            colors[ImGuiCol_PlotHistogram]  = ImVec4(0.00f, 0.00f, 0.50f, 1.0f);
            break;
        }
        case THEME_FALLOUT_GREEN: {
            ResetToWin95Geometry(style);
            colors[ImGuiCol_Text]           = ImVec4(0.00f, 1.00f, 0.20f, 1.0f);
            colors[ImGuiCol_TextDisabled]   = ImVec4(0.00f, 0.40f, 0.10f, 1.0f);
            colors[ImGuiCol_WindowBg]       = ImVec4(0.02f, 0.04f, 0.02f, 1.0f);
            colors[ImGuiCol_ChildBg]        = ImVec4(0.01f, 0.02f, 0.01f, 1.0f);
            colors[ImGuiCol_TitleBgActive]  = ImVec4(0.00f, 0.50f, 0.10f, 1.0f);
            colors[ImGuiCol_Button]         = ImVec4(0.05f, 0.15f, 0.05f, 1.0f);
            colors[ImGuiCol_FrameBg]        = ImVec4(0.00f, 0.05f, 0.00f, 1.0f);
            colors[ImGuiCol_CheckMark]      = ImVec4(0.00f, 1.00f, 0.20f, 1.0f);
            colors[ImGuiCol_PlotHistogram]  = ImVec4(0.00f, 0.80f, 0.15f, 1.0f);
            break;
        }
        case THEME_MODERN_DARK: {
            // revert to standard config
            style.WindowRounding    = 6.0f;
            style.FrameRounding     = 4.0f;
            style.WindowBorderSize  = 1.0f;
            ImGui::StyleColorsDark(); // default layout
            break;
        }
    }
}