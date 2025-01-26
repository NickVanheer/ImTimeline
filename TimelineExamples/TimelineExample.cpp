#include "TimelineExample.h"
#include "../ImTimeline.h"
#include "../Timeline.h"
#include "../TimelineViews/CustomNodeTest.h"

void ImTimeline::ShowDemoWindow()
{
    enum eExampleType {
        ExampleType_ObjectOriented = 0,
        ExampleType_Immediate,
    };

    static int toggleValue = 0;
    ImGui::RadioButton("Object Oriented", &toggleValue, eExampleType::ExampleType_ObjectOriented);
    ImGui::SameLine();
    ImGui::RadioButton("Immediate (ImGui style)", &toggleValue, eExampleType::ExampleType_Immediate);
    ImGui::SameLine();

    switch (toggleValue) {
    case eExampleType::ExampleType_ObjectOriented: {
        ShowDemoWindowObjectOriented();
    } break;
    case eExampleType::ExampleType_Immediate: {
        ShowDemoWindowImmediate();
    } break;
    default:
        break;
    }
}

void ImTimeline::ShowDemoWindowImmediate()
{
    ImGui::Begin("Timeline Demo Immediate");
    ImTimeline::ShowTimelineDebugUI("Timeline");

    const int CATEGORY_SWEETS_ID = 0;
    const int CATEGORY_DRINKS_ID = 1;
    const int CATEGORY_FOOD_ID = 2;

    ImTimeline::BeginTimeline("Timeline");

    s32 uniqueID = 0;
    ::ImTimeline::BeginTimelineContent(++uniqueID, "Pudding", CATEGORY_SWEETS_ID, 10, 30);
    ImTimeline::BeginTimelineContent(++uniqueID, "Coffee", CATEGORY_DRINKS_ID, 20, 40);
    ImTimeline::BeginTimelineContent(++uniqueID, "Caffee Latte", CATEGORY_DRINKS_ID, 45, 60, std::make_shared<CustomNodeTest>());
    ImTimeline::BeginTimelineContent(++uniqueID, "Pasta", CATEGORY_FOOD_ID, 25, 70);

    // ImTimeline::SetTimelineName(CATEGORY_SWEETS_ID, "Sweets");
    // ImTimeline::SetTimelineName(CATEGORY_DRINKS_ID, "Drinks");
    // ImTimeline::SetTimelineName(CATEGORY_FOOD_ID, "Food");

    auto& displayProps = ImTimeline::GetTimelineDisplayProperties(CATEGORY_DRINKS_ID);
    displayProps.mHeight = 70.0f;

    ImTimeline::ShowActiveTimeline();

    ImGui::End();
}

void ImTimeline::ShowDemoWindowObjectOriented()
{
    static ImTimeline::Timeline mTimeline = ImTimeline::Timeline();
    static bool bInit = false;

    if (bInit == false) {
        const int CATEGORY_SWEETS_ID = 0;
        const int CATEGORY_DRINKS_ID = 1;
        const int CATEGORY_FOOD_ID = 2;

        mTimeline.AddNewNode(CATEGORY_SWEETS_ID, 10, 30, "Pudding");
        mTimeline.AddNewNode(CATEGORY_DRINKS_ID, 20, 40, "Coffee");
        mTimeline.AddNewNode(CATEGORY_DRINKS_ID, 45, 60, "Cafe Latte", std::make_shared<CustomNodeTest>());
        mTimeline.AddNewNode(CATEGORY_FOOD_ID, 25, 70, "Pasta");

        mTimeline.SetTimelineName(CATEGORY_SWEETS_ID, "Sweets");
        mTimeline.SetTimelineName(CATEGORY_FOOD_ID, "Food");
        mTimeline.SetTimelineName(CATEGORY_DRINKS_ID, "Drinks");
        auto& displayProps = mTimeline.GetSectionDisplayProperties(CATEGORY_DRINKS_ID);
        displayProps.mHeight = 70.0f;

        mTimeline.SetMaxFrame(140);

        // mTimeline.SetTimelineHeight(CATEGORY_DRINKS_ID, 70.0f);

        bInit = true;
    }
    ImGui::Begin("Timeline Demo Object Oriented");
    mTimeline.DrawDebugGUI();
    mTimeline.DrawTimeline();
    ImGui::End();
}
