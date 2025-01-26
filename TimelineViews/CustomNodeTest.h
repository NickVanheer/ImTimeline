
#include "../TimelineCore/TimelineDefines.h"
#include "../Core/ImTimelineLog.h"

class CustomNodeTest : public CustomNodeBase
{
public:
    CustomNodeTest() : CustomNodeBase() {}

    void OnDraw(const TimelineNode &nodeData, ImRect drawArea, bool& refIsSelected) override
    {
        ImU32 foregroundColor = IM_COL32(40, 40, 40, 255);
        f32 borderRadius = 14.0f;
        ImVec2 areaBot = drawArea.Max;
        areaBot.y -= 30.0f;
        ImGui::GetWindowDrawList()->AddRectFilled(drawArea.Min, areaBot, mBackgroundColor, borderRadius, 0);
    
        if(refIsSelected)
        {
            f32 borderThickness = 2.0f;
            ImGui::GetWindowDrawList()->AddRect(drawArea.Min, areaBot - ImVec2(borderThickness, borderThickness), foregroundColor, borderRadius, 0, borderThickness);
        }
    
        areaBot.x = drawArea.Min.x;
        areaBot.y += 10;
        ImGui::SetCursorScreenPos(areaBot);
        if(ImGui::Button("Make Coffee"))
        {
            mBackgroundColor = IM_COL32(150, 75, 0, 255);
        }
    }

    void OnDebugGUI() override
    {
        ImGui::Text("Custom Debug");
    }

    void OnNodeActivate() override
    {
        LOG_INFO("Custom Node Played");
    }

    private:
        ImU32 mBackgroundColor = IM_COL32(20, 160, 20, 255);
};