#pragma once

#include <array>

class Timer
{
public:
    static void Update(double now);

    static float DeltaTime() { return s_DeltaTime; }
    static float UnscaledDeltaTime() { return s_UnscaledDeltaTime; }
    static float FixedDeltaTime() { return s_FixedDeltaTime; }

    static double TimeSinceStartup() { return s_TimeSinceStart; }
    static uint64_t FrameCount() { return s_FrameCount; }

    static float FPS() { return s_FPS; }
    static float AverageFrameTime() { return s_AverageFrameTime; }

private:
    static inline double s_LastFrameTime = 0.0;
    static inline float  s_DeltaTime = 0.0f;
    static inline float  s_UnscaledDeltaTime = 0.0f;

    static inline float  s_FixedDeltaTime = 1.0f / 60.0f;

    static inline double s_TimeSinceStart = 0.0;
    static inline uint64_t s_FrameCount = 0;

    // Profiling
    static inline float s_FPS = 0.0f;
    static inline float s_AverageFrameTime = 0.0f;

    static inline std::array<float, 120> s_FrameTimes{};
    static inline int s_FrameIndex = 0;
};
