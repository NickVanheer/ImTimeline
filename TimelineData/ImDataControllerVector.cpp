#include "ImDataControllerVector.h"
#include "../Core/ImTimelineLog.h"
#include <algorithm>

void VectorContainer::iterate(const std::function<void(TimelineNode&)>& func)
{
    for (TimelineNode& element : mContainer) {
        func(element);
    }
}

void VectorContainer::PerformanceDebugUI() const
{
    size_t totalSizeBytes = 0;
    size_t sizeNode = sizeof(TimelineNode);

    size_t nodeCount = 0;
    size_t nodeCountAllocated = 0;

    size_t maxCapacity = mContainer.capacity();
    size_t vectorSize = sizeof(mContainer);
    size_t totalSize = vectorSize + (sizeNode * maxCapacity);

    nodeCount += mContainer.size();
    nodeCountAllocated += maxCapacity;

    totalSizeBytes += totalSize;

    double sizeInKB = static_cast<double>(totalSizeBytes) / 1024;

    ImGui::Text("Timeline Data Size: %.2f KB", sizeInKB);
    ImGui::SameLine();
    ImGui::Text("Allocated Node Count: %d", (s32)nodeCountAllocated);
    ImGui::SameLine();
    ImGui::Text("Displayed Node Count: %d", (s32)nodeCount);
}

VectorContainer::~VectorContainer()
{
    mContainer.clear();
}

int VectorContainer::fix_overlap(const NodeInitDescriptor& notused)
{
    auto start = mContainer.begin();
    auto end = mContainer.end();
    std::sort(start, end, [](const TimelineNode& a, const TimelineNode& b) { return a.start < b.start; });

    for (auto it = mContainer.begin(); it != mContainer.end();) {
        if (it->start < 0) {
            int offsetFrom0 = 0 - it->start;
            it->start = 0;
            it->end += offsetFrom0;
        }

        int endPrevious = it->end;

        ++it;

        if (it != mContainer.end() && it->start < endPrevious) {
            int duration = it->end - it->start;

            auto newNode = it;
            newNode->start = endPrevious + 1;
            newNode->end = it->start + duration;
        }
    }

    return 0;
}

TimelineNode& VectorContainer::emplace_back_direct(TimelineNode& newElement, const NodeInitDescriptor& descriptor /* = NodeInitDescriptor() */)
{
     if (mContainer.size() >= mContainer.capacity()) {
        IM_ASSERT(false);
        // mContainer.reserve(mContainer.size() + 1);
    }

    TimelineNode* lastInsertedNode = nullptr;

    // empty or back
    if (mContainer.size() == 0 || mContainer.back().start < newElement.start) {
        auto& newData = mContainer.emplace_back(newElement);
        lastInsertedNode = &newData;
        // auto* ptr = std::addressof(mContainer.emplace_back(newElement));
    }

    // front insert
    if (lastInsertedNode == nullptr && mContainer.begin()->start > newElement.start) {
        mContainer.insert(mContainer.begin(), newElement);
        lastInsertedNode = &*mContainer.begin();
    }

    // middle insert
    if (lastInsertedNode == nullptr) {
        auto itInsert = mContainer.begin();

        for (auto it = mContainer.begin(); it != mContainer.end(); ++it) {
            if (it->start < newElement.start) {
                itInsert = it;
            } else {
                break;
            }
        }

        if (mContainer.size() > 1 && itInsert != mContainer.end()) {
            itInsert++;
            IM_ASSERT(itInsert != mContainer.end()); // there should be a next node
        }

        IM_ASSERT(mContainer.capacity() > mContainer.size());
        mContainer.insert(itInsert, newElement);
        lastInsertedNode = &*itInsert; // TODO there's nothing wrong with it but I dont like this
    }

    IM_ASSERT(lastInsertedNode != nullptr);

    if (descriptor.bMoveOverlappingNext) 
    {
        NodeInitDescriptor descriptor;
        descriptor.start = 0;
        s32 result = fix_overlap(descriptor);
    }

    LOG_INFO_PRINTF("Emplaced node ID %d in section %d (start %d)", (s32)lastInsertedNode->GetID(), lastInsertedNode->GetSection(), lastInsertedNode->start);

    return *lastInsertedNode;

    // MEMO: Iterator might become invalidated. As long as the vector is sized, it should be good
    // It would be safer to research the vector for the last inserted element and return that.
}

int VectorContainer::rebuild(const NodeInitDescriptor& descriptor)
{
    return fix_overlap(descriptor);
}

int VectorContainer::delete_node(const NodeInitDescriptor& descriptor)
{
    int start = descriptor.start;
    int end = descriptor.end;

    int deleteCount = 0;

    for (auto it = mContainer.begin(); it != mContainer.end();) {
        if (it->start >= start && it->end <= end) {
            LOG_INFO_PRINTF("Deleted node ID %d in section %d (start %d)", (s32)it->GetID(), it->GetSection(), it->start);
            it = mContainer.erase(it);
            deleteCount++;
        } else {
            ++it;
        }
    }

    if (deleteCount == 0) {
        // TODO rare chance that descriptor.section is not the actual section ID of this container?
        LOG_INFO_PRINTF("Trying to delete a node in section %d but no node was deleted...", descriptor.section);
    }

    return deleteCount;
}

TimelineNode* VectorContainer::get_node_id(const NodeInitDescriptor& descriptor)
{
    for (auto it = mContainer.begin(); it != mContainer.end(); ++it) {
        if (it->GetID() == descriptor.ID) {
            TimelineNode* node = &*it;
            return node;
        }
    }

    return nullptr;
}

std::vector<TimelineNode*> VectorContainer::get_node_range(const NodeInitDescriptor& descriptor)
{
    std::vector<TimelineNode*> nodes;

    for (auto it = mContainer.begin(); it != mContainer.end(); ++it) {
        if (it->start >= descriptor.start && it->end <= descriptor.end) {
            TimelineNode* node = &*it;
            nodes.push_back(node);
        }
    }

    return nodes;
}
