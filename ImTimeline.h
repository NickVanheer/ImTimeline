/**
 * @file   ImTimeline.h
 * @author Nick
 * @brief Definitions of Timeline.h and an immediate mode API
 * @date   2024.12
 */

#pragma once
#include "Timeline.h"

namespace ImTimeline 
{

struct stImmediateModeTimelineData {
    Timeline* mTimelineObject = nullptr;
    std::string mIDString;
    std::unordered_map<NodeID, bool> mAddNodeMap;

    enum eImmediateFlags {
        BeginTimelineCalled,
        EndTimelineCalled,
        ImmediateFlagsMax
    };

    std::bitset<eImmediateFlags::ImmediateFlagsMax> mFlags;
};

struct stImmediateData {
    std::unordered_map<std::string, stImmediateModeTimelineData> mTimelineDataMap;
    stImmediateModeTimelineData* mCurrentTimelineData = nullptr;
};

static stImmediateData GImmediateData;

void BeginTimeline(const char* str_id, const ImTimelineStyle& style = ImTimelineStyle());
TimelineNode* BeginTimelineContent(NodeID UUID, const std::string& label, s32 section, s32 start, s32 end, std::shared_ptr<CustomNodeBase> customNode = nullptr);
TimelineNode* BeginTimelineContent(const NodeInitDescriptor& nodeDescriptor);
void SetTimelineProperties(s32 section_id, const TimelineSectionProperties& properties);

ImTimelineStyle& GetTimelineStyle();
sGenericDisplayProperties& GetTimelineDisplayProperties(s32 section_id);

void SetTimelineName(s32 section_id, std::string name);

void ShowActiveTimeline();
void ShowTimelineDebugUI(const char* str_id);
}
