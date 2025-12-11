#include "Timer.h"

void Timer::Update(double now)
{
    s_UnscaledDeltaTime = float(now - s_LastFrameTime);

    // Protect against huge delta (pause / breakpoint)
    if (s_UnscaledDeltaTime > 0.25f)
        s_UnscaledDeltaTime = 0.25f;

    s_DeltaTime = s_UnscaledDeltaTime;

    s_LastFrameTime = now;
    s_TimeSinceStart = now;
    s_FrameCount++;

    s_FrameTimes[s_FrameIndex] = s_UnscaledDeltaTime;
    s_FrameIndex = (s_FrameIndex + 1) % s_FrameTimes.size();

    float sum = 0;
    for (float time : s_FrameTimes)
        sum += time;

    float avg = sum / s_FrameTimes.size();
    s_AverageFrameTime = avg;

    s_FPS = (avg > 0.0f) ? (1.0f / avg) : 0.0f;
}