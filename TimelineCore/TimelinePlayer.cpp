
#include "TimelinePlayer.h"
#include "../Core/ImTimelineLog.h"
#include "../TimelineData/ImDataController.h"

/******
 ImTimeline::TimelinePlayer
 author: nick
 =========================
 - Play nodes in sequence from a single timeline, or multiple child timelines, with state tracking.
  Supports custom UI through the ITimelinePlayerView interface, has no UI by default.
  It's up to the user to provide a play/pause/stop UI controls.

  FIXME: This needs a ready check feature, a continuous node play feature and more event propagation to be more versatile
  FIXME: No hard differentiation between root timeline and child timelines? See IsRootTimeline();
 */

ImTimeline::TimelinePlayer::TimelinePlayer()
{
    mUniqueID = mIDGenerator.GetUniqueID();
}

ImTimeline::TimelinePlayer::~TimelinePlayer()
{
    //mTimelineData should be cleaned up higher-up
}

void ImTimeline::TimelinePlayer::Setup(ImDataController* aTimelineData, s32 aStartTimestamp)
{
    mTimeStep = IntTimelineTimeStep(aStartTimestamp);
    mTimelineData = aTimelineData;
    mPlayingNode = nullptr;
    mbIsInitialized = true;
    ChangeState(eTimelineState::eState_None);
}

bool ImTimeline::TimelinePlayer::IsRootTimeline() const
{
    return mTimelineData == nullptr && mPlayers.size() > 0;
}

void ImTimeline::TimelinePlayer::SetViewUI(std::shared_ptr<ITimelinePlayerView> newView)
{
    mPlayerView = newView;
}

void ImTimeline::TimelinePlayer::Update(f32 aDeltaTime)
{
    if (mState != eTimelineState::eState_Playing)
        return;

    mTimeStep.Update(aDeltaTime);

    bool bChildTimelineFinished = true;

    for (auto ptr_player : mPlayers) {
        auto player = ptr_player;

        if (player == nullptr)
            continue;

        player->Update(aDeltaTime);

        if (player->mState != eTimelineState::eState_Finished)
            bChildTimelineFinished = false;
    }

    // everything finished playing and self has no timeline attached
    if (bChildTimelineFinished && mTimelineData == nullptr) {
        LOG_INFO("All timelines finished Playing");
        mState = eTimelineState::eState_Finished;
    }

    if (mTimelineData == nullptr)
        return;

    if (mPlayingNode == nullptr) {
        mPlayingNode = GetNextNodeToPlay();
        mPlayingNodeProperties.mState = ePlayingNodeState::None;

        if (mPlayingNode == nullptr) {
            mState = eTimelineState::eState_Finished;
            return;
        }
    }

    if (mPlayingNode && mTimeStep.GetTimestamp() >= (f32)mPlayingNode->start) {
        if (mPlayingNodeProperties.mState == ePlayingNodeState::None) {
            // play node
            if (mPlayingNode->GetCustomNode()) {
                mPlayingNode->GetCustomNode()->OnNodeActivate();
            }

            if (mPlayerView) {
                sNodePlayProperties nodePlayProperties; // todo current timestamp and metadata
                mPlayerView->OnNodeActivate(mPlayingNode, nodePlayProperties);
            }

            mPlayingNodeProperties.mState = ePlayingNodeState::IsPlayed;
        }

        if (mPlayingNodeProperties.mState == ePlayingNodeState::IsPlayed && mTimeStep.GetTimestamp() >= (f32)mPlayingNode->end) {
            //end play
            if (mPlayingNode->GetCustomNode()) {
                mPlayingNode->GetCustomNode()->OnNodeDeactivate();
            }

            if (mPlayerView) {
                sNodePlayProperties nodePlayProperties; // todo current timestamp and metadata
                mPlayerView->OnNodeDeactivate(mPlayingNode, nodePlayProperties);
            }

            mPlayingNodeProperties.mState = ePlayingNodeState::IsFinishedPlaying;
            mPlayingNode = nullptr;
        }
    }

    // node play logic
}

void ImTimeline::TimelinePlayer::Play()
{
    if (mbIsInitialized == false) {
        Setup(mTimelineData, mTimeStep.GetTimestamp());
    }

    if (mbIsInitialized == false) {
        LOG_WARNING_PRINTF("TimelinePlayer not initialized", 0);
        return;
    }

    if (mState == eTimelineState::eState_Playing) {
        return;
    }

    f32 timestamp = mTimeStep.GetTimestamp();

    if (mState == eTimelineState::eState_Stopped) {
        timestamp = 0.0f;
    }

    mTimeStep.SetTimestamp(timestamp);
    mState = eTimelineState::eState_Playing;

    for (auto ptr_player : mPlayers) {
        auto player = ptr_player;

        if (player == nullptr)
            continue;
        player->Play();
    }
}

void ImTimeline::TimelinePlayer::Pause()
{
    ChangeState(eTimelineState::eState_Paused);
}

void ImTimeline::TimelinePlayer::Stop()
{
    ChangeState(eTimelineState::eState_Stopped);

    Setup(mTimelineData, 0);
    // TODO fire event?
}

void ImTimeline::TimelinePlayer::SetStartTimestamp(s32 aStartTimestamp)
{
    if (mState == eTimelineState::eState_Playing)
        return;

    // We've clicked somewhere and changed the active timestep, so pause and reset the active timestep

    mTimeStep.SetTimestamp(aStartTimestamp);
    mState = eTimelineState::eState_Paused;
    mPlayingNode = nullptr;

    for (auto ptr_player : mPlayers) {
        auto player = ptr_player;

        if (player == nullptr)
            continue;

        player->SetStartTimestamp(aStartTimestamp);
    }
}

void ImTimeline::TimelinePlayer::DrawPlayer()
{
    if (mPlayerView) {
        mPlayerView->Draw();
    } else {
        ImGui::Text("No Player View Attached:");
    }
}

void ImTimeline::TimelinePlayer::OnDebugGUI()
{
    if (ImGui::TreeNodeEx("Debug State")) {
        ImGui::Text("IsInitialized: %d", mbIsInitialized);
        ImGui::Text("TimelinePlayer Count: %d", (s32)mPlayers.size());
        ImGui::Text("IsRootTimeline: %d", IsRootTimeline());
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        ImGui::Text("Play State:");
        ImGui::Text("- IsNoneState: %d", mState == eTimelineState::eState_None);
        ImGui::Text("- IsStopped: %d", mState == eTimelineState::eState_Stopped);
        ImGui::Text("- IsPaused: %d", mState == eTimelineState::eState_Paused);
        ImGui::Text("- IsPlaying: %d", mState == eTimelineState::eState_Playing);
        ImGui::Text("- IsFinished: %d", mState == eTimelineState::eState_Finished);

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Debug Play UI")) {
        if (mPlayingNode != nullptr) {
            ImGui::Text("Playing Node: %d", mPlayingNode->GetID());
        }

        if (mState == eTimelineState::eState_None || mState == eTimelineState::eState_Stopped) {
            if (ImGui::Button("Play from start")) {
                Play();
            }
        }

        if (mState == eTimelineState::eState_Playing && ImGui::Button("Stop")) {
            Stop();
        }

        if (mState == eTimelineState::eState_Playing && ImGui::Button("Pause")) {
            Pause();
        }

        if (mState == eTimelineState::eState_Paused && ImGui::Button("Play from current frame")) {
            Play();
        }
        ImGui::TreePop();
    }

    ImGui::Text("Current Frame: %d", GetCurrentTimestamp());

    if (mPlayingNode != nullptr) {
        ImGui::Text("Current/Next Playing Node: %d", mPlayingNode->GetID());
    }
    ImGui::Text("Child Players: %d", static_cast<s32>(mPlayers.size()));
    for (auto ptr_player : mPlayers) {
        auto player = ptr_player;

        if (player == nullptr)
            continue;

        ImGui::Text("Child Player: %d Frame: %d", player->mUniqueID, player->GetCurrentTimestamp());
    }
}

void ImTimeline::TimelinePlayer::ChangeState(eTimelineState aState)
{
    // check for transitions that are impossible
    //...

    for (auto ptr_player : mPlayers) {
        auto player = ptr_player;

        if (player == nullptr)
            continue;

        player->ChangeState(aState);
    }

    mState = aState;
}

TimelineNode* ImTimeline::TimelinePlayer::GetNextNodeToPlay()
{
    TimelineNode* result = nullptr;
    mTimelineData->iterate([this, &result](TimelineNode& node) {
        if (result) {
            return;
        }

        if (mPlayingNode && mPlayingNode->GetID() == node.GetID()) {
            return;
        }

        if ((f32)node.start > mTimeStep.GetTimestamp()) {
            result = &node;
        }
    });

    return result;
}