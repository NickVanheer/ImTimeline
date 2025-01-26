#include "../TimelineCore/TimelinePlayer.h"

namespace ImTimeline
{
   class DebugPlayerView : public ITimelinePlayerView
   {
   public:
      DebugPlayerView() : ITimelinePlayerView() {}
  
      virtual void OnTimelinePlayStart(const sNodePlayProperties& properties = sNodePlayProperties()); // timeline play
      virtual void OnNodeActivate(TimelineNode* node, const sNodePlayProperties& properties = sNodePlayProperties()); // node play
      virtual void OnNodeDeactivate(TimelineNode* node, const sNodePlayProperties& properties = sNodePlayProperties());// node stop play
      virtual void Draw();

   private:
      std::vector<std::string> mDebugTexts;
      
   };
}