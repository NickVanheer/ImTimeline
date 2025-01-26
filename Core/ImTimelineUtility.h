#pragma once
#include "../dependencies/imgui/imgui.h"
#include "CoreDefines.h"

class ImTimelineUtility {
public:

struct Color {
    static const ImU32 Black = IM_COL32(0, 0, 0, 255);
    static const ImU32 White = IM_COL32(255, 255, 255, 255);
    static const ImU32 Red = IM_COL32(255, 0, 0, 255);
    static const ImU32 Green = IM_COL32(0, 255, 0, 255);
    static const ImU32 Blue = IM_COL32(0, 0, 255, 255);
    static const ImU32 Yellow = IM_COL32(255, 255, 0, 255);
    static const ImU32 Cyan = IM_COL32(0, 255, 255, 255);
    static const ImU32 Magenta = IM_COL32(255, 0, 255, 255);
    static const ImU32 Gray = IM_COL32(128, 128, 128, 255);
    static const ImU32 DarkGray = IM_COL32(64, 64, 64, 255);
    static const ImU32 Orange = IM_COL32(255, 165, 0, 255);
    static const ImU32 Purple = IM_COL32(128, 0, 128, 255);
    static const ImU32 Pink = IM_COL32(255, 192, 203, 255);
    static const ImU32 Brown = IM_COL32(165, 42, 42, 255);
    static const ImU32 Lime = IM_COL32(50, 205, 50, 255);

    static const ImU32 LightRed = IM_COL32(140, 90, 110, 255);
    static const ImU32 LightBlue = IM_COL32(76, 111, 155, 255);
    static const ImU32 LightGreen = IM_COL32(128, 150, 90, 255);
    static const ImU32 LightGray = IM_COL32(160, 160, 160, 255);
};

    static ImU32 GetRandomColor()
    {
        
        // std::vector<ImU32> colors = { Color::Red, Color::Green, Color::Blue, Color::Cyan, Color::Purple, Color::Magenta };
        std::vector<ImU32> colors = { Color::LightRed, Color::LightGreen, Color::LightBlue, Color::LightGray };
        std::srand(std::time(nullptr));
        int randomIndex = std::rand() % colors.size();
        return colors[randomIndex];
    }

    // static std::unordered_map<int, u32> uniqueRectColors;

    static void DebugColor(const char* label, ImU32& color)
    {
        float fColor[4];
        // ImGuiColorEditFlags misc_flags = ImGuiColorEditFlags_AlphaPreview;
        fColor[0] = ((color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
        fColor[1] = ((color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
        fColor[2] = ((color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;
        fColor[3] = ((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;

        if (ImGui::ColorEdit4(label, fColor)) {
            color = ImGui::ColorConvertFloat4ToU32(ImVec4(fColor[0], fColor[1], fColor[2], fColor[3]));
        }
    }

    static void DebugRect(float x, float y, float w, float h, const std::string& aLabel = "")
    {
        ImRect rect(ImVec2(x, y), ImVec2(x + w, y + h));
        DebugRect(rect, aLabel);
    }

    static void DebugRect(const ImRect& rect, const std::string& aLabel = "", bool bFilled = false)
    {
        int color = 0xAAEAFFAA;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        if (aLabel.size() > 0) {
            draw_list->AddText(rect.Min, 0xFFFFFFFF, aLabel.c_str());

            // if (uniqueRectColors.find(ColorHash) == uniqueRectColors.end())
            {
                // uniqueRectColors[ColorHash] = color;
            }

            // color = uniqueRectColors[ColorHash];
        }
        if (bFilled) {
            draw_list->AddRect(rect.Min, rect.Max, color);
        } else {
            draw_list->AddRectFilled(rect.Min, rect.Max, color);
        }
    }

    static void sprint_f(std::string& _outString, const std::string& format, ...) 
    { 
        va_list args; 
        va_start(args, format); 
        
        int size = std::vsnprintf(nullptr, 0, format.c_str(), args) + 1; // +1 for null terminator 
        
         std::vector<char> buffer(size); 
         std::vsnprintf(buffer.data(), size, format.c_str(), args); 
         
         va_end(args); 
         
         _outString = std::string(buffer.data(), buffer.size() - 1); 
    }
};

#include <ctime>
#include <iostream>
#include <random>

class Random {
public:
    static int RandomIntRange(int min, int max)
    {
        static std::mt19937 generator(static_cast<unsigned long>(std::time(nullptr)));
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(generator);
    }

    static float RandomFloat()
    {
        static std::mt19937 generator(static_cast<unsigned long>(std::time(nullptr)));
        std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
        return distribution(generator);
    }
};
