#include "MotionSimulator.h"
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>

namespace chip {
namespace app {

bool MotionSimulator::Execute(DoneCallback && doneCallback, ProgressCallback && progressCallback)
{
    if (mMoveDuration.count() <= 0)
    {
        ChipLogError(NotSpecified, "MotionSimulator: Move duration must be positive.");
        return false;
    }

    mDoneCallback = std::move(doneCallback);
    mProgressCallback = std::move(progressCallback);
    mIsRunning = true;
    mProgressStep = 0;

    StartTimer(mMoveDuration / 4); // Start the timer for the first progress step
    EmitProgress("Starting movement...");

    return true;
}

void MotionSimulator::StartTimer(System::Clock::Timeout duration)
{
    DeviceLayer::SystemLayer().StartTimer(duration, &MotionSimulator::OnTimerDone, this);
}

void MotionSimulator::Cancel()
{
    if (mIsRunning)
    {
        DeviceLayer::SystemLayer().CancelTimer(&MotionSimulator::OnTimerDone, this);
        mIsRunning = false;
        ChipLogDetail(Zcl, "MotionSimulator: Movement canceled.");
        EmitProgress("Movement canceled.");
    }
}

void MotionSimulator::OnTimerDone(System::Layer * layer, void * appState)
{
    MotionSimulator * that = reinterpret_cast<MotionSimulator *>(appState);
    that->Next();
}

void MotionSimulator::Next()
{
    if (!mIsRunning)
    {
        return; // If the simulator is not running, do nothing.
    }

    mProgressStep++;

    switch (mProgressStep)
    {
    case 1:
        EmitProgress("25% of movement complete...");
        StartTimer(mMoveDuration / 4); // Continue to the next step
        break;
    case 2:
        EmitProgress("50% of movement complete...");
        StartTimer(mMoveDuration / 4); // Continue to the next step
        break;
    case 3:
        EmitProgress("75% of movement complete...");
        StartTimer(mMoveDuration / 4); // Continue to the next step
        break;
    case 4:
        EmitProgress("Movement complete.");
        mIsRunning = false;
        if (mDoneCallback)
        {
            mDoneCallback(); // Notify that the motion has completed.
        }
        break;
    default:
        mIsRunning = false;
        break;
    }
}

void MotionSimulator::EmitProgress(const char * message)
{
    if (mProgressCallback)
    {
        mProgressCallback(message);
    }
    ChipLogDetail(Zcl, "MotionSimulator: %s", message);
}

} // namespace app
} // namespace chip
