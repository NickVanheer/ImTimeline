#include "DebugPlayerView.h"

void ImTimeline::DebugPlayerView::OnTimelinePlayStart(const sNodePlayProperties& properties)
{
}


void ImTimeline::DebugPlayerView::OnNodeActivate(TimelineNode* node, const sNodePlayProperties& properties)
{
    std::string debugText;
    ImTimelineUtility::sprint_f(debugText, "Play Node: ID %d", node->GetID());
    mDebugTexts.push_back(debugText);
}

void ImTimeline::DebugPlayerView::OnNodeDeactivate(TimelineNode* node, const sNodePlayProperties& properties)
{
    std::string debugText;
    ImTimelineUtility::sprint_f(debugText, "End Play Node: ID %d", node->GetID());
    mDebugTexts.push_back(debugText);
}

void ImTimeline::DebugPlayerView::Draw()
{
    for(const auto& debugString : mDebugTexts)
    {
        ImGui::Text("%s", debugString.c_str());
    }
}
