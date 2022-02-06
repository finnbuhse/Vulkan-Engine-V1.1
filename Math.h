#pragma once
#include <chrono>

#define PI 3.1415926536f

float lerp(const float& start, const float& end, const float& progress);

float slerp(const float& start, const float& end, const float& progress);

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