#include "ImDataController.h"
#include <vector>

// template <typename T>
class VectorContainer : public ImDataController {
public:
    VectorContainer(size_t max_capacity)
        : ImDataController()
    {
        mContainer.reserve(max_capacity);
        // static_assert(std::is_base_of<TimelineNode, T>::value, "type parameter of this class must derive from TimelineNode");
    }

    virtual void iterate(const std::function<void(TimelineNode&)>& func) override;
    int fix_overlap(const NodeInitDescriptor& notused);
    TimelineNode& emplace_back_direct(TimelineNode& node, const NodeInitDescriptor& descriptor = NodeInitDescriptor()) override;
    virtual int rebuild(const NodeInitDescriptor& descriptor) override;
    int delete_node(const NodeInitDescriptor& descriptor) override;
    TimelineNode* get_node_id(const NodeInitDescriptor& descriptor) override;
    std::vector<TimelineNode*> get_node_range(const NodeInitDescriptor& descriptor) override;

    virtual void PerformanceDebugUI() const override;

    virtual ~VectorContainer() override;

private:
    std::vector<TimelineNode> mContainer;
};