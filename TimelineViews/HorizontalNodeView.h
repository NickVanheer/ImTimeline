#pragma once
#include "../TimelineViews/INodeView.h"

class HorizontalNodeView : public INodeView 
{
public:
    HorizontalNodeView() { }
    virtual ~HorizontalNodeView() { }
    virtual void PreDraw() override;

    virtual void DrawNodeView(const ImRect &area, const sTimelineSection& timeline, ImTimeline::Timeline* context) override;
    virtual void defaultNodeDraw(const ImRect& area, const TimelineNode& node, ImTimeline::Timeline* timeline) override;
    virtual void PerformanceDebugUI() const override;

    // custom
    virtual void DrawLegendArea(const sTimelineSection& timeline, ImTimeline::Timeline* pContext, const ImRect& area);

private:
    int mNodesDrawSkipped = 0;
};
