#include "ImTimeline_internal.h"
#include "../Timeline.h"
#include "../TimelineData/ImDataControllerVector.h"
#include "../TimelineViews/DebugPlayerView.h"
#include "../TimelineViews/HorizontalNodeView.h"
#include "../dependencies/include/nameof/nameof.hpp"

/*******  COMMAND LOGIC *******/

void ImTimelineInternal::AddCommand::command_do()
{
    IM_ASSERT(mTimeline != nullptr);

    if(mTimeline->HasSection(this->mNewNode.GetSection()) == false)
    {
        mTimeline->InitializeTimelineSection(this->mNewNode.GetSection(), "Unnamed");
    }
   
    TimelineNode* newNode = mTimeline->AddNewNode(&mNewNode);

    if(newNode == nullptr)
    {
        LOG_WARNING_PRINTF("Failed to add node %d", this->mNewNode.GetID());
        return;
    }

    // update internal data
    this->mNewNode.start = newNode->start;
    this->mNewNode.end = newNode->end;

    IM_ASSERT(this->mNewNode.GetID() == newNode->GetID()); // specified ID should not change internally

    //add? on delete, the mEndTimestamp should ideally shrink
}
void ImTimelineInternal::AddCommand::command_undo()
{
    IM_ASSERT(mTimeline != nullptr);
    mTimeline->DeleteItem(this->mNewNode.GetSection(), this->mNewNode.start, this->mNewNode.end);
}

//MoveNode

void ImTimelineInternal::MoveNodeCommand::command_do()
{
    IM_ASSERT(mNewSectionID != -1);
    IM_ASSERT(mNodeToMove != nullptr);
    bool bSectionDifferent = mNewSectionID != mNodeToMove->GetSection();
    f32 nodeWith = mNodeToMove->end - mNodeToMove->start;

    s32 oldStart = mNodeToMove->start;
    s32 oldCat = mNodeToMove->GetSection();

    if (bSectionDifferent) {
        AddCommand addCommand(this->mTimeline);
        addCommand.mNewNode = *mNodeToMove; //copy data -> todo assignment operator override or Clone function

        addCommand.mNewNode.section = mNewSectionID;
        addCommand.mNewNode.start = mNewStart;
        addCommand.mNewNode.end = mNewStart + static_cast<s32>(round(nodeWith));
        addCommand.mNewNode.mFlags.set(eTimelineNodeFlags::TimelineNodeFlags_MovedToDifferentTimeline, true);

        addCommand.command_do();

        mTimeline->SetCommandEnable(false);
        mTimeline->DeleteItem(mNodeToMove->section, oldStart, oldStart + nodeWith); // mNodeToMove gets deleted
        mTimeline->SetCommandEnable(true);

        NodeInitDescriptor searchDescriptor;
        searchDescriptor.ID = addCommand.mNewNode.ID;
        //TimelineNode* node = this->mTimeline->mTimelines[addCommand.mNewNode.section].mNodeData->get_node_id(searchDescriptor); //todo remove
        TimelineNode* node = this->mTimeline->FindNodeByNodeID(addCommand.mNewNode.section, addCommand.mNewNode.ID);

        if (node) {
            mNodeToMove = node;
        }
    } else {
        mNodeToMove->start = mNewStart;
        mNodeToMove->end = mNewStart + nodeWith;

        LOG_INFO_PRINTF("Move node on same timeline. ID: %d", mNodeToMove->ID);
    }

    mTimeline->forceRebuild(mNewSectionID);

    // For Undo
    mNewStart = oldStart;
    mNewSectionID = oldCat;
}

void ImTimelineInternal::MoveNodeCommand::command_undo()
{
    command_do();
}

void ImTimelineInternal::DeleteCommand::command_do()
{
    if (mTimeline->HasSection(section) == false) {
        LOG_WARNING_PRINTF("DeleteItem section %d does not exist", section);
        return;
    }
    NodeInitDescriptor descriptor;
    descriptor.start = start;
    descriptor.end = end;

    mDeletedNodes.clear();

    auto nodeList = mTimeline->mTimelines[section].mNodeData->get_node_range(descriptor);
    mDeletedNodes.reserve(nodeList.size());
    for (auto node : nodeList) {
        mDeletedNodes.push_back(*node);
    }

    mTimeline->mTimelines[section].mNodeData->delete_node(descriptor);
}

void ImTimelineInternal::DeleteCommand::command_undo()
{
    for(auto node : mDeletedNodes)
    {
        mTimeline->AddNewNode(&node);
    }

    mDeletedNodes.clear();
}
/****************************/

//utility
void ImTimelineInternal::ShowTimelineNodeFlagsDebugUI(TimelineNode* node)
{
    IM_ASSERT(node != nullptr);
    for(u32 i = 0; i < eTimelineNodeFlags::TimelineNodeFlags_Max; ++i)
    {
        auto flag = static_cast<eTimelineNodeFlags>(i);
        bool bSet = node->mFlags.test(flag);

        std::string name_label = NAMEOF_ENUM(flag).data();
        if (ImGui::Checkbox(name_label.c_str(), &bSet)) {
            node->mFlags.set(flag, bSet);
        }
    }
}

ImDataController* ImTimelineInternal::CreateDefaultDataController()
{
    auto container = new VectorContainer(ImTimelineInternal::TIMELINE_RESERVE_NODE_COUNT);
    return container;
}

std::shared_ptr<INodeView> ImTimelineInternal::CreateDefaultNodeView()
{
    return std::make_shared<HorizontalNodeView>();
}

std::shared_ptr<ITimelinePlayerView> ImTimelineInternal::CreateDefaultPlayerView()
{
    return std::make_shared<ImTimeline::DebugPlayerView>();
}
