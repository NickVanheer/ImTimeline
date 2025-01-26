
# ImTimeline 

A small, single-include library to draw a customizable timeline inside of an ImGUI window. 

My goal was to create an easy-to-use timeline API that should feel right at home when you know how ImGUI works, and which can be extended and customized to enable more complex scenarios.

## Features:
* Adding, deleting, moving nodes with drag & drop, undo & redo functionality
* Generic Node Playing functionality
* Custom UI for nodes and the timeline UI
* Customizable styles and flags similar to how ImGUI works
* Debug UI and samples to get you started
* Fixed memory data source, with customization as to how data is fetched internally

By default, when adding new items (from hereon: "nodes") to the timeline, they will  be displayed in a horizontal fashion similar to a video editor timeline. Each timeline has a fixed amount of memory that does not change or grow over time. Both data handling and UI is abstracted away through a base class, and can be overwritten with a custom implementation.
The provided default implemetations mimick common applications of a chronological horizontal timeline, such as a video editor or Unreal Engine's Sequencer.

## Getting started
See the TimelineExamples folder for an example.
```cpp
#include "TimelineExamples/TimelineExample.h"
ImTimeline::ShowDemoWindow();
```

### Object-oriented way
In your header

```cpp
#include "TimelineCore/ImTimeline.h"
ImTimeline::Timeline mTimeline;
```

In your initializion function
```cpp
const int CATEGORY_SWEETS_ID = 0;
const int CATEGORY_DRINKS_ID = 1;
const int CATEGORY_FOOD_ID = 2;

mTimeline.AddNewNode(CATEGORY_SWEETS_ID, 10, 30, "Pudding");
mTimeline.AddNewNode(CATEGORY_DRINKS_ID, 20, 40, "Coffee");
mTimeline.AddNewNode(CATEGORY_DRINKS_ID, 45, 60, "Cafe Latte", std::make_shared<CustomNodeTest>());
mTimeline.AddNewNode(CATEGORY_FOOD_ID, 25, 70, "Pasta");
    
auto& displayProps = mTimeline.GetSectionDisplayProperties(CATEGORY_DRINKS_ID);
displayProps.mHeight = 70.0f;
```

In your draw logic
```cpp
mTimeline.DrawDebugGUI();
mTimeline.DrawTimeline();
```


### Immediate way
An ImGUI-style immediate API is also provided in the ImTimeline namespace, mimicking ImGui's Begin APIs.
Unlike ImGUI destroying and recreating everything every frame, in this API we memorize the initially added items through a Unique ID. Because of this, you can freely rearrange nodes during runtime without them reapparing again.

Personally, since the timeline data is usually something that will be used for a long time, I prefer the object-oriented way above, but if you really want, you can also do it like this:

```cpp
ImTimeline::BeginTimeline("Timeline");

s32 uniqueID = 0;
ImTimeline::BeginTimelineContent(++uniqueID, "Pudding", CATEGORY_SWEETS_ID, 10, 30);
ImTimeline::BeginTimelineContent(++uniqueID, "Coffee", CATEGORY_DRINKS_ID, 20, 40);
ImTimeline::BeginTimelineContent(++uniqueID, "Caffee Latte", CATEGORY_DRINKS_ID, 45, 60, std::make_shared<CustomNodeTest>());
ImTimeline::BeginTimelineContent(++uniqueID, "Pasta", CATEGORY_FOOD_ID, 25, 70);


auto& displayProps = ImTimeline::GetTimelineDisplayProperties(CATEGORY_DRINKS_ID);
displayProps.mHeight = 70.0f;

ImTimeline::ShowActiveTimeline();
```



## Terminology

| Name | Usage | 
|----------|----------|
| Timeline | A collection of Timeline sections as well as the basic functionality to manipulate those sections  |
| TimelineSection | A single timeline that has its own UI, data container, properties |
| TimelineNode | An item placed on a TimelineSection. Edits directly affect what's drawn on screen |
| ImDataController | The data container that hosts and iterates through all the nodes  |
| NodeInitDescriptor | Metadata that is used to manipulate nodes in the ImDataController. For example, retrieving multiple nodes inbetween a certain range |
| INodeView | UI logic that uses an ImDataController to draw nodes on the screen
| ITimelinePlayerView | Playing logic that uses an ImDataController to "execute" nodes, think of this as the play logic in a video player application |

## Customization & Interfaces
Both the way data is handled, the node UI and individual nodes can be customized through the use of polymorphism: 

| Usage | Base Class | Example Implementation | 
|----------|----------|----------| 
| Data container | ImDataController | ImDataControllerVector.h |
| Timeline and default node UI | INodeView | HorizontalNodeView.h | 
| Custom Node UI | CustomNodeBase | CustomNodeTest.h |
| Timeline Player UI | ITimelinePlayerView | DebugPlayerView.h |

### Shortcomings
* Each timeline can be individually modified, but the outer container or header cannot be modified yet, limiting the customization options
* No save/load serialization
* Memory for each timeline is fixed and allocated when a new timeline is created for the first time. There is a limit of ImTimeline::TIMELINE_RESERVE_NODE_COUNT for each timeline
* No navigation polish features such as: node edge drag node resize, mouse zoom or mutli-node select support.
* The timestamp can't be customized fully yet and is limited to a float value

### Sidenotes

**Sidenote:** This "I'll do this in a week" small side project grew in size and complexity as it went along and sort of consumed me. I'm not entirely satisfied with everything, and some of the customization and bells & whiskers are still lacking. But it works, so I just throw it out here so I can move on with my life and hopefully occasionally update this.

**Sidenote:**  This is my first time writing a C++ API and it's somewhat different from my day-to-day job (UI/tool engineer at a game development studio) so please go easy on me.

**Sidenote:** I wrote this on a MacBook while sitting in many different cafes in Tokyo, so depending on the coffee quality there might be a momentarily dip in code quality. 

**Sidenote:** Due to the MacBook & VSCode/Clang nature and the lunatic default formatting of the VSCode Format option, some things might seem off coming from a Visual Studio background. I'll try to clean up things and provide a Visual Studio project in the future. Not even at gunpoint I would write function signatures like in this project, but the autoformatting just did it and I'm too lazy to fix it every single time. 

### License
* This project is licensed under the MIT license and can be used free of charge for commercial and non-commercial use. 
* This project, as of the time of writing contains copies of the Nameof library for debug purposes. The library is equally licensed under MIT. Its license can be displayed through the debug UI

### Other
* Version: 0.2.0 WIP
* Requires C++ 17
* Made on top of ImGUI v1.91.0, but it's likely compatible with older ImGUI versions too.
* Compatibility tested under Mac OSX, Windows 10
* January 2025
* Dev Blog: https://immersivenick.wordpress.com/ 


Blog post stuff:

- Things I dont like about ImGui:
* Immediate so data that could get cached gets recreated every frame
* All source is in one giant header
* There is a split between public and internal source, but the end user always sees both when using intellisense/code-completion, as the logic of hoth files reside in the same namespacee