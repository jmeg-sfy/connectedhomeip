#include "MotionSimulator.h"
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>

namespace chip {
namespace app {

MotionSimulator::MotionSimulator() : mMoveDuration(0), mCalibrationDuration(0) {}


void MotionSimulator::StartMotion(CompleteCallback onComplete, ProgressCallback onProgress) 
{
    if (mState == SimulatorState::InMotion) 
    {
        // If already in motion, cancel the current operation before starting a new one
        ChipLogDetail(NotSpecified, "MotionSimulator: Cancelling current motion to start a new MoveTo command");
        Cancel();
    }

    ChipLogDetail(NotSpecified, "MotionSimulator Start Motion");
    // Set up for new motion
    mMoveStartTime = System::SystemClock().GetMonotonicTimestamp();
    mState = SimulatorState::InMotion; 
    NextPhase(onComplete, onProgress, mMoveStartTime, mMoveDuration);
}

void MotionSimulator::StartCalibration(CompleteCallback onComplete, ProgressCallback onProgress) 
{
    if (mState == SimulatorState::Calibrating) 
    {
        ChipLogDetail(NotSpecified, "MotionSimulator already calibrating");
        return;
    }

    ChipLogDetail(NotSpecified, "MotionSimulator Start Calibration");
    mCalibrationStartTime = System::SystemClock().GetMonotonicTimestamp();
    mState = SimulatorState::Calibrating;
    NextPhase(onComplete, onProgress, mCalibrationStartTime, mCalibrationDuration);
}

void MotionSimulator::StartFallback(CompleteCallback onComplete, ProgressCallback onProgress) 
{
    if (mState == SimulatorState::PendingFallback) 
    {
        ChipLogDetail(NotSpecified, "MotionSimulator already in pending fallback");
        return;
    }

    ChipLogDetail(NotSpecified, "MotionSimulator Start Fallback");
    mFallbackStartTime = System::SystemClock().GetMonotonicTimestamp();
    mState = SimulatorState::PendingFallback;  
    NextPhase(onComplete, onProgress, mFallbackStartTime, mFallbackDuration);
}

MotionSimulator & MotionSimulator::SetMoveDuration(System::Clock::Milliseconds32 duration)
{
    mMoveDuration = duration;
    return *this;
}

MotionSimulator & MotionSimulator::SetCalibrationDuration(System::Clock::Milliseconds32 duration)
{
    mCalibrationDuration = duration;
    return *this;
}

MotionSimulator & MotionSimulator::SetFallbackDuration(System::Clock::Milliseconds32 duration)
{
    mFallbackDuration = duration;
    return *this;
}

void MotionSimulator::Cancel() {

    if (mCurrentContext) 
    {
        DeviceLayer::SystemLayer().CancelTimer(TimerCallback, mCurrentContext.get());
        mCurrentContext.reset();
    }

    mState = SimulatorState::Stopped;
}

void MotionSimulator::TimerCallback(chip::System::Layer * systemLayer, void * appState) {
    auto * context = static_cast<TimerContext *>(appState);

    if (context && context->simulator) {
        if (context->simulator->mState == SimulatorState::InMotion) 
        {
            context->simulator->NextPhase(context->onComplete, context->onProgress, context->simulator->mMoveStartTime, context->simulator->mMoveDuration);
        } 
        else if (context->simulator->mState == SimulatorState::Calibrating) 
        {
            context->simulator->NextPhase(context->onComplete, context->onProgress, context->simulator->mCalibrationStartTime, context->simulator->mCalibrationDuration);
        }
        else if (context->simulator->mState == SimulatorState::PendingFallback) 
        {
            context->simulator->NextPhase(context->onComplete, context->onProgress, context->simulator->mFallbackStartTime, context->simulator->mFallbackDuration);
        }  
        else 
        {
            ChipLogError(Zcl, "MotionSimulator: TimerCallback called in Stopped state.");
        }
    }
}

std::string MotionSimulator::GetProgressMessage(float percentage) {
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "Progress: %.1f%%", percentage);
    return std::string(buffer);
}

void MotionSimulator::NextPhase(CompleteCallback onComplete, ProgressCallback onProgress, System::Clock::Timestamp startTime, System::Clock::Milliseconds32 duration)
{
    auto elapsedTime = System::SystemClock().GetMonotonicTimestamp() - startTime;
    float progressPercentage = (static_cast<float>(elapsedTime.count()) / duration.count()) * 100;

    if (elapsedTime >= duration)
    {
        onProgress(GetProgressMessage(100).c_str());
        onComplete();
        mCurrentContext.reset();
        return;
    }

    onProgress(GetProgressMessage(progressPercentage).c_str());
    mCurrentContext = std::make_unique<TimerContext>(TimerContext{this, onComplete, onProgress});

    DeviceLayer::SystemLayer().StartTimer(
        System::Clock::Milliseconds32(500),
        TimerCallback,
        mCurrentContext.get());
}

} // namespace app
} // namespace chip
