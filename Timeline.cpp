#include "Timeline.h"
#include "TimelineCore/ImTimeline_internal.h"

#include "Core/ImTimelineUtilityTime.h"
#include "TimelineData/ImDataController.h"
#include "TimelineViews/INodeView.h"
#include "TimelineCore/TimelinePlayer.h"

namespace ImTimeline {

Timeline::Timeline()
{
    mEmptyDummyNode.mFlags.set(eTimelineNodeFlags::TimelineNodeFlags_UseSectionBackground, true);
    mEmptyDummyNode.mFlags.set(eTimelineNodeFlags::TimelineNodeFlags_AutofitHeight, true);
    mEmptyDummyNode.mFlags.set(eTimelineNodeFlags::TimelineNodeFlags_MoveSurroundingNodesToTheRight, true);
    mEmptyDummyNode.mFlags.set(eTimelineNodeFlags::TimelineNodeFlags_MovedToDifferentTimeline, false);

    mMainPlayer = std::make_shared<TimelinePlayer>();

    IM_ASSERT(mMainPlayer != nullptr);

    if (mMainPlayer->IsSetup() == false) {
        mMainPlayer->Setup(nullptr, 0);
    }

    if (mMainPlayer->GetViewUI() == nullptr) {
        std::shared_ptr debugView = ImTimelineInternal::CreateDefaultPlayerView();
        SetTimelinePlayerUI(debugView);
    }
}

/* NODE ADD & COMMAND LOGIC */

// memo: this parameter might as well been a NodeInitDescriptor and that would clean things up, but maybe further down the line,
// more data would be added to the node, so just in case pass & copy the entire structure.
TimelineNode* Timeline::AddNewNode(TimelineNode* node)
{
    IM_ASSERT(node != nullptr);
    if (node->ID == InvalidNodeID) {
        node->ID = mIDGenerator.GetUniqueID();
    }

    InitializeTimelineSection(node->section, "Unnamed");
    if (mTimelines.find(node->section) == mTimelines.end()) {
        return nullptr;
    }

    if (mTimelines[node->section].mbIsInitialized == false) {
        return nullptr;
    }
    if (mTimelines[node->section].mNodeData == nullptr) {
        IM_ASSERT(false); // arienai
    }

    // Node is not new, apply existing settings
    if (node->mFlags.test(eTimelineNodeFlags::TimelineNodeFlags_MovedToDifferentTimeline)) {
        // good!
    } else {
        node->mFlags = mEmptyDummyNode.mFlags;

        if (node->CustomNode != nullptr) {
            node->mFlags.set(eTimelineNodeFlags::TimelineNodeFlags_CustomDraw, true);
        }

        // TODO what if we want to set default flags on a node-by-node basis, or pass flags in the NodeInitDescriptor?
    }

    // node gets recreated internally, original pointer can be deleted
    NodeInitDescriptor descriptor;
    bool bMoveOverlap = node->mFlags.test(eTimelineNodeFlags::TimelineNodeFlags_MoveSurroundingNodesToTheRight);
    descriptor.bMoveOverlappingNext = bMoveOverlap;

    IM_ASSERT(mTimelines[node->section].mNodeData != nullptr);

    mTimelines[node->section].mNodeData->emplace_back_direct(*node, descriptor);

    NodeInitDescriptor searchDescriptor;
    searchDescriptor.ID = node->ID;
    TimelineNode* newlyAddedNode = mTimelines[node->section].mNodeData->get_node_id(searchDescriptor);

    IM_ASSERT(newlyAddedNode != nullptr);

    if (newlyAddedNode->end > this->mFrameMax)
        this->mFrameMax = newlyAddedNode->end + 50;

    if (newlyAddedNode->end > mTimelines[newlyAddedNode->section].mProps.mEndTimestamp) {
        mTimelines[newlyAddedNode->section].mProps.mEndTimestamp = newlyAddedNode->end;
    }

    return newlyAddedNode;
}

TimelineNode& Timeline::AddNewNode(s32 section, s32 start, s32 end, const std::string& text, std::shared_ptr<CustomNodeBase> customNodeUI)
{
       NodeID uniqueID = mIDGenerator.GetUniqueID();

    auto cmd = std::make_unique<ImTimelineInternal::AddCommand>(this);
    cmd->mNewNode.ID = uniqueID;
    cmd->mNewNode.section = section;
    cmd->mNewNode.start = start;
    cmd->mNewNode.end = end;
    cmd->mNewNode.displayText = text;
    
    if(customNodeUI)
    {
        bool bCustomUI = true;
        cmd->mNewNode.InitalizeCustomNode(customNodeUI, bCustomUI);
    }

    cmd->command_do();

    PushCommand(std::move(cmd));
    // WARNING mNewNode becomes invalidated

    if (mTimelines.find(section) == mTimelines.end()) {
        LOG_WARNING_PRINTF("DeleteItem section %d does not exist", section);
        return mEmptyDummyNode;
    }

    NodeInitDescriptor searchDescriptor;
    searchDescriptor.ID = uniqueID;
    TimelineNode* node = mTimelines[section].mNodeData->get_node_id(searchDescriptor);

    if (node) {
        return *node;
    }

    return mEmptyDummyNode;
}

/* NODE MOVE & COMMAND LOGIC */

void Timeline::MoveNode(TimelineNode* node, s32 newStart, s32 newSection)
{
    LOG_INFO("MoveCommand:");
    auto cmd = std::make_unique<ImTimelineInternal::MoveNodeCommand>(this);

    cmd->mNodeToMove = mSelectedNode;
    cmd->mNewStart = newStart;
    cmd->mNewSectionID = newSection;
    cmd->command_do();

    PushCommand(std::move(cmd));
}

TimelineNode* Timeline::FindNodeByNodeID(NodeID nodeID) const
{
    if (nodeID == InvalidNodeID) {
        return nullptr;
    }
    NodeInitDescriptor searchDescriptor;
    searchDescriptor.ID = nodeID;

    for (auto& timeline : mTimelines) {
        TimelineNode* node = timeline.second.mNodeData->get_node_id(searchDescriptor);
        if (node != nullptr) {
            return node;
        }
    }
    return nullptr;
}

TimelineNode* Timeline::FindNodeByNodeID(s32 section, NodeID nodeID) const
{
    if (nodeID == InvalidNodeID) {
        return nullptr;
    }
    NodeInitDescriptor searchDescriptor;
    searchDescriptor.ID = nodeID;
    auto& catData = GetTimelineSection(section);
    TimelineNode* node = catData.mNodeData->get_node_id(searchDescriptor);
    return node;
}

bool Timeline::HasSection(s32 section) const
{
    if (mTimelines.find(section) == mTimelines.end()) {
        return false;
    }

    return true;
}

void Timeline::Undo()
{
    if (mCommandIndex >= 0) {
        mCommandHistory[mCommandIndex]->command_undo();
        --mCommandIndex;
    }
}

void Timeline::Redo()
{
    if (mCommandIndex + 1 < static_cast<int>(mCommandHistory.size())) {
        ++mCommandIndex;
        mCommandHistory[mCommandIndex]->command_do();
    }
}

void Timeline::DeleteItem(s32 section, s32 start, s32 end)
{
    LOG_INFO("DeleteItem Command:");
    auto cmd = std::make_unique<ImTimelineInternal::DeleteCommand>(this);

    cmd->section = section;
    cmd->start = start;
    cmd->end = end;
    cmd->command_do();

    PushCommand(std::move(cmd));
}

void Timeline::DeleteSelection()
{
    if (mSelectedNode == nullptr)
        return;

    DeleteItem(mSelectedNode->section, mSelectedNode->start, mSelectedNode->end);
    mSelectedNode = nullptr;

    // add? maybe: delete selection when a node gets deleted by DeleteItem that is the selection
    // not easy to do with the current setup
}

void Timeline::DeleteSection(s32 section)
{
    if (HasSection(section) == false) {
        return;
    }

    DeleteItem(section, 0, mTimelines[section].mProps.mEndTimestamp);
    mTimelines.erase(section);
}


bool Timeline::InitializeTimelineSection(s32 index, std::string name, ImDataController* data /* nullptr */)
{
    ImU32 bgColor = ImTimelineUtility::Color::LightGray;

    auto itr = mTimelines.find(index);
    if (itr != mTimelines.end() && itr->second.mbIsInitialized == true) {
        bgColor = itr->second.mProps.mDisplayProperties.mBackgroundColor;
        return false;
    } else {
        std::vector<ImU32> colors = { ImTimelineUtility::Color::LightBlue, ImTimelineUtility::Color::LightGreen, ImTimelineUtility::Color::LightRed, ImTimelineUtility::Color::LightGray };

        int color_id = mCurrentSectionColorIndex % colors.size();
        bgColor = colors[color_id];
        mCurrentSectionColorIndex++;
    }

    sTimelineSection& section = mTimelines[index]; // new or existing
    section.mID = index;
    section.mProps.mDisplayProperties.mHeight = 40.0f; // todo set section defaults
    section.mProps.mSectionName = name;
    section.mProps.mDisplayProperties.mBackgroundColor = bgColor;

    if (data != nullptr) {
        section.mNodeData = data;
    }

    // if the first node of a section, allocate memory for all the nodes in the section
    if (section.mNodeData == nullptr) {
        section.mNodeData = ImTimelineInternal::CreateDefaultDataController();
    }

    if (section.mNodeView == nullptr) {
        section.mNodeView = ImTimelineInternal::CreateDefaultNodeView();
    }

    if (section.mTimelinePlayer == nullptr) {
        std::shared_ptr<TimelinePlayer> playerPtr = std::make_shared<TimelinePlayer>();
        section.mTimelinePlayer = playerPtr;
    }

    // individual timelines will send their signals to the parent timeline view
    if (mMainPlayer && mMainPlayer->GetViewUI() && section.mTimelinePlayer) {
        section.mTimelinePlayer->SetViewUI(mMainPlayer->GetViewUI());
    }

    if (section.mTimelinePlayer && section.mTimelinePlayer->IsSetup() == false) {
        s32 startTime = 0;
        section.mTimelinePlayer->Setup(section.mNodeData, startTime);

        if (mMainPlayer)
            mMainPlayer->AddPlayer(section.mTimelinePlayer);
    }

    if (section.mbIsInitialized == false) {
        section.mbIsInitialized = true;
        LOG_INFO("Initialize new timeline section");
    }

    return true;
}

bool Timeline::InitializeTimelineSectionEx(s32 index, std::string name, ImDataController* data, std::shared_ptr<ITimelinePlayerView> playerViewVUI, std::shared_ptr<INodeView> nodeViewUI)
{
    ImU32 bgColor = ImTimelineUtility::Color::LightGray;

    auto itr = mTimelines.find(index);
    if (itr != mTimelines.end() && itr->second.mbIsInitialized == true) {
        bgColor = itr->second.mProps.mDisplayProperties.mBackgroundColor;
        return false;
    } else {
        std::vector<ImU32> colors = { ImTimelineUtility::Color::LightBlue, ImTimelineUtility::Color::LightGreen, ImTimelineUtility::Color::LightRed, ImTimelineUtility::Color::LightGray };

        int color_id = mCurrentSectionColorIndex % colors.size();
        bgColor = colors[color_id];
        mCurrentSectionColorIndex++;
    }

    sTimelineSection& section = mTimelines[index]; // new or existing
    section.mID = index;
    section.mProps.mDisplayProperties.mHeight = 35.0f;
    section.mProps.mSectionName = name;
    section.mProps.mDisplayProperties.mBackgroundColor = bgColor;

    if (section.mNodeData == nullptr) {
        section.mNodeData = data;
    }

    // if the first node of a section, allocate memory for all the nodes in the section
    if (section.mNodeData == nullptr) {
        section.mNodeData =  ImTimelineInternal::CreateDefaultDataController();
    }

    if (section.mNodeView == nullptr) {
        section.mNodeView = nodeViewUI;
    }

    if (section.mNodeView == nullptr) {
        section.mNodeView = ImTimelineInternal::CreateDefaultNodeView();
    }

    if (section.mTimelinePlayer == nullptr) {
        std::shared_ptr<TimelinePlayer> playerPtr = std::make_shared<TimelinePlayer>();
        section.mTimelinePlayer = playerPtr;
    }

    // individual timelines will send their signals to the parent timeline view
    if (playerViewVUI.get() && section.mTimelinePlayer) {
        section.mTimelinePlayer->SetViewUI(playerViewVUI);
    }

    if (section.mTimelinePlayer->GetViewUI()) {

    } else {
        section.mTimelinePlayer->SetViewUI(mMainPlayer->GetViewUI());
    }

    if (section.mTimelinePlayer && section.mTimelinePlayer->IsSetup() == false) {
        s32 startTime = 0;
        section.mTimelinePlayer->Setup(section.mNodeData, startTime);

        if (mMainPlayer)
            mMainPlayer->AddPlayer(section.mTimelinePlayer);
    }

    if (section.mbIsInitialized == false) {
        section.mbIsInitialized = true;
        LOG_INFO("Initialize new timeline section");
    }

    return true;
}

sTimelineSection& Timeline::GetTimelineSection(s32 index)
{
    if (mTimelines.find(index) == mTimelines.end()) {
        LOG_WARNING_PRINTF("GetTimelineSection section %d does not exist", index);
        return mEmptyDummySection;
    }

    return mTimelines[index];
}

const sTimelineSection& Timeline::GetTimelineSection(s32 index) const
{
    if (mTimelines.find(index) == mTimelines.end()) {
        LOG_WARNING_PRINTF("GetTimelineSection section %d does not exist", index);
        return mEmptyDummySection;
    }

    return mTimelines.at(index);
}

void Timeline::SetTimelineName(s32 index, std::string name)
{
    mTimelines[index].mProps.mSectionName = name;
}

void Timeline::SetTimelineHeight(s32 index, f32 height)
{
    if (mTimelines.find(index) == mTimelines.end()) {
        LOG_WARNING_PRINTF("GetTimelineSection section %d does not exist", index);
        return;
    }

    mTimelines[index].mProps.mDisplayProperties.mHeight = height;
}

void Timeline::SetTimelinePlayerUI(std::shared_ptr<ITimelinePlayerView> uiView)
{
    // old data might be deleted
    // std::shared_ptr<ITimelinePlayerView> baseView = std::dynamic_pointer_cast<ITimelinePlayerView>(debugView);
    mMainPlayer->SetViewUI(uiView);
}

void Timeline::SetNodeViewUI(std::shared_ptr<INodeView> uiView)
{
    for (auto& timeline : mTimelines) {
        if (timeline.second.mNodeView != uiView) {
            timeline.second.mNodeView = uiView;
        }
    }
}

bool Timeline::DrawTimeline()
{
    ScopedTimer timer = ScopedTimer("Timeline Draw");

    ImGuiIO& io = ImGui::GetIO();
    CollectInputData(mInputData, io.DeltaTime);
    updateTimelinePlayer(io.DeltaTime);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    int frameCount = ImMax(mFrameMax - mFrameMin, 1);

    // zoom in/out
    mVisibleFrameCount = (int)floorf((canvas_size.x - mStyle.LegendWidth) / mZoom);
    mStartFrame += mInputData.MouseScrollVertical * mInputData.ScrollDirection * mInputData.ScrollSpeed;
    mStartFrame = ImClamp(mStartFrame, mFrameMin, mFrameMax - mVisibleFrameCount);
    mZoom = ImLerp(mZoom, mZoomLerpTarget, 0.10f);

    if (mVisibleFrameCount >= frameCount && mStartFrame)
        mStartFrame = mFrameMin;

    if(IsDragging())
    {
         ImGuiIO& io = ImGui::GetIO();
        updateSideDragLogic(io.DeltaTime);
    }

    if(mStartFrame + mVisibleFrameCount > mFrameMax)
    {
        mFrameMax = mStartFrame + mVisibleFrameCount;
    }

    if(mStartFrame < 0)
    {
        mStartFrame = 0;
    }

    //
    for (auto& timeline : mTimelines) {
        if (timeline.second.mNodeView) {
            timeline.second.mNodeView->PreDraw(); // TODO add deltatime
        }
    }

    // TODO Header draw in INodeView
    ImRect headerRect;
    headerRect.Min = canvas_pos;
    headerRect.Max = ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + mStyle.HeaderHeight);
    DrawHeader(headerRect);

    ImVec2 childFrameSize(canvas_size.x, canvas_size.y - 8.f - headerRect.GetHeight() - (mStyle.HasScrollbar ? static_cast<int>(mStyle.ScrollbarThickness) : 0.f));

    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
    ImGui::BeginChild(889, childFrameSize, ImGuiChildFlags_FrameStyle);

    const ImVec2 contentMin = ImGui::GetItemRectMin();
    const ImVec2 contentMax = ImGui::GetItemRectMax();
    const ImRect contentRect(contentMin, contentMax);
    mContentAreaRect = contentRect;

    draw_list->PushClipRect(contentMin, contentMax, true);

    size_t customHeight = 0;

    for (auto& timeline : mTimelines) {
        ScopedTimer timer = ScopedTimer("NodeView Draw");
        f32 timeline_spacing = 5.0f;

        size_t localCustomHeight = timeline.second.mProps.mDisplayProperties.mHeight;
        ImVec2 pos = ImVec2(contentMin.x, contentMin.y + 1 + customHeight);
        ImVec2 sz = ImVec2(canvas_size.x + canvas_pos.x, pos.y + localCustomHeight);
        ImRect contentRect = ImRect(pos, sz);

        customHeight += localCustomHeight + timeline_spacing;

        if (timeline.second.mNodeView) {
            timeline.second.mNodeView->DrawNodeView(contentRect, timeline.second, this);
        }
    }

    if (IsDragging() && mSelectedNode != nullptr) {
        s32 mouseTimestamp = GetTimestampAtPixelPosition(mInputData.MousePos.x);

        if (mDragData.DragStartTimestamp == -1)
            mDragData.DragStartTimestamp = mouseTimestamp - mSelectedNode->start;

        mDragData.DragNode.start = mouseTimestamp - mDragData.DragStartTimestamp;
        f32 nodeWith = mSelectedNode->end - mSelectedNode->start;
        mDragData.DragNode.end = mDragData.DragNode.start + nodeWith;
        mDragData.DragNode.displayProperties.mBackgroundColor = mSelectedNode->displayProperties.mBackgroundColor + 0x00301000;

        if (mInputData.LeftMouseDown == false) {
            mDragData.DragState = eDragState::DragNodeEnd;
        }

        if (mDragData.DragState == eDragState::DragNodeEnd) {

            if(mSelectedTimelineIndex >= 0)
                {
                    MoveNode(mSelectedNode, mDragData.DragNode.start, mSelectedTimelineIndex);
                }
  
            mDragData.DragState = eDragState::None;
            mDragData.DragStartTimestamp = -1;
        }
    }

    if (IsDragging()) {
        f32 width = mDragData.DragRect.GetWidth();
        f32 height = mDragData.DragRect.GetHeight();
        ImVec2 slotP1(mInputData.MousePos.x - mDragData.DragStartMouseDelta.x, mInputData.MousePos.y - mDragData.DragStartMouseDelta.y);
        ImVec2 slotP2(slotP1.x + width, slotP1.y + height);

        // background drop shadow
        ImVec2 slotP3 = slotP2;
        slotP3.x = slotP2.x + 20;
        slotP3.y = slotP2.y + contentRect.GetHeight();
        ImRect bgRect = ImRect(slotP1, slotP3);
        draw_list->AddRectFilled(slotP1, slotP3, ImTimelineUtility::Color::Black);

        ImVec2 slotP2Node(slotP1.x + width / 2, slotP1.y + height / 2);

        TimelineSectionProperties props = mTimelines[mDragData.DragNode.section].mProps;
        if (mTimelines[mDragData.DragNode.section].mNodeView) {
            mTimelines[mDragData.DragNode.section].mNodeView->defaultNodeDraw(ImRect(slotP1, slotP2), mDragData.DragNode, this);
        }
    }

    draw_list->PopClipRect();

    if (mStyle.HasSeekbar) {
        drawSeekbarUI();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (mStyle.HasScrollbar) {
        DrawScrollbar();
    }

    return true;
}

void Timeline::DrawDebugGUI()
{
    if (ImGui::BeginTable("MainLayoutTable", 2)) {
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            OnCoreDebugGUI();
            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("Timeline", ImVec2(0, 250), true);
            OnDebugGUIRightSidePane();
            ImGui::EndChild();
        }
        ImGui::EndTable();
    }
}

void Timeline::DrawHeader(const ImRect& headerRect)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 headerSize(headerRect.GetWidth(), (f32)mStyle.HeaderHeight);

    ImVec2 canvas_pos = headerRect.Min;

    draw_list->AddRectFilled(headerRect.Min, headerRect.Max, mStyle.HeaderBackgroundColor, 0);

    bool bCanShowDebugMenu = true;
    if (bCanShowDebugMenu) {
        draw_list->AddText(headerRect.Min, mStyle.HeaderTimeStampColor, "Header");
        // todo flyout menu here with some options?
    }

    ImRect timestampAreaClippingRect = ImRect(headerRect.Min + ImVec2(mStyle.LegendWidth, 0), headerRect.Min + headerSize);
    draw_list->PushClipRect(timestampAreaClippingRect.Min, timestampAreaClippingRect.Max, true);

    if (timestampAreaClippingRect.Contains(mInputData.MousePos) && mInputData.LeftMouseDown && !IsDragging()) {
        s32 mouseTimestamp = GetTimestampAtPixelPosition(mInputData.MousePos.x);

        if (mMainPlayer)
            mMainPlayer->SetStartTimestamp(mouseTimestamp);
    }

    int useFrameStep = 1;
    int timestampCount = 10;

    auto drawLine = [&](int i, int height, bool bDrawLabel) {
        int window_x = (int)canvas_pos.x + int(i * mZoom) + mStyle.LegendWidth - int(mStartFrame * mZoom);
        int window_y = canvas_pos.y + height;
        draw_list->AddLine(ImVec2(window_x, canvas_pos.y), ImVec2(window_x, window_y), mStyle.HeaderTimeStampColor, 1);

        if (bDrawLabel) {
            std::string label;
            ImTimelineUtility::sprint_f(label, "%d", i);
            draw_list->AddText(ImVec2((float)window_x + 3.f, canvas_pos.y), mStyle.HeaderTimeStampColor, label.c_str());
        }
    };

    while ((timestampCount * mZoom) < 150) {
        timestampCount *= 2;
        useFrameStep *= 2;
    };
    int halfModFrameCount = timestampCount / 2;

    for (int i = mFrameMin; i <= mFrameMax; i += useFrameStep) {
        int multiplier = 1;

        if (i % halfModFrameCount == 0)
            multiplier = 2;

        int drawHeight = (mStyle.HeaderHeight / 2) * multiplier;

        bool bDrawText = ((i % timestampCount) == 0) || (i == mFrameMax || i == mFrameMin);
        drawLine(i, drawHeight, bDrawText);
    }

    ImGui::PopClipRect();

    // moving internal xy coord down
    ImGui::InvisibleButton("header_top_height", ImVec2(200, headerSize.y));
}

void Timeline::DrawScrollbar()
{
    if (mStyle.HasScrollbar == false)
        return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    ImVec2 scrollBarSize(canvas_size.x, 12.f);
    int frameCount = ImMax(mFrameMax - mFrameMin, 1);

    const float barWidthRatio = ImMin(mVisibleFrameCount / (float)frameCount, 1.f);
    const float barWidthInPixels = barWidthRatio * (canvas_size.x - mStyle.LegendWidth);

    ImGui::InvisibleButton("scrollBar", scrollBarSize);
    ImVec2 scrollBarMin = ImGui::GetItemRectMin();
    ImVec2 scrollBarMax = ImGui::GetItemRectMax();

    // ratio = number of frames visible in control / number to total frames

    float startFrameOffset = ((float)(mStartFrame - mFrameMin) / (float)frameCount) * (canvas_size.x - mStyle.LegendWidth);
    ImVec2 scrollBarA(scrollBarMin.x + mStyle.LegendWidth, scrollBarMin.y - 2);
    ImVec2 scrollBarB(scrollBarMin.x + canvas_size.x, scrollBarMax.y - 1);
    draw_list->AddRectFilled(scrollBarA, scrollBarB, 0xFF222222, 0);

    ImRect scrollBarRect(scrollBarA, scrollBarB);
    bool inScrollBar = scrollBarRect.Contains(mInputData.MousePos);

    draw_list->AddRectFilled(scrollBarA, scrollBarB, 0xFF101010, 8);

    ImVec2 scrollBarC(scrollBarMin.x + mStyle.LegendWidth + startFrameOffset, scrollBarMin.y);
    ImVec2 scrollBarD(scrollBarMin.x + mStyle.LegendWidth + barWidthInPixels + startFrameOffset, scrollBarMax.y - 2);
    draw_list->AddRectFilled(scrollBarC, scrollBarD, (inScrollBar || mInputData.IsMovingScrollBar) ? 0xFF606060 : 0xFF505050, 6);

    ImRect barHandleLeft(scrollBarC, ImVec2(scrollBarC.x + 14, scrollBarD.y));
    ImRect barHandleRight(ImVec2(scrollBarD.x - 14, scrollBarC.y), scrollBarD);

    bool onLeft = barHandleLeft.Contains(mInputData.MousePos);
    bool onRight = barHandleRight.Contains(mInputData.MousePos);

    static bool sizingRBar = false;
    static bool sizingLBar = false;

    draw_list->AddRectFilled(barHandleLeft.Min, barHandleLeft.Max, (onLeft || sizingLBar) ? 0xFFAAAAAA : 0xFF666666, 6);
    draw_list->AddRectFilled(barHandleRight.Min, barHandleRight.Max, (onRight || sizingRBar) ? 0xFFAAAAAA : 0xFF666666, 6);

    ImRect scrollBarThumb(scrollBarC, scrollBarD);

    if (mInputData.IsMovingScrollBar) {
        if (mInputData.LeftMouseDown == false) {
            mInputData.IsMovingScrollBar = false;
        } else {
            float framesPerPixelInBar = barWidthInPixels / (f32)mVisibleFrameCount;
            mStartFrame = int((mInputData.MousePos.x - mPanningData.panningViewSource.x) / framesPerPixelInBar) - mPanningData.panningViewFrame;
            mStartFrame = ImClamp(mStartFrame, mFrameMin, ImMax(mFrameMax - mVisibleFrameCount, mFrameMin));
        }
    } else {
        if (scrollBarThumb.Contains(mInputData.MousePos) && ImGui::IsMouseClicked(0)) {
            mInputData.IsMovingScrollBar = true;
            mPanningData.panningViewSource = mInputData.MousePos;
            mPanningData.panningViewFrame = -mStartFrame;
        }

        if (!sizingRBar && onRight && ImGui::IsMouseClicked(0))
            sizingRBar = true;
        if (!sizingLBar && onLeft && ImGui::IsMouseClicked(0))
            sizingLBar = true;
    }
}
/******* DEBUG UI **************/

sGenericDisplayProperties& Timeline::GetSectionDisplayProperties(s32 section_index)
{
    if (HasSection(section_index)) {
        return mTimelines.at(section_index).mProps.mDisplayProperties;
    }

    return mEmptyDummySection.mProps.mDisplayProperties;
}

void Timeline::OnCoreDebugGUI()
{
    ImGuiWindowFlags child_flags;
    ImGui::BeginChild("Scroll Area", ImVec2(-100, 250), ImGuiChildFlags_None, child_flags);
    ImGui::PushItemWidth(120);
    // ImGuiTreeNodeFlags_DefaultOpen
    if (ImGui::TreeNodeEx("Navigation")) {

        if (mMainPlayer) {
            s32 current_timestamp = mMainPlayer->GetCurrentTimestamp();
            if (ImGui::DragInt("Current Frame", &current_timestamp, 1.f, 0, mFrameMax)) {
                mMainPlayer->SetStartTimestamp(current_timestamp);
            }
        }

        ImGui::DragInt("Min", &mFrameMin, 1.f, 0, mFrameMax);
        ImGui::SameLine();
        ImGui::DragInt("Max", &mFrameMax);

        ImGui::DragInt("Start Frame", &mStartFrame, 1.f, 0, mFrameMax);
        ImGui::Text("Selected Timeline Index %d", mSelectedTimelineIndex);
        bool bIsDragging = IsDragging();
        ImGui::Checkbox("IsDragging (readonly)", &bIsDragging);

        if(ImGui::CollapsingHeader("Mouse Info"))
        {
            ImGui::Text("Mouse: %f", mInputData.MousePos.x);
            ImGui::Text("Mouse Down Duration: %f", mInputData.MouseDownDuration);

            s32 timestampMouse = GetTimestampAtPixelPosition(mInputData.MousePos.x);
            f32 pixelPositionTimestamp = GetPixelPositionAtTimestamp(timestampMouse);
            ImGui::Text("Mouse Timestamp: %d", timestampMouse);
            ImGui::Text("Pixel Pos Mouse: %f", mInputData.MousePos.x);
            ImGui::Text("Pixel Pos Current Mouse Timestamp: %f", pixelPositionTimestamp);
        }
    
        ImGui::SliderFloat("Scale", &mZoomLerpTarget, 1.f, 80.f);
        ImGui::InputFloat("Mouse Scroll Speed", &mInputData.ScrollSpeed);
        ImGui::InputFloat("Edge Grwoth Speed", &mEdgeMoveSpeed);

        ImGui::Text("Visible Frame Count: %d", mVisibleFrameCount);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Display Style")) {
        ImGui::Text("Timeline Style:");
        ImGui::DragInt("Header height", &mStyle.HeaderHeight, 1.f, 10.f, 300.f);
        ImGui::DragFloat("Legend Width", &mStyle.LegendWidth, 1.f, 10.f, 300.f);
        ImGui::DragInt("Scrollbar Thickness", &mStyle.ScrollbarThickness, 1.f, 10.f, 300.f);

        ImTimelineUtility::DebugColor("Header Background Color", mStyle.HeaderBackgroundColor);
        ImTimelineUtility::DebugColor("Header Timestamp Color", mStyle.HeaderTimeStampColor);

        ImGui::Checkbox("HasScrollbar", &mStyle.HasScrollbar);
        ImGui::Checkbox("HasSeekbar", &mStyle.HasSeekbar);
        ImGui::Checkbox("IsMovingScrollBar", &mInputData.IsMovingScrollBar);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Add/Delete Test")) {
        static s32 toAdd_cat = 0;
        static s32 toAdd_start = 0;
        static s32 toAdd_end = 0;

        ImGui::Text("New:");
        ImGui::InputInt("Section:", &toAdd_cat);
        static char item_label_str[128] = "New Item";
        ImGui::InputText("Text:", item_label_str, IM_ARRAYSIZE(item_label_str));
        ImGui::InputInt("Start:", &toAdd_start);
        ImGui::InputInt("End:", &toAdd_end);

        if (ImGui::Button("Add new:")) {
            AddNewNode(toAdd_cat, toAdd_start, toAdd_end, item_label_str);
        }

        ImGui::Text("Command Index: %d", mCommandIndex);

        ImGui::SameLine();

        if (ImGui::Button("Delete Selected")) {
            DeleteSelection();
        }

        ImGui::SameLine();

        ImGui::SameLine();

        if (ImGui::Button("Delete Range")) {
            DeleteItem(toAdd_cat, toAdd_start, toAdd_end);
        }

        ImGui::SameLine();

        if (ImGui::Button("Delete Section")) {
            DeleteSection(toAdd_cat);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Behavior Flags")) {

        ImGui::Text("Default Node Flags:");
        ::ImTimelineInternal::ShowTimelineNodeFlagsDebugUI(&mEmptyDummyNode);
        ImGui::Text("Timeline Flags:");

        bool bSkipRebuild = mFlags.test(TimelineFlags_SkipTimelineRebuild);

        if (ImGui::Checkbox("Skip Rebuild", &bSkipRebuild)) {
            mFlags.set(TimelineFlags_SkipTimelineRebuild, bSkipRebuild);
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Performance")) {
        OnDebugGUIPerformance();
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Other")) {
        ImGui::Text("Licenses:");
        ImGui::Text("Nameof");

        if (ImGui::Button("Show License: Nameof"))
            ImGui::OpenPopup("NameofLicense");
        if (ImGui::BeginPopup("NameofLicense")) {
            ImGui::Text(ImTimelineLicense::LicenseNameof);
            ImGui::EndPopup();
        }

        ImGui::Text("Version %s", IMTIMELINE_VERSION_STR);

        ImGui::TreePop();
    }

    if (ImGui::Button("Undo")) {
        Undo();
    }

    ImGui::SameLine();

    if (ImGui::Button("Redo")) {
        Redo();
    }

    ImGui::PopItemWidth();
    ImGui::EndChild();

}

void Timeline::OnDebugGUITimelineList()
{
    ImGui::Text("Timeline Display Properties:");

    if (ImGui::BeginTabBar("TimelineSectionTab") == false)
        return;

    for (auto& timeline : mTimelines) {
        if (ImGui::BeginTabItem(timeline.second.mProps.mSectionName.c_str())) {

            if (ImGui::Button("Rebuild")) {
                NodeInitDescriptor descriptor;
                descriptor.start = 0;
                timeline.second.mNodeData->rebuild(descriptor);
            }

            ImGui::Indent(25.f);
            OnDebugGuiDisplayProps(timeline.second.mProps.mDisplayProperties);
            ImGui::Indent(-25.f);

            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();
}

void Timeline::OnDebugGuiDisplayProps(sGenericDisplayProperties& displayPropsRef)
{
    ImGui::PushItemWidth(80);
    ImGui::DragFloat("Height: ", &displayPropsRef.mHeight);
    ImGui::DragFloat("Width: ", &displayPropsRef.mWidth);
    ImGui::DragFloat("BorderRadius: ", &displayPropsRef.BorderRadius);

    ImGui::DragInt("Accent Thickness: ", &displayPropsRef.AccentThickness);

    ImTimelineUtility::DebugColor("Background Color", displayPropsRef.mBackgroundColor);
    ImTimelineUtility::DebugColor("Background Color 2", displayPropsRef.mBackgroundColorTwo);
    ImTimelineUtility::DebugColor("Foreground Color", displayPropsRef.mForegroundColor);

    ImGui::PopItemWidth();
}

void Timeline::OnDebugGUIPerformance()
{
    ImGui::Text("Performance:");
    ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);

    for (auto& timeline : mTimelines) {
        ImGui::Text("[%s]", timeline.second.mProps.mSectionName.c_str());
        timeline.second.mNodeData->PerformanceDebugUI();
    }

    ImGui::Text("Performance Timers:");
    ScopedTimer::DebugPrint();

    ImGui::Text("NodeView Performance:");

    for (auto& timeline : mTimelines) {
        ImGui::Text("[%s]", timeline.second.mProps.mSectionName.c_str());
        // if(timeline.second.mNodeView)
        // timeline.second.mNodeView->PerformanceDebugUI();
    }

    static s32 toAdd_number = 100;
    static s32 toAdd_SectionIndex = 1;
    static s32 add_start = 3;
    static s32 count = 0;

    ImGui::Text("New:");
    ImGui::InputInt("Add Count:", &toAdd_number);
    ImGui::InputInt("Add Section:", &toAdd_SectionIndex);
    ImGui::Text("Total Add count: %d", count);
    ImGui::InputInt("Add start position:", &add_start);

    if (ImGui::Button("Add bulk:")) {
        for (s32 i = 0; i < toAdd_number; ++i) {
            s32 width = Random::RandomIntRange(1, 4);
            AddNewNode(toAdd_SectionIndex, add_start, add_start + width, "New Item");

            add_start += width + Random::RandomIntRange(1, 3);
            ++count;
        }
    }
}

void Timeline::OnDebugGUIPlayer()
{
    if (mMainPlayer.get() == nullptr) {
        return;
    }
    mMainPlayer->OnDebugGUI();

    if (ImGui::TreeNodeEx("Custom TimelinePlayer View UI")) {
        if (mMainPlayer->GetViewUI()) {
            mMainPlayer->GetViewUI()->Draw();
        } else {
            ImGui::Text("No Custom View specified");
        }
        ImGui::TreePop();
    }
}

void Timeline::OnDebugGUILog()
{
    ImTimelineLog::getInstance().OnDebugGUI();
}

bool Timeline::OnDebugGUISelection()
{
    ImGui::Text("Selected: Node Properties:");

    if (mSelectedNode == nullptr)
        return false;

    ImGui::Text("ID: %d", mSelectedNode->ID);
    ImGui::Text("Section: %d", mSelectedNode->section);
    ImGui::Text("Text: %s", mSelectedNode->displayText.c_str());
    ImGui::DragInt("Start", &mSelectedNode->start);
    ImGui::DragInt("End", &mSelectedNode->end);

    OnDebugGuiDisplayProps(mSelectedNode->displayProperties);

    ImGui::Text("Flags:");

    ::ImTimelineInternal::ShowTimelineNodeFlagsDebugUI(mSelectedNode);

    if (mSelectedNode->CustomNode) {
        ImGui::Text("Custom Debug:");
        mSelectedNode->CustomNode->OnDebugGUI();
    }

    return true;
}

void Timeline::OnDebugGUIRightSidePane()
{
    if (ImGui::BeginTabBar("RightSidePaneTabs") == false)
        return;

    if (ImGui::BeginTabItem("Selected Node")) {
        OnDebugGUISelection();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Debug ImTimelineLog")) {
        OnDebugGUILog();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Timeline Player")) {
        OnDebugGUIPlayer();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Timeline Display Properties")) {
        OnDebugGUITimelineList();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}

f32 Timeline::getSeekbarPositionX()
{
    if (mMainPlayer.get() == nullptr) {
        return 0.0f;
    }
    f32 timestamp_current = mMainPlayer->GetCurrentTimestamp();
    f32 base = mContentAreaRect.Min.x;
    f32 x = base + mStyle.LegendWidth + (timestamp_current - mStartFrame) * mZoom + mZoom / 2;
    return x;
}
void Timeline::drawSeekbarUI()
{
    if (mMainPlayer.get() == nullptr) {
        return;
    }
    f32 current_timestamp = mMainPlayer->GetCurrentTimestamp();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();

    f32 x = getSeekbarPositionX();
    f32 y = mContentAreaRect.Min.y;

    f32 x2 = x + mStyle.SeekbarWidth;
    f32 y2 = mContentAreaRect.Max.y;
    y2 = canvas_pos.y + canvas_size.y;

    // don't draw the seekbar when it goes out of view (when the start position shifts)
    if (current_timestamp >= mStartFrame && current_timestamp <= mFrameMax) {
        draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x2, y2), mStyle.SeekbarColor, 0);
    }

    // timestamp text
    if (current_timestamp >= mStartFrame && current_timestamp <= mFrameMax) {
        std::string seekbarLabel;
        ImTimelineUtility::sprint_f(seekbarLabel, "%d", static_cast<s32>(current_timestamp));
        draw_list->AddText(ImVec2(x + 10, y + 2), mStyle.SeekbarColor, seekbarLabel.c_str());
    }
}
s32 Timeline::GetTimestampAtPixelPosition(f32 pixelPos)
{
    const ImVec2 windowMin = ImGui::GetWindowPos();
    f32 pixelPosRel = pixelPos - mStyle.LegendWidth;
    f32 start = pixelPosRel - windowMin.x;
    return (start / mZoom) + mStartFrame;
}
s32 Timeline::GetPixelPositionAtTimestamp(s32 timestamp)
{
    return (timestamp - mStartFrame) * mZoom;
}

void Timeline::updateTimelinePlayer(f32 deltaTime)
{
    for (auto& timeline : mTimelines) {
        auto player = timeline.second.mTimelinePlayer;
        if (player == nullptr)
            continue;
        player->Update(deltaTime);
    }

    if (mMainPlayer && mMainPlayer->IsPlaying()) {
        mMainPlayer->Update(deltaTime);
    }
}

void Timeline::forceRebuild(s32 section, NodeInitDescriptor descriptor)
{
    if (mFlags.test(TimelineFlags_SkipTimelineRebuild))
        return;

    if (mTimelines.find(section) == mTimelines.end())
        return;

    if (mTimelines[section].mNodeData == nullptr)
        return;

    mTimelines[section].mNodeData->rebuild(descriptor);
}

void Timeline::CollectInputData(sInputData& a_outInputData, f32 aDeltaTime)
{
    bool bMouseDownLastFrame = mInputData.LeftMouseDown;

    ImGuiIO& io = ImGui::GetIO();
    mInputData.MousePos = io.MousePos;
    mInputData.LeftMouseDown = io.MouseDown[0];
    mInputData.RightMouseDown = io.MouseDown[1];
    mInputData.MouseScrollVertical = io.MouseWheel;
    // mInputData.MouseDownDuration += io.MouseDownDuration[0];

    if (io.KeysDown[ImGuiKey_Delete] || io.KeysDown[ImGuiKey_Backspace]) {
        // DeleteSelection();
        mNextActionFlags.set(eNextAction::ActionDelete);
    }

    if (io.KeysDown[ImGuiKey_LeftCtrl] & io.KeysDown[ImGuiKey_Z]) {
        // DeleteSelection();
        mNextActionFlags.set(eNextAction::ActionUndo);
    }

    // TODO Redo: Mac Command + Shift + Z. Windows: Ctrl + Z
    if (mNextActionFlags.test(eNextAction::ActionDelete)) {
        DeleteSelection();
    }

    if (mNextActionFlags.test(eNextAction::ActionUndo)) {
        Undo();
    }

    if (mNextActionFlags.test(eNextAction::ActionRedo)) {
        Redo();
    }

    mNextActionFlags.reset();

    if (mInputData.MouseDownDurationLastFrame > 0)
        mInputData.MouseDownDurationLastFrame = 0;

    if (mInputData.LeftMouseDown) {
        mInputData.MouseDownDuration++;
    }

    if (bMouseDownLastFrame && mInputData.LeftMouseDown == false) {
        // mouse release
        mInputData.MouseDownDurationLastFrame = mInputData.MouseDownDuration;
        mInputData.MouseDownDuration = 0;
    }
}

void Timeline::PushCommand(std::unique_ptr<BaseCommand> command)
{
    if (mEnableCommands == false)
        return;
    if (mCommandIndex < static_cast<int>(mCommandHistory.size()) - 1) {
        mCommandHistory.erase(mCommandHistory.begin() + mCommandIndex + 1, mCommandHistory.end());
    }
    mCommandHistory.push_back(std::move(command));
    mCommandIndex++;
}

//auto move when dragging a node to the side
void Timeline::updateSideDragLogic(f32 deltaTime)
{
    s32 timestampMouse = GetTimestampAtPixelPosition(mInputData.MousePos.x);
    s32 timestampRightEdge = mStartFrame + mVisibleFrameCount;
    s32 timestampLeftEdge = mStartFrame;
    s32 autoMoveThreshold = 10;

    s32 moveDirection = 0;

    if (timestampMouse > timestampRightEdge - autoMoveThreshold)
    {
        moveDirection = 1;
    }

     if (timestampMouse <= timestampLeftEdge + autoMoveThreshold)
    {
        moveDirection = -1;
    }

    if(moveDirection != 0)
    {
        mEdgeMoveAmount += mEdgeMoveSpeed * deltaTime;

        if(mEdgeMoveAmount > 1)
        {
            mStartFrame += 1 * moveDirection;
            mEdgeMoveAmount = 0;
        }
    }
}
}