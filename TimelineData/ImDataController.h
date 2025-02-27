#pragma once
#include <iostream>
#include <functional>
#include "../TimelineCore/TimelineDefines.h"

class ImDataController {
public:
    ImDataController() { }
    ImDataController(const ImDataController&) = delete;
    ImDataController(ImDataController&&) = delete;
    ImDataController& operator=(const ImDataController&) = delete;
    ImDataController& operator=(ImDataController&&) = delete;
    virtual ~ImDataController() { OnFinalize();};

    virtual TimelineNode& emplace_back_direct(TimelineNode& newElement, const NodeInitDescriptor& descriptor = NodeInitDescriptor()) = 0;
    virtual int delete_node(const NodeInitDescriptor& descriptor) = 0;
    virtual void iterate(const std::function<void(TimelineNode&)>& func) = 0;
    virtual int rebuild(const NodeInitDescriptor& descriptor) = 0;

    virtual TimelineNode* get_node_id(const NodeInitDescriptor& descriptor) = 0;
    virtual std::vector<TimelineNode*> get_node_range(const NodeInitDescriptor& descriptor) = 0;

    virtual void PerformanceDebugUI() const { }

    virtual void OnFinalize() { }

   
};
