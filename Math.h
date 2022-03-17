#pragma once
#include <math.h>
#include <chrono>

#define PI 3.1415926536

float roundUpToMultiple(const float& x, const float& multiple);

float roundDownToMultiple(const float& x, const float& multiple);

template <typename T>
T lerp(const T& start, const T& end, const T& progress)
{
    return start + (end - start) * progress;
}

template <typename T>
T slerp(const T& start, const T& end, const T& progress)
{
    T smoothProgress = progress > 0.0 ? 0.5 - 0.5 * cos(T(PI) * progress) : 0.5 * cos(T(PI) * progress) - 0.5;
    return lerp(start, end, smoothProgress);
}

class LerpTime
{
private:
    float mMinimum;
    float mMaximum;
    const float mDuration;

    float mElapsed = 0.0f;

    std::chrono::high_resolution_clock::time_point mLastTime;

    bool mPaused = true;
    bool mReverse = false;

public:
    LerpTime(const float& minimum, const float& maximum, const float& duration);

    void start();

    void stop();

    float sample();

    void reverse();

    void set(const float& progress);
};

class SlerpTime
{
private:
    float mMinimum;
    float mMaximum;
    const float mDuration;

    float mElapsed = 0.0f;

    std::chrono::high_resolution_clock::time_point mLastTime;

    bool mPaused = true;
    bool mReverse = false;

public:
    SlerpTime(const float& minimum, const float& maximum, const float& duration);

    void start();

    void stop();

    float sample();

    void reverse();

    void set(const float& progress);
};