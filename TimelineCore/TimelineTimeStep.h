#pragma once
#include "../Core/CoreDefines.h"
#include <chrono>

//TODO use this more for step progression everywhere

namespace ImTimeline
{   
   class TimelineTimeStep
   {
   public:
      TimelineTimeStep() { };

      virtual void Update(f32 aDeltaTime) = 0;
      virtual inline f32 GetTimestamp() const = 0;
   };

    class IntTimelineTimeStep : TimelineTimeStep
   {
   public:
      IntTimelineTimeStep() : mTimestamp(0)
      {
         mLastUpdateTime = std::chrono::steady_clock::now(); 
      }
      IntTimelineTimeStep(s32 start) : mTimestamp(start)
      {

      }

    void Update(f32 aDeltaTime) override
    {
      if(mFirstUpdate == false)
      {
        mFirstUpdate = true;
        mLastUpdateTime = std::chrono::steady_clock::now(); 

      }
            auto currentTime = std::chrono::steady_clock::now(); 
            std::chrono::duration<float> elapsed = currentTime - mLastUpdateTime; 
            
            if (elapsed.count() >= 1.0f) 
            { 
                ++mTimestamp; 
                mLastUpdateTime = currentTime; 
            }
    }
      virtual inline f32 GetTimestamp() const override
      {
        return mTimestamp;
      }

      void SetTimestamp(s32 aTimestamp)
      {
        mTimestamp = aTimestamp;
      }

    private:
      s32 mTimestamp = 0;
      std::chrono::steady_clock::time_point mLastUpdateTime;
      bool mFirstUpdate = false;
   };
}
