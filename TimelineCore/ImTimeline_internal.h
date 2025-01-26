
#include "TimelineDefines.h"

namespace ImTimelineInternal
{
    static constexpr int TIMELINE_RESERVE_NODE_COUNT = 500;
   //command
    class AddCommand : public BaseCommand {
    public:
        AddCommand(ImTimeline::Timeline* aTimeline) : BaseCommand(aTimeline) {}
        virtual void command_do() override;
        virtual void command_undo() override;
        TimelineNode mNewNode;
    };

    class MoveNodeCommand : public BaseCommand {
    public:
        MoveNodeCommand(ImTimeline::Timeline* aTimeline) : BaseCommand(aTimeline) {}
        virtual void command_do() override;
        virtual void command_undo() override;
    
        s32 mNewStart = 0;
        s32 mNewSectionID = -1;
        TimelineNode* mNodeToMove = nullptr;
    };

   class DeleteCommand : public BaseCommand {
    public:
        DeleteCommand(ImTimeline::Timeline* aTimeline) : BaseCommand(aTimeline) {}
        virtual void command_do() override;
        virtual void command_undo() override;
        
        s32 section;
        s32 start;
        s32 end;

        std::vector<TimelineNode> mDeletedNodes;
    };
 

        //ImTimelineUtility
     void ShowTimelineNodeFlagsDebugUI(TimelineNode* node);

    //class ImDataController;

     ImDataController* CreateDefaultDataController();
     std::shared_ptr<INodeView> CreateDefaultNodeView();
     std::shared_ptr<ITimelinePlayerView> CreateDefaultPlayerView();



}