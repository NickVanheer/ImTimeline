/**
 * @file   Timeline.h
 * @author Nick
 * @brief  Main context class for a single timeline group, housing its nodes
 * and an API to common features such as adding, deleting, changing properties
 * @date   2024.12
 */

#pragma once
#include "TimelineCore/TimelineDefines.h"
#include "Core/IDGeneratorUtility.h"

struct ImDrawList;
struct ImRect;
class ImDataController;
class ITimelinePlayerView;

namespace ImTimelineInternal {
class MoveNodeCommand;
class DeleteCommand;
}

namespace ImTimeline
{
class Timeline {
public:
    Timeline();
    Timeline(const Timeline&) = delete;
    Timeline(Timeline&&) = delete;
    Timeline& operator=(const Timeline&) = delete;
    Timeline& operator=(Timeline&&) = delete;
    virtual ~Timeline() = default;

    bool InitializeTimelineSection(s32 index, std::string name, ImDataController* data = nullptr);
    bool InitializeTimelineSectionEx(s32 index, std::string name, ImDataController* data, std::shared_ptr<ITimelinePlayerView> playerViewVUI, std::shared_ptr<INodeView> nodeViewUI);

    sTimelineSection& GetTimelineSection(s32 index);
    const sTimelineSection& GetTimelineSection(s32 index) const;

    sGenericDisplayProperties& GetSectionDisplayProperties(s32 section_index);
    bool HasSection(s32 section) const;

    void SetTimelineName(s32 index, std::string name);
    void SetTimelineHeight(s32 index, f32 height);
    void SetTimelineStyle(const ImTimelineStyle& style) { mStyle = style; };
    void SetTimelinePlayerUI(std::shared_ptr<ITimelinePlayerView> uiView);
    void SetNodeViewUI(std::shared_ptr<INodeView> uiView);

    TimelineNode* AddNewNode(TimelineNode* node);
    TimelineNode& AddNewNode(s32 section, s32 start, s32 end, const std::string& text = "", std::shared_ptr<CustomNodeBase> customNodeUI = nullptr);
    void DeleteItem(s32 section, s32 start, s32 end);
    void DeleteSelection();
    void DeleteSection(s32 section);
    void MoveNode(TimelineNode* node, s32 newStart, s32 newSection);

    TimelineNode* FindNodeByNodeID(NodeID nodeID) const;
    TimelineNode* FindNodeByNodeID(s32 section, NodeID nodeID) const;

    ////
    bool DrawTimeline();
    void DrawDebugGUI();

    bool IsDragging() const { return mDragData.DragState != eDragState::None; }

    void SetStartFrame(s32 frame) { mStartFrame = frame; }
    void SetMaxFrame(s32 frame) { mFrameMax = frame; }
    s32 GetMaxFrame() const { return mFrameMax; }
    void SelectNode(TimelineNode* node) { mSelectedNode = node; };
    TimelineNode* GetSelectedNode() const { return mSelectedNode; }

    s32 GetTimestampAtPixelPosition(f32 pixelPosition);
    s32 GetPixelPositionAtTimestamp(s32 timestamp);

    f32 GetScale() const { return mZoom; }
    void SetScale(f32 scale) { mZoom = scale; }

    f32 GetStartTimestamp() const { return mStartFrame; }
    void SetStartTimestamp(s32 timestamp) { mStartFrame = timestamp; }
    s32 GetSelectedSection() const { return mSelectedTimelineIndex; }
    void SetSelectedTimeline(s32 index) { mSelectedTimelineIndex = index; }

    const sInputData& GetLastInputData() const { return mInputData; }

    void Undo();
    void Redo();

    // Debug
    
    void OnCoreDebugGUI();
    void OnDebugGUITimelineList();
    void OnDebugGuiDisplayProps(sGenericDisplayProperties& displayPropsRef);
    void OnDebugGUIPerformance();
    void OnDebugGUIPlayer();
    void OnDebugGUILog();
    bool OnDebugGUISelection();
    void OnDebugGUIRightSidePane();

    // Globals scoped to this timeline:
    std::bitset<TimelineFlags::TimelineFlags_Max> mFlags;

    ImTimelineStyle mStyle;
    DragData mDragData;
    ImRect mContentAreaRect;

    struct sPanningProperties {
        ImVec2 panningViewSource;
        int panningViewFrame;
    };

    sPanningProperties mPanningData;

protected:
    void drawSeekbarUI();
    void updateTimelinePlayer(f32 deltaTime);
    void forceRebuild(s32 section, NodeInitDescriptor descriptor = NodeInitDescriptor());
    virtual void DrawHeader(const ImRect& area);
    virtual void DrawScrollbar();
    f32 getSeekbarPositionX();

    TimelineDataMap mTimelines;
    std::bitset<(s32)eNextAction::ActionMax> mNextActionFlags;

    std::shared_ptr<TimelinePlayer> mMainPlayer;
    TimelineNode* mSelectedNode = nullptr;

private:
    void CollectInputData(sInputData& a_outInputData, f32 aDeltaTime);
    void PushCommand(std::unique_ptr<BaseCommand> command);
    void SetCommandEnable(bool aEnable) { mEnableCommands = aEnable; }
    void updateSideDragLogic(f32 deltaTime);

    IDGenerator mIDGenerator;
    s32 mStartFrame = 0;
    s32 mStartFrameVertical = 0;
    f32 mZoom = 10.f;
    f32 mZoomLerpTarget = 10.f;
    s32 mFrameMin = 0;
    s32 mFrameMax = 0;
    s32 mVisibleFrameCount = 0;
    s32 mSelectedTimelineIndex = -1;
    s32 mCurrentSectionColorIndex = 0;

    TimelineNode mEmptyDummyNode = TimelineNode();
    sTimelineSection mEmptyDummySection = sTimelineSection();

    // command
    std::vector<std::unique_ptr<BaseCommand>> mCommandHistory;
    s32 mCommandIndex = -1;
    bool mEnableCommands = true;

    // input
    sInputData mInputData;

    f32 mEdgeMoveAmount = 0.0f;
    f32 mEdgeMoveSpeed = 15.0f;

    friend class ::ImTimelineInternal::MoveNodeCommand;
    friend class ::ImTimelineInternal::DeleteCommand;
};

} //ImTimeline