#pragma once
#include "../TimelineCore/TimelineDefines.h"

class ITimelinePlayerView
{
    
public:
    ITimelinePlayerView() {}
    virtual void SetTimelinePlayer(ImTimeline::TimelinePlayer* aPlayer) { mPlayer = aPlayer; }
    virtual void OnTimelinePlayStart(const sNodePlayProperties& properties = sNodePlayProperties()) = 0; // timeline play
    virtual void OnNodeActivate(TimelineNode* node, const sNodePlayProperties& properties = sNodePlayProperties()) = 0; // node play
    virtual void OnNodeDeactivate(TimelineNode* node, const sNodePlayProperties& properties = sNodePlayProperties()) = 0; // node stop play
    virtual void Draw() = 0;
    virtual void OnFinalize() {}

private:
    ImTimeline::TimelinePlayer* mPlayer = nullptr;
};