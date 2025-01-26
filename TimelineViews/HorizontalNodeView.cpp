#include "HorizontalNodeView.h"

#include "../Core/CoreDefines.h"
#include "../Core/ImTimelineUtility.h"
#include "../TimelineData/ImDataController.h"
#include "../Timeline.h"


void HorizontalNodeView::PreDraw() 
{
    mNodesDrawSkipped = 0;
}

void HorizontalNodeView::DrawNodeView(const ImRect &area, const sTimelineSection& timeline, ImTimeline::Timeline* pContext)
{
    // ImTimeline::Timeline& timeline = *pContext;
    if (timeline.mbIsInitialized == false) {
        return;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_size = pContext->mContentAreaRect.GetSize();

    ImVec2 contentMin = pContext->mContentAreaRect.Min;

    IM_ASSERT(timeline.mNodeData != nullptr);

    // background

    auto bgColorFullBar = IM_COL32(40, 50, 50, 255);

    if (area.Contains(pContext->GetLastInputData().MousePos)) {
        bgColorFullBar += 0x80201008;
        pContext->SetSelectedTimeline(timeline.mID);
    }

    draw_list->AddRectFilled(area.Min, area.Max, bgColorFullBar, 0);

    DrawLegendArea(timeline, pContext, area);

    ImRect timelinePanelRect = ImRect(area.Min, area.Max);
    timelinePanelRect.Min.x += pContext->mStyle.LegendWidth - (pContext->GetStartTimestamp() * pContext->GetScale());
    timelinePanelRect.Max.x -= pContext->mStyle.LegendWidth - (pContext->GetStartTimestamp() * pContext->GetScale());

    // timelinePanelRect.Min.y += (mStartFrameVertical * mZoom);
    // timelinePanelRect.Max.y -= (mStartFrameVertical * mZoom);

    // TODO VERTICAL SCROLL

    ImRect timelinePanelRectAbsolute = ImRect(area.Min, area.Max);
    timelinePanelRectAbsolute.Min.x += pContext->mStyle.LegendWidth;

    //+ mStyle.LegendWidth - (m_FirstFrame * mZoom)

    draw_list->PushClipRect(timelinePanelRectAbsolute.Min, timelinePanelRectAbsolute.Max, true);

    IM_ASSERT(timeline.mNodeData != nullptr);

    // foreground
    auto& item_list = timeline.mNodeData;
    size_t sectionHeight = timeline.mProps.mDisplayProperties.mHeight;

    s32 index = 0;

    item_list->iterate([&](TimelineNode& node) {
        index++;
        if (node.mFlags.test(eTimelineNodeFlags::TimelineNodeFlags_AutofitHeight) == false) {
            sectionHeight = node.displayProperties.mHeight;
        }

        if (pContext->IsDragging() && pContext->mDragData.DragNode.GetID() == node.GetID()) {
            return;
        }

        s32 scale = pContext->GetScale();

        ImVec2 slotP1(timelinePanelRect.Min.x + node.start * scale, timelinePanelRect.Min.y);
        ImVec2 slotP2(timelinePanelRect.Min.x + node.end * scale + scale, slotP1.y + sectionHeight - node.displayProperties.AccentThickness);

        ImRect nodeRect = ImRect(slotP1, slotP2);

        bool canDraw = slotP1.x <= (canvas_size.x + contentMin.x) && slotP2.x >= (contentMin.x + pContext->mStyle.LegendWidth);

        if (!canDraw) {
            mNodesDrawSkipped++;
            return;
        }

        defaultNodeDraw(nodeRect, node, pContext);

#if defined IM_TIMELINE_DEBUG_INFO
        std::string nodeDebugText = "";
        ImTimelineUtility::sprint_f(nodeDebugText, "id: %d - index: %d", node.ID, index);
        draw_list->AddText(nodeRect.Min + ImVec2(10, 20), node.displayProperties.mForegroundColor, nodeDebugText.c_str());
#endif
        bool bIsSelected = ImRect(slotP1, slotP2).Contains(pContext->GetLastInputData().MousePos) && pContext->GetLastInputData().LeftMouseDown;
        bool bInputDelay = (pContext->GetLastInputData().MouseDownDuration > 40.0f);

        if (bIsSelected && pContext->mDragData.DragState == eDragState::None) {
            pContext->SelectNode(&node);
        }

        if (bIsSelected && pContext->IsDragging() == false && bInputDelay) {
            pContext->mDragData.DragState = eDragState::DragNode;
            pContext->mDragData.DragNode = *pContext->GetSelectedNode();
            pContext->mDragData.DragStartMouseDelta = pContext->GetLastInputData().MousePos - slotP1;
            pContext->mDragData.DragRect = nodeRect;
        }
    });

    draw_list->PopClipRect();

    if (pContext->IsDragging() && pContext->GetSelectedSection() == timeline.mID) {
        draw_list->AddRect(area.Min, area.Max, ImTimelineUtility::Color::Yellow, 0);
    }
}

void HorizontalNodeView::defaultNodeDraw(const ImRect& area, const TimelineNode& node, ImTimeline::Timeline* timeline)
{
    bool bSelected = timeline->GetSelectedNode() == &node;

    // Draw Other ImGUI elements starting from here
    ImGui::SetCursorScreenPos(area.Min);

    if (node.GetCustomNode() && node.mFlags.test(eTimelineNodeFlags::TimelineNodeFlags_CustomDraw)) {
        node.GetCustomNode()->OnDraw(node, area, bSelected);
        return;
    }

    ImU32 bgColor = node.displayProperties.mBackgroundColor;
    ImU32 bgColor2 = node.displayProperties.mBackgroundColorTwo;

    if (node.mFlags.test(eTimelineNodeFlags::TimelineNodeFlags_UseSectionBackground)) {
        bgColor = timeline->GetSectionDisplayProperties(node.GetSection()).mBackgroundColor;
        bgColor2 = timeline->GetSectionDisplayProperties(node.GetSection()).mBackgroundColorTwo;
    }
    if (area.Contains(timeline->GetLastInputData().MousePos)) {
        bgColor += 0x00201000;
    }

    ImU32 gradientStart = bgColor;
    ImU32 gradientEnd = bgColor + 0xFF402000;


    f32 borderRadius = node.displayProperties.BorderRadius;
    ImU32 fgColor = bSelected ? timeline->mStyle.SelectedNodeOutlineColor : node.displayProperties.mForegroundColor;

    auto* draw_list = ImGui::GetWindowDrawList();
    ImRect outlineRect = ImRect(area.Min, area.Max);

    //draw_list->AddRectFilled(outlineRect.Min, outlineRect.Max, bgColor, borderRadius);
    draw_list->AddRectFilledMultiColor(outlineRect.Min, outlineRect.Max, gradientStart, gradientStart, gradientEnd, gradientEnd);
    draw_list->AddText(area.Min + ImVec2(node.displayProperties.AccentThickness, node.displayProperties.AccentThickness), node.displayProperties.mForegroundColor, node.displayText.c_str());

    //TODO: ADD Text Clipping when text goes out of scope
    f32 borderThickness = node.displayProperties.BorderThickness;
    if (bSelected) {
        draw_list->AddRect(outlineRect.Min, outlineRect.Max - ImVec2(borderThickness, borderThickness), timeline->mStyle.SelectedNodeOutlineColor, borderRadius, 0, borderThickness);
    } else {
        draw_list->AddRect(outlineRect.Min, outlineRect.Max - ImVec2(borderThickness, borderThickness), fgColor, borderRadius, 0, borderThickness);
    }
}

void HorizontalNodeView::PerformanceDebugUI() const
{
    ImGui::Text("Nodes Draw Skipped: %d", mNodesDrawSkipped);
}

void HorizontalNodeView::DrawLegendArea(const sTimelineSection& timeline, ImTimeline::Timeline* pContext, const ImRect& area)
{
    std::string label;
    ImTimelineUtility::sprint_f(label, "[%d] (%s)", timeline.mID, timeline.mProps.mSectionName.c_str());

    auto* draw_list = ImGui::GetWindowDrawList();
    ImVec2 tpos(area.Min.x + 3, area.Min.y);
    draw_list->AddText(tpos, 0xFFFFFFFF, label.c_str());

#if defined IM_TIMELINE_DEBUG_INFO
    // BUG improper drawing starting from the second timeline onwards
    if (ImGui::Button("Rebuild")) {
        mTimelines[section].mNodeData->rebuild(NodeInitDescriptor());
    }
#endif
}

// TODO ScrollBar/Header draw functions here
