#pragma once
#include <iostream>
#include <deque>
#include <string>
#include <mutex>

class ImTimelineLog {
public:

  enum class eLogLevel
    {
        None,
        Warning,
        Error,
        Max
    };

    ImTimelineLog(const ImTimelineLog&) = delete;
    ImTimelineLog& operator=(const ImTimelineLog&) = delete;

    static ImTimelineLog& getInstance(size_t capacity = 10) {
        static ImTimelineLog instance(capacity);
        return instance;
    }

    void addLog(eLogLevel aLevel, const std::string format, ...) {
        std::lock_guard<std::mutex> lock(mMutex);

         if (mLogBuffer.size() == mCapacity) {
            mLogBuffer.pop_front(); 
        }

        va_list args; 
        va_start(args, format); 
        
        int size = std::vsnprintf(nullptr, 0, format.c_str(), args) + 1; // +1 for null terminator 
        
         std::vector<char> buffer(size); 
         std::vsnprintf(buffer.data(), size, format.c_str(), args); 
         
         va_end(args); 
         
         LogEntry newEntry = {};
         newEntry.message = std::string(buffer.data(), buffer.size() - 1);
         newEntry.level = aLevel;

        mLogEntryID++;
        newEntry.timestamp = mLogEntryID;
        
         //newEntry.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        mLogBuffer.emplace_back(newEntry);

        std::cout << newEntry.message << std::endl;
    }

    void OnDebugGUI() {
        std::lock_guard<std::mutex> lock(mMutex); 
        
        if(ImGui::Button("Clear"))
        {
            mLogBuffer.clear();
        }

        for (size_t i = mLogBuffer.size(); i > 0; --i) {
            auto& logEntry =  mLogBuffer[i - 1];
             if (logEntry.level == eLogLevel::Warning)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); 
    }
            ImGui::Text("%d - %s", logEntry.timestamp, logEntry.message.c_str());
            ImGui::PopStyleColor();
        }
    }

private:
    struct LogEntry {
        std::string message;
        s32 timestamp;
        eLogLevel level = eLogLevel::None;
    };

    ImTimelineLog(size_t capacity) : mCapacity(capacity) {}

    std::deque<LogEntry> mLogBuffer;
    size_t mCapacity = 100;
    mutable std::mutex mMutex;
    s32 mLogEntryID = 0;
};

#define LOG_INFO(str) ImTimelineLog::getInstance().addLog(ImTimelineLog::eLogLevel::None, str, NULL);
#define LOG_INFO_PRINTF(str, ...) ImTimelineLog::getInstance().addLog(ImTimelineLog::eLogLevel::None, str, __VA_ARGS__);
#define LOG_WARNING_PRINTF(str, ...) ImTimelineLog::getInstance().addLog(ImTimelineLog::eLogLevel::Warning, str, __VA_ARGS__);
