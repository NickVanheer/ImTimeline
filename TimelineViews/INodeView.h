#pragma once
#include <iostream>
#include "../TimelineCore/TimelineDefines.h"

namespace ImTimeline
{
    class ImTimeline;
}

class INodeView {
public:
    INodeView() { }
    INodeView(const INodeView&) = delete;
    INodeView(INodeView&&) = delete;
    INodeView& operator=(const INodeView&) = delete;
    INodeView& operator=(INodeView&&) = delete;
    virtual ~INodeView() { OnFinalize();};

    virtual void PreDraw() { }
    virtual void OnFinalize() { }

    virtual void DrawNodeView(const ImRect &area, const sTimelineSection& timeline, ImTimeline::Timeline* context) { };
    virtual void defaultNodeDraw(const ImRect &area, const TimelineNode& node, ImTimeline::Timeline* context) { };

    virtual void PerformanceDebugUI() const { }
};