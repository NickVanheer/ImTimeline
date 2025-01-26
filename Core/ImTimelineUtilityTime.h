#pragma once

#include <iostream>
#include <chrono>
#include <string>
#include <unordered_map>
#include "../dependencies/imgui/imgui.h"

class ScopedTimer 
{
public:
    ScopedTimer(const std::string& name)
        : mName(name), start(std::chrono::high_resolution_clock::now()) {}

    ~ScopedTimer() 
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::chrono::duration<double, std::milli> durationMilli = duration;
        ScopedTimer::st_TimeMap[mName] = durationMilli.count();
    }

    static void DebugPrint()
    {
        for (const auto& [name, time] : ScopedTimer::st_TimeMap)
        {
            ImGui::Text("%s: %.8fms", name.c_str(), time);
        }
    }

public:
    static std::unordered_map<std::string, double> st_TimeMap;

private:
    std::string mName;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

std::unordered_map<std::string, double> ScopedTimer::st_TimeMap;
