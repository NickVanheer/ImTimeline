#pragma once
#include "../Core/CoreDefines.h"
#include "../Core/ImTimelineLog.h"

#include <bitset>

#define IMTIMELINE_VERSION_STR "0.2.0 WIP"
#define IMTIMELINE_VERSION_NUM 002010

enum TimelineFlags : s32 {
    TimelineFlags_None,
    TimelineFlags_SkipTimelineRebuild,
    TimelineFlags_Max,
};

enum TimelineSectionFlags : s32 {
    TimelineSectionFlags_None,
    TimelineSectionFlags_Max,
};

typedef int TimelineNodeFlags;
enum eTimelineNodeFlags : s32 {
    TimelineNodeFlags_None = 0,
    TimelineNodeFlags_CustomDraw,
    TimelineNodeFlags_AutofitHeight, // Display a node with its height fit to the timeline size
    TimelineNodeFlags_UseSectionBackground,
    TimelineNodeFlags_MoveSurroundingNodesToTheRight, // On insertion or moving, move the surrounding nodes to the right tomake this node fit
    TimelineNodeFlags_MovedToDifferentTimeline,
    TimelineNodeFlags_Max
};

enum TimelineDisplayFlags : s32 {
    None,
    Max,
};

enum TimelineSectionDisplayFlags : s32 {
    TimelineSectionDisplayFlagsNone,
    TimelineSectionDisplayFlagsMax,
};

enum class ePlayingNodeState {
    None,
    IsPlayed,
    IsFinishedPlaying,
    IsPaused,
    Max
};

const int STACK_INTERVAL_THRESHOLD = 1; // unused

typedef s32 NodeID;
static const NodeID InvalidNodeID = -1;

struct sGenericDisplayProperties {
    f32 mHeight = 0.0f;
    f32 mWidth = 0.0f;
    ImU32 mBackgroundColor;
    ImU32 mBackgroundColorTwo;
    ImU32 mForegroundColor = IM_COL32(255, 255, 255, 255);
    s32 AccentThickness = 8; // todo split for node drawing into text padding and node padding
    f32 Spacing = 0.0f;
    f32 BorderRadius = 0.0f;
    f32 BorderThickness = 1;
};

class CustomNodeBase;

namespace ImTimeline {
class Timeline;
}

namespace ImTimelineInternal {
class MoveNodeCommand;
}

struct TimelineNode {
    sGenericDisplayProperties displayProperties;
    std::bitset<eTimelineNodeFlags::TimelineNodeFlags_Max> mFlags;
    std::string displayText;
    s32 start = 0;
    s32 end = 0;

    NodeID GetID() const { return ID; }
    s32 GetSection() const { return section; }
    std::shared_ptr<CustomNodeBase> GetCustomNode() const { return CustomNode; }

    bool operator<(const TimelineNode& other) const
    {
        return end < other.start;
    }

    virtual ~TimelineNode()
    {
    }

    void InitalizeCustomNode(std::shared_ptr<CustomNodeBase> node, bool bCustomUI = true)
    {
        CustomNode = node;
        mFlags.set(eTimelineNodeFlags::TimelineNodeFlags_CustomDraw, bCustomUI);
    }

    // TimelineNode& operator=(TimelineNode other) {

    // }

    void Setup(s32 a_cat, s32 a_start, s32 a_end, const std::string& a_text)
    {
        ID = InvalidNodeID;
        section = a_cat;
        start = a_start;
        end = a_end;
        displayText = a_text;
    }

    virtual void OnClone(const CustomNodeBase& nodeData) { } // TODO

private:
    std::shared_ptr<CustomNodeBase> CustomNode = nullptr;
    NodeID ID = InvalidNodeID;
    s32 section = 0;

    friend class ::ImTimeline::Timeline;
    friend class ::ImTimelineInternal::MoveNodeCommand;
};

class CustomNodeBase {
public:
    CustomNodeBase()
    {
    }

    virtual void OnDraw(const TimelineNode& nodeData, ImRect drawArea, bool& refIsSelected) { }
    virtual void OnDebugGUI() { }

    virtual void OnTimelinePlayerSetup() {}; // node is about to be played
    bool IsReady() { return true; }; // node can be played
    virtual void OnNodeActivate() {}; // node play
    virtual void OnNodeDeactivate() {}; // node stop play
};

struct sNodePlayProperties {
    // todo?
};

namespace ImTimeline {
class TimelinePlayer;
}

struct TimelineSectionProperties {
    std::string mSectionName;
    sGenericDisplayProperties mDisplayProperties;
    std::bitset<TimelineSectionDisplayFlags::TimelineSectionDisplayFlagsMax> mFlags;

    s32 mEndTimestamp;
    // ImRect mLegendAndContentRect; omit: Calculated by ImTimeline
};

namespace ImTimeline {
class TimelinePlayer;
}

class ImDataController;
class INodeView;
class ITimelinePlayerView;

struct sTimelineSection {
    u32 mID = -1;
    std::shared_ptr<ImTimeline::TimelinePlayer> mTimelinePlayer;
    ImDataController* mNodeData = nullptr;
    std::shared_ptr<INodeView> mNodeView;
    bool mbIsInitialized = false;

    TimelineSectionProperties mProps;

    ~sTimelineSection()
    {
        OnFinalize();
    }

    void OnFinalize()
    {
        delete mNodeData;
        mNodeData = nullptr;
    }
};

using TimelineDataMap = std::unordered_map<u32, sTimelineSection>;

namespace ImTimeline {
class Timeline;

}
class BaseCommand {
public:
    BaseCommand(ImTimeline::Timeline* aTimeline) { mTimeline = aTimeline; }
    virtual void command_do() = 0;
    virtual void command_undo() = 0;
    virtual ~BaseCommand()
    {
    }

protected:
    ImTimeline::Timeline* mTimeline = nullptr;
};

struct NodeInitDescriptor {
    NodeID ID = InvalidNodeID;
    std::string label = "";
    int section = 0;
    int start = 0;
    int end = 0;
    std::shared_ptr<CustomNodeBase> customNode = nullptr;
    bool bMoveOverlappingNext = false; // if the wdith of the node is greater than the start of the next node, the next node will be moved to make place

    NodeInitDescriptor() { }
    NodeInitDescriptor(std::string label, s32 pSection, s32 pStart, s32 pEnd, std::shared_ptr<CustomNodeBase> customNode)
        : label(std::move(label))
        , section(pSection)
        , start(pStart)
        , end(pEnd)
        , customNode(std::move(customNode))
    {
    }
};

struct ImTimelineStyle {
    float LegendWidth = 200;
    int HeaderHeight = 15;
    int ScrollbarThickness = 12;
    unsigned int HeaderBackgroundColor = 0xFF3D3837;
    unsigned int HeaderTimeStampColor = 0xFFC4C4C4;
    ImU32 SelectedNodeOutlineColor = 0xEA7915FF;
    bool HasScrollbar = true;
    bool HasSeekbar = true;
    ImU32 SeekbarColor = 0xFF2A2AFF;
    f32 SeekbarWidth = 3.0f;
};

struct sInputData {
    ImVec2 MousePos;
    bool LeftMouseDown = false;
    f32 MouseDownDuration = 0.0f;
    f32 MouseDownDurationLastFrame = 0.0f;
    bool RightMouseDown = false;
    bool IsMovingScrollBar = false;
    f32 MouseScrollVertical = 0.0f;
    f32 ScrollSpeed = 1.0f; //TODO: allow for more granular slower scroll speeds
    s32 ScrollDirection = -1;
};

enum class eDragState {
    None,
    DragNode,
    DragNodeEnd
};

struct DragData {
    eDragState DragState = eDragState::None;
    TimelineNode DragNode = TimelineNode();
    s32 DragStartTimestamp = -1;
    ImRect DragRect;
    ImVec2 DragStartMouseDelta;
};

enum eNextAction : s32 {
    ActionNone,
    ActionDelete,
    ActionUndo,
    ActionRedo,
    ActionMax,
};

namespace ImTimelineLicense {
static const char* LicenseNameof = R"(
MIT License

Copyright (c) 2016 - 2024 Daniil Goncharov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
)";
}