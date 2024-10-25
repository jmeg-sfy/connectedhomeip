#include <functional>
#include <system/SystemLayer.h>

namespace chip {
namespace app {

class MotionSimulator
{
public:
    using DoneCallback = std::function<void()>;
    using ProgressCallback = std::function<void(const char * progressMessage)>;

    MotionSimulator() = default;

    bool Execute(DoneCallback && doneCallback, ProgressCallback && progressCallback);

    MotionSimulator & SetMoveDuration(System::Clock::Milliseconds32 duration)
    {
        mMoveDuration = duration;
        return *this;
    }

    void Cancel();

private:
    void StartTimer(System::Clock::Timeout duration);
    static void OnTimerDone(System::Layer * layer, void * appState);
    void Next();
    void EmitProgress(const char * message);

    DoneCallback mDoneCallback;
    ProgressCallback mProgressCallback;
    System::Clock::Milliseconds32 mMoveDuration{};
    bool mIsRunning{ false };
    uint8_t mProgressStep{ 0 };
};

} // namespace app
} // namespace chip
