#include "ImTimeline.h"
#include "Timeline.h"

namespace ImTimeline
{

void BeginTimeline(const char* str_id, const ImTimelineStyle& style)
{
    auto it = GImmediateData.mTimelineDataMap.find(str_id);
    if (it != GImmediateData.mTimelineDataMap.end()) {
        GImmediateData.mCurrentTimelineData = &it->second;
    } else {
        Timeline* newTimeline = new Timeline();
        GImmediateData.mTimelineDataMap[str_id].mTimelineObject = newTimeline;
        GImmediateData.mTimelineDataMap[str_id].mIDString = str_id;
        GImmediateData.mCurrentTimelineData = &GImmediateData.mTimelineDataMap[str_id];
    }
    GImmediateData.mCurrentTimelineData->mTimelineObject->SetTimelineStyle(style);
    GImmediateData.mTimelineDataMap[str_id].mFlags.set(stImmediateModeTimelineData::eImmediateFlags::BeginTimelineCalled, true);
}

TimelineNode* BeginTimelineContent(NodeID UUID, const std::string& label, s32 section, s32 start, s32 end, std::shared_ptr<CustomNodeBase> customNode)
{
    NodeInitDescriptor descriptor(label, section, start, end, customNode);
    descriptor.ID = UUID;
    return BeginTimelineContent(descriptor);
}


/// @brief Adds a new node to the active timeline, only if the ID hasn't been added before
/// @param nodeDescriptor
/// @return
TimelineNode* BeginTimelineContent(const NodeInitDescriptor& nodeDescriptor)
{
    if (GImmediateData.mCurrentTimelineData == nullptr) {
        BeginTimeline("Timeline");
        return nullptr;
    }

    IM_ASSERT(GImmediateData.mCurrentTimelineData->mTimelineObject != nullptr);

    bool bBeginCalled = GImmediateData.mCurrentTimelineData->mFlags.test(stImmediateModeTimelineData::eImmediateFlags::BeginTimelineCalled);

    if (bBeginCalled == false) {
        ::ImTimeline::BeginTimeline(GImmediateData.mCurrentTimelineData->mIDString.c_str(), GImmediateData.mCurrentTimelineData->mTimelineObject->mStyle);
    }
    auto& mAddedDescriptors = GImmediateData.mCurrentTimelineData->mAddNodeMap;
    if (mAddedDescriptors.find(nodeDescriptor.ID) != mAddedDescriptors.end()) {
        // We've already added this node. It might already been deleted by the user, but regardless never add it again programmatically
        return nullptr;
    }

    mAddedDescriptors.emplace(nodeDescriptor.ID, true);

    TimelineNode newNode;
    newNode.Setup(nodeDescriptor.section, nodeDescriptor.start, nodeDescriptor.end, nodeDescriptor.label);
    bool bShowCustomUI = true;
    newNode.InitalizeCustomNode(nodeDescriptor.customNode, bShowCustomUI);

    // Create new internal node
    TimelineNode* nodeInstance = GImmediateData.mCurrentTimelineData->mTimelineObject->AddNewNode(&newNode);

    return nodeInstance;
}

void ShowTimelineDebugUI(const char* str_id)
{
    if (GImmediateData.mCurrentTimelineData == nullptr)
        return;

    GImmediateData.mCurrentTimelineData->mTimelineObject->DrawDebugGUI();
}

ImTimelineStyle& GetTimelineStyle()
{
    static ImTimelineStyle dummyStyle = ImTimelineStyle();
    if(GImmediateData.mCurrentTimelineData == nullptr)
        return dummyStyle;
    
    IM_ASSERT(GImmediateData.mCurrentTimelineData->mTimelineObject != nullptr);

    return GImmediateData.mCurrentTimelineData->mTimelineObject->mStyle;
}

sGenericDisplayProperties& GetTimelineDisplayProperties(s32 section_id)
{
    static sGenericDisplayProperties dummy = sGenericDisplayProperties();
    if(GImmediateData.mCurrentTimelineData == nullptr)
        return dummy; //NG C++ 17: std::optional<GenericDisplayProperties&>
    
    IM_ASSERT(GImmediateData.mCurrentTimelineData->mTimelineObject != nullptr);

    return GImmediateData.mCurrentTimelineData->mTimelineObject->GetSectionDisplayProperties(section_id);
}

void SetTimelineName(s32 section_id, std::string name)
{
      if(GImmediateData.mCurrentTimelineData == nullptr)
        return;
    
    IM_ASSERT(GImmediateData.mCurrentTimelineData->mTimelineObject != nullptr);

    return GImmediateData.mCurrentTimelineData->mTimelineObject->SetTimelineName(section_id, name);
}

void ShowActiveTimeline()
{
    if (GImmediateData.mCurrentTimelineData == nullptr)
        return;

    if(GImmediateData.mCurrentTimelineData->mTimelineObject)
        GImmediateData.mCurrentTimelineData->mTimelineObject->DrawTimeline();
}

void SetTimelineProperties(s32 section_id, const TimelineSectionProperties& properties)
{
    if (GImmediateData.mCurrentTimelineData == nullptr)
        return;

    GImmediateData.mCurrentTimelineData->mTimelineObject->InitializeTimelineSection(section_id, properties.mSectionName.c_str());
    GImmediateData.mCurrentTimelineData->mTimelineObject->GetTimelineSection(section_id).mProps = properties;
}

} // namespace ImTimeline
