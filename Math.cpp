#include "Math.h"
#include <cmath>

float lerp(const float& start, const float& end, const float& progress)
{
    return start + (end - start) * progress;
}

float slerp(const float& start, const float& end, const float& progress)
{
    float smoothProgress = 0.5 - 0.5 * cos(PI * progress);
    return lerp(start, end, smoothProgress);
}

LerpTime::LerpTime(const float& minimum, const float& maximum, const float& duration) : mMinimum(minimum), mMaximum(maximum), mDuration(duration)
{
    
}

void LerpTime::start()
{
    mLastTime = std::chrono::high_resolution_clock::now();
    mPaused = false;
}

void LerpTime::stop()
{
    float deltaTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - mLastTime).count();
    mElapsed += mReverse ? -deltaTime : deltaTime;
    mPaused = true;
}

float LerpTime::sample()
{
    if (!mPaused)
    {
        float deltaTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - mLastTime).count();
        mElapsed += mReverse ? -deltaTime : deltaTime;
    }

    mLastTime = std::chrono::high_resolution_clock::now();

    if (mElapsed < 0.0f)
    {
        mElapsed = 0.0f;
        return mMinimum;
    }
    else if (mElapsed > mDuration)
    {
        mElapsed = mDuration;
        return mMaximum;
    }
    return lerp(mMinimum, mMaximum, mElapsed / mDuration);
}

void LerpTime::reverse()
{
    mReverse = !mReverse;
}

SlerpTime::SlerpTime(const float& minimum, const float& maximum, const float& duration) : mMinimum(minimum), mMaximum(maximum), mDuration(duration)
{

}

void SlerpTime::start()
{
    mLastTime = std::chrono::high_resolution_clock::now();
    mPaused = false;
}

void SlerpTime::stop()
{
    float deltaTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - mLastTime).count();
    mElapsed += mReverse ? -deltaTime : deltaTime;
    mPaused = true;
}

float SlerpTime::sample()
{
    if (!mPaused)
    {
        float deltaTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - mLastTime).count();
        mElapsed += mReverse ? -deltaTime : deltaTime;
    }

    mLastTime = std::chrono::high_resolution_clock::now();

    if (mElapsed < 0.0f)
    {
        mElapsed = 0.0f;
        return mMinimum;
    }
    else if (mElapsed > mDuration)
    {
        mElapsed = mDuration;
        return mMaximum;
    }
    return slerp(mMinimum, mMaximum, mElapsed / mDuration);
}

void SlerpTime::reverse()
{
    mReverse = !mReverse;
}