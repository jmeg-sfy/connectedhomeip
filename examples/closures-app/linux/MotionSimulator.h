#include <functional>
#include <system/SystemLayer.h>
#include <memory>

namespace chip {
namespace app {

class MotionSimulator
{
public:
    using CompleteCallback = std::function<void()>;
    using ProgressCallback = std::function<void(const char * progressMessage)>;

    MotionSimulator();

    MotionSimulator & SetMoveDuration(System::Clock::Milliseconds32 duration);
    MotionSimulator & SetCalibrationDuration(System::Clock::Milliseconds32 duration);

    void StartMotion(CompleteCallback onComplete, ProgressCallback onProgress);
    void StartCalibration(CompleteCallback onComplete, ProgressCallback onProgress);
    void Cancel();

private:
    System::Clock::Milliseconds32 mMoveDuration{};
    System::Clock::Milliseconds32 mCalibrationDuration{};

    System::Clock::Timestamp mMoveStartTime;
    System::Clock::Timestamp mCalibrationStartTime;

    enum class SimulatorState { Stopped, InMotion, Calibrating }; 
    SimulatorState mState = SimulatorState::Stopped;
    struct TimerContext {
        MotionSimulator * simulator;
        CompleteCallback onComplete;
        ProgressCallback onProgress;
    };

    std::unique_ptr<TimerContext> mCurrentContext;

    // Static callback function for timers
    static void TimerCallback(System::Layer * systemLayer, void * appState);
    void NextMotion(CompleteCallback onComplete, ProgressCallback onProgress);
    void NextCalibration(CompleteCallback onComplete, ProgressCallback onProgress);
    std::string GetProgressMessage(float percentage);
};

} // namespace app
} // namespace chip
