#include "MotionSimulator.h"
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>

namespace chip {
namespace app {

MotionSimulator::MotionSimulator() : mMoveDuration(0), mCalibrationDuration(0) {}


void MotionSimulator::StartMotion(CompleteCallback onComplete, ProgressCallback onProgress) {
    
    if (mState == SimulatorState::InMotion) 
    {
        // If already in motion, cancel the current operation before starting a new one
        ChipLogDetail(NotSpecified, "MotionSimulator: Cancelling current motion to start a new MoveTo command");
        Cancel();
    }

    // Set up for new motion
    mMoveStartTime = System::SystemClock().GetMonotonicTimestamp();
    mState = SimulatorState::InMotion;  // Set state to InMotion
    NextMotion(onComplete, onProgress); // Start the new motion
}

void MotionSimulator::StartCalibration(CompleteCallback onComplete, ProgressCallback onProgress) {

    if (mState == SimulatorState::Calibrating) 
    {
        ChipLogDetail(NotSpecified, "MotionSimulator already calibrating");
        return;
    }

    mCalibrationStartTime = System::SystemClock().GetMonotonicTimestamp();
    mState = SimulatorState::Calibrating;  // Set state to Calibrating
    NextCalibration(onComplete, onProgress); // Start calibration
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

    // Check simulator state to determine which function to call
    if (context && context->simulator) {
        if (context->simulator->mState == SimulatorState::InMotion) {
            context->simulator->NextMotion(context->onComplete, context->onProgress);
        } else if (context->simulator->mState == SimulatorState::Calibrating) {
            context->simulator->NextCalibration(context->onComplete, context->onProgress);
        } else {
            ChipLogError(Zcl, "MotionSimulator: TimerCallback called in Stopped state.");
        }
    }
}

std::string MotionSimulator::GetProgressMessage(float percentage) {
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "Progress: %.1f%%", percentage);
    return std::string(buffer);
}

void MotionSimulator::NextMotion(CompleteCallback onComplete, ProgressCallback onProgress) {
    auto elapsedTime = System::SystemClock().GetMonotonicTimestamp() - mMoveStartTime;
    float progressPercentage = (static_cast<float>(elapsedTime.count()) / mMoveDuration.count()) * 100;

    if (elapsedTime >= mMoveDuration) {
        onProgress(GetProgressMessage(100).c_str());
        onComplete();
        mCurrentContext.reset();
        return;
    }

    onProgress(GetProgressMessage(progressPercentage).c_str());
    mCurrentContext = std::make_unique<TimerContext>(TimerContext{this, onComplete, onProgress});

    // Schedule the next update
    DeviceLayer::SystemLayer().StartTimer(
        System::Clock::Milliseconds32(500),
        TimerCallback,
        mCurrentContext.get());
}

void MotionSimulator::NextCalibration(CompleteCallback onComplete, ProgressCallback onProgress) {
    auto elapsedTime = System::SystemClock().GetMonotonicTimestamp() - mCalibrationStartTime;
    float progressPercentage = (static_cast<float>(elapsedTime.count()) / mCalibrationDuration.count()) * 100;

    if (elapsedTime >= mCalibrationDuration) {
        onProgress(GetProgressMessage(100).c_str());
        onComplete();
        mCurrentContext.reset();  // Clear the context to signal end
        return;
    }

    onProgress(GetProgressMessage(progressPercentage).c_str());
    mCurrentContext = std::make_unique<TimerContext>(TimerContext{this, onComplete, onProgress});

    // Schedule next update
    DeviceLayer::SystemLayer().StartTimer(
        System::Clock::Milliseconds32(500),
        TimerCallback,
        mCurrentContext.get());
}

} // namespace app
} // namespace chip
