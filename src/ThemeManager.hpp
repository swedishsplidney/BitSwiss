#pragma once
#include <imgui.h>
#include <imgui_internal.h>

enum ThemeType {
    THEME_DARK_WIN95 = 0,
    THEME_CLASSIC_WIN95,
    THEME_FALLOUT_GREEN,
    THEME_FALLOUT_PURPLE,
    THEME_MODERN_DARK,
    THEME_COUNT
};

inline const char* GetThemeName(int theme) {
    switch (theme) {
        case THEME_DARK_WIN95:     return "Windows 95 (dark)";
        case THEME_CLASSIC_WIN95:  return "Windows 95 (classic)";
        case THEME_FALLOUT_GREEN:  return "terminal green";
        case THEME_FALLOUT_PURPLE: return "terminal purple";
        case THEME_MODERN_DARK:    return "modern dark";
        default:                   return "unknown";
    }
}

inline void ResetToWin95Geometry(ImGuiStyle& style) {
    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.FramePadding      = ImVec2(6.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);

    // bevels
    style.WindowBorderSize  = 2.0f;
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 2.0f;

    // no rounded corners
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
            colors[ImGuiCol_Text]                 = ImVec4(0.85f, 0.85f, 0.88f, 1.0f);
            colors[ImGuiCol_TextDisabled]         = ImVec4(0.40f, 0.40f, 0.45f, 1.0f);
            colors[ImGuiCol_WindowBg]             = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
            colors[ImGuiCol_ChildBg]              = ImVec4(0.09f, 0.09f, 0.10f, 1.0f);
            colors[ImGuiCol_TitleBgActive]        = ImVec4(0.00f, 0.00f, 0.50f, 1.0f);
            colors[ImGuiCol_Button]               = ImVec4(0.20f, 0.20f, 0.24f, 1.0f);
            colors[ImGuiCol_ButtonHovered]        = ImVec4(0.24f, 0.24f, 0.28f, 1.0f);
            colors[ImGuiCol_ButtonActive]         = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
            colors[ImGuiCol_PopupBg]              = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
            colors[ImGuiCol_FrameBg]              = ImVec4(0.07f, 0.07f, 0.08f, 1.0f);
            colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
            colors[ImGuiCol_FrameBgActive]        = ImVec4(0.05f, 0.05f, 0.06f, 1.0f);
            colors[ImGuiCol_Header]               = ImVec4(0.00f, 0.00f, 0.50f, 1.0f);
            colors[ImGuiCol_HeaderHovered]        = ImVec4(0.00f, 0.00f, 0.65f, 1.0f);
            colors[ImGuiCol_HeaderActive]         = ImVec4(0.00f, 0.00f, 0.40f, 1.0f);
            colors[ImGuiCol_CheckMark]            = ImVec4(0.30f, 0.65f, 1.00f, 1.0f);
            colors[ImGuiCol_PlotHistogram]        = ImVec4(0.00f, 0.00f, 0.50f, 1.0f);
            colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 0.00f, 0.65f, 1.0f);
            colors[ImGuiCol_Border]               = ImVec4(0.35f, 0.35f, 0.40f, 1.0f);
            colors[ImGuiCol_BorderShadow]         = ImVec4(0.05f, 0.05f, 0.06f, 1.0f);
            break;
        }
        case THEME_CLASSIC_WIN95: {
            ResetToWin95Geometry(style);
            colors[ImGuiCol_Text]                 = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
            colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
            colors[ImGuiCol_WindowBg]             = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
            colors[ImGuiCol_ChildBg]              = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
            colors[ImGuiCol_TitleBgActive]        = ImVec4(0.00f, 0.00f, 0.50f, 1.0f);
            colors[ImGuiCol_TitleBg]              = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
            colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
            colors[ImGuiCol_Button]               = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
            colors[ImGuiCol_ButtonHovered]        = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
            colors[ImGuiCol_ButtonActive]         = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
            colors[ImGuiCol_PopupBg]              = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
            colors[ImGuiCol_FrameBg]              = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
            colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
            colors[ImGuiCol_FrameBgActive]        = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
            colors[ImGuiCol_CheckMark]            = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
            colors[ImGuiCol_PlotHistogram]        = ImVec4(0.00f, 0.00f, 0.50f, 1.0f);
            colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 0.00f, 0.65f, 1.0f);
            colors[ImGuiCol_Border]               = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
            colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
            colors[ImGuiCol_Header]               = ImVec4(0.00f, 0.00f, 0.50f, 1.0f);
            colors[ImGuiCol_HeaderHovered]        = ImVec4(0.00f, 0.00f, 0.50f, 1.0f);
            colors[ImGuiCol_HeaderActive]         = ImVec4(0.00f, 0.00f, 0.30f, 1.0f);
            break;
        }
        case THEME_FALLOUT_GREEN: {
            ResetToWin95Geometry(style);
            colors[ImGuiCol_Text]                 = ImVec4(0.00f, 1.00f, 0.20f, 1.0f);
            colors[ImGuiCol_TextDisabled]         = ImVec4(0.00f, 0.40f, 0.10f, 1.0f);
            colors[ImGuiCol_WindowBg]             = ImVec4(0.02f, 0.04f, 0.02f, 1.0f);
            colors[ImGuiCol_ChildBg]              = ImVec4(0.01f, 0.02f, 0.01f, 1.0f);
            colors[ImGuiCol_TitleBgActive]        = ImVec4(0.00f, 0.50f, 0.10f, 1.0f);
            colors[ImGuiCol_Button]               = ImVec4(0.05f, 0.15f, 0.05f, 1.0f);
            colors[ImGuiCol_ButtonHovered]        = ImVec4(0.08f, 0.25f, 0.08f, 1.0f);
            colors[ImGuiCol_ButtonActive]         = ImVec4(0.03f, 0.10f, 0.03f, 1.0f);
            colors[ImGuiCol_PopupBg]              = ImVec4(0.02f, 0.04f, 0.02f, 1.0f);
            colors[ImGuiCol_FrameBg]              = ImVec4(0.00f, 0.05f, 0.00f, 1.0f);
            colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.01f, 0.08f, 0.01f, 1.0f);
            colors[ImGuiCol_FrameBgActive]        = ImVec4(0.00f, 0.03f, 0.00f, 1.0f);
            colors[ImGuiCol_Header]               = ImVec4(0.00f, 0.40f, 0.10f, 1.0f);
            colors[ImGuiCol_HeaderHovered]        = ImVec4(0.00f, 0.60f, 0.15f, 1.0f);
            colors[ImGuiCol_HeaderActive]         = ImVec4(0.00f, 0.30f, 0.05f, 1.0f);
            colors[ImGuiCol_CheckMark]            = ImVec4(0.00f, 1.00f, 0.20f, 1.0f);
            colors[ImGuiCol_PlotHistogram]        = ImVec4(0.00f, 0.80f, 0.15f, 1.0f);
            colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 1.00f, 0.20f, 1.0f);
            colors[ImGuiCol_Border]               = ImVec4(0.00f, 0.50f, 0.10f, 1.0f);
            colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.10f, 0.02f, 1.0f);
            break;
        }
        case THEME_FALLOUT_PURPLE: {
            ResetToWin95Geometry(style);
            colors[ImGuiCol_Text]                 = ImVec4(0.49f, 0.00f, 1.00f, 1.0f);
            colors[ImGuiCol_TextDisabled]         = ImVec4(0.25f, 0.00f, 0.50f, 1.0f);
            colors[ImGuiCol_WindowBg]             = ImVec4(0.03f, 0.00f, 0.05f, 1.0f);
            colors[ImGuiCol_ChildBg]              = ImVec4(0.01f, 0.00f, 0.02f, 1.0f);
            colors[ImGuiCol_TitleBgActive]        = ImVec4(0.35f, 0.00f, 0.70f, 1.0f);
            colors[ImGuiCol_Button]               = ImVec4(0.12f, 0.00f, 0.25f, 1.0f);
            colors[ImGuiCol_ButtonHovered]        = ImVec4(0.20f, 0.00f, 0.40f, 1.0f);
            colors[ImGuiCol_ButtonActive]         = ImVec4(0.08f, 0.00f, 0.15f, 1.0f);
            colors[ImGuiCol_PopupBg]              = ImVec4(0.03f, 0.00f, 0.05f, 1.0f);
            colors[ImGuiCol_FrameBg]              = ImVec4(0.05f, 0.00f, 0.10f, 1.0f);
            colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.08f, 0.00f, 0.15f, 1.0f);
            colors[ImGuiCol_FrameBgActive]        = ImVec4(0.03f, 0.00f, 0.06f, 1.0f);
            colors[ImGuiCol_Header]               = ImVec4(0.25f, 0.00f, 0.50f, 1.0f);
            colors[ImGuiCol_HeaderHovered]        = ImVec4(0.40f, 0.00f, 0.80f, 1.0f);
            colors[ImGuiCol_HeaderActive]         = ImVec4(0.15f, 0.00f, 0.30f, 1.0f);
            colors[ImGuiCol_CheckMark]            = ImVec4(0.49f, 0.00f, 1.00f, 1.0f);
            colors[ImGuiCol_PlotHistogram]        = ImVec4(0.49f, 0.00f, 1.00f, 1.0f);
            colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.60f, 0.10f, 1.00f, 1.0f);
            colors[ImGuiCol_Border]               = ImVec4(0.35f, 0.00f, 0.70f, 1.0f);
            colors[ImGuiCol_BorderShadow]         = ImVec4(0.08f, 0.00f, 0.15f, 1.0f);
            break;
        }
        case THEME_MODERN_DARK: {
            style.WindowRounding    = 6.0f;
            style.FrameRounding     = 4.0f;
            style.WindowBorderSize  = 1.0f;
            style.FrameBorderSize   = 1.0f;
            style.PopupRounding     = 4.0f;
            ImGui::StyleColorsDark(); // reapply defaults

            style.Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.40f, 0.67f, 1.00f, 1.0f);
            break;
        }
    }
}

// retro style custom workers
inline bool Win95Button(const char* label, const ImVec2& size = ImVec2(0, 0)) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 actual_size = ImGui::CalcItemSize(size, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    ImRect bb(pos, ImVec2(pos.x + actual_size.x, pos.y + actual_size.y));
    ImGui::ItemSize(actual_size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    bool is_sunken = held && hovered;
    ImU32 bg_col = ImGui::GetColorU32(is_sunken ? ImGuiCol_ButtonActive : (hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button));
    ImU32 light_edge = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImU32 dark_shadow = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImU32 mid_shadow = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

    window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col);

    if (is_sunken) {
        window->DrawList->AddRect(bb.Min, bb.Max, dark_shadow, 0.0f, 0, 1.0f);
    } else {
        window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x - 1, bb.Min.y), light_edge);
        window->DrawList->AddLine(bb.Min, ImVec2(bb.Min.x, bb.Max.y - 1), light_edge);
        window->DrawList->AddLine(ImVec2(bb.Min.x, bb.Max.y - 1), ImVec2(bb.Max.x, bb.Max.y - 1), dark_shadow);
        window->DrawList->AddLine(ImVec2(bb.Max.x - 1, bb.Min.y), ImVec2(bb.Max.x - 1, bb.Max.y), dark_shadow);
        window->DrawList->AddLine(ImVec2(bb.Min.x + 1, bb.Max.y - 2), ImVec2(bb.Max.x - 1, bb.Max.y - 2), mid_shadow);
        window->DrawList->AddLine(ImVec2(bb.Max.x - 2, bb.Min.y + 1), ImVec2(bb.Max.x - 2, bb.Max.y - 1), mid_shadow);
    }

    ImVec2 text_pos = ImVec2(bb.Min.x + style.FramePadding.x, bb.Min.y + style.FramePadding.y);
    if (is_sunken) { text_pos.x += 1.0f; text_pos.y += 1.0f; }
    ImGui::RenderText(text_pos, label);

    return pressed;
}