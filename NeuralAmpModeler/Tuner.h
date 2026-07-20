#pragma once

// A lightweight monophonic pitch detector (YIN algorithm) used by the tuner.
// It accumulates mono input into a ring buffer and, every hop, estimates the
// fundamental frequency in the guitar/bass range. Detection only runs while the
// tuner UI is open, so it costs nothing in normal use.

#include <algorithm>
#include <cmath>
#include <vector>

class PitchDetector
{
public:
  void Reset(double sampleRate, int windowSize = 2048, int hopSize = 1024)
  {
    mSampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    mWindow = windowSize;
    mHopSize = hopSize;
    mBuf.assign(mWindow, 0.0f);
    mFrame.assign(mWindow, 0.0f);
    mDiff.assign(mWindow / 2 + 1, 0.0f);
    mWritePos = 0;
    mFilled = 0;
    mHop = 0;
    mLastFreq = -1.0f;
  }

  // Push mono samples. Runs an analysis whenever a hop's worth of new samples
  // has arrived and the window is full.
  template <typename T>
  void Push(const T* x, int n)
  {
    if (mBuf.empty())
      return;

    for (int i = 0; i < n; i++)
    {
      mBuf[mWritePos] = static_cast<float>(x[i]);
      mWritePos = (mWritePos + 1) % mWindow;
      if (mFilled < mWindow)
        mFilled++;
      if (++mHop >= mHopSize && mFilled >= mWindow)
      {
        mHop = 0;
        mLastFreq = Analyze();
      }
    }
  }

  // Latest estimate in Hz, or -1 if no confident pitch (silence / noise).
  float GetFrequency() const { return mLastFreq; }

private:
  float Analyze()
  {
    const int W = mWindow;
    const int halfW = W / 2;

    // Linearize the ring buffer, oldest sample first.
    double rms = 0.0;
    for (int i = 0; i < W; i++)
    {
      const float s = mBuf[(mWritePos + i) % W];
      mFrame[i] = s;
      rms += static_cast<double>(s) * s;
    }
    rms = std::sqrt(rms / W);
    if (rms < kSignalFloor) // too quiet to be a note
      return -1.0f;

    const int tauMin = std::max(2, static_cast<int>(mSampleRate / kMaxHz));
    const int tauMax = std::min(halfW - 1, static_cast<int>(mSampleRate / kMinHz));
    if (tauMax <= tauMin)
      return -1.0f;

    // 1) Difference function.
    for (int tau = 0; tau <= tauMax; tau++)
    {
      double sum = 0.0;
      for (int j = 0; j < halfW; j++)
      {
        const double d = static_cast<double>(mFrame[j]) - mFrame[j + tau];
        sum += d * d;
      }
      mDiff[tau] = static_cast<float>(sum);
    }

    // 2) Cumulative mean normalized difference.
    mDiff[0] = 1.0f;
    double running = 0.0;
    for (int tau = 1; tau <= tauMax; tau++)
    {
      running += mDiff[tau];
      mDiff[tau] = static_cast<float>(mDiff[tau] * tau / (running > 0.0 ? running : 1.0));
    }

    // 3) Absolute threshold: first dip below threshold within the tau range,
    //    descending to its local minimum.
    int tauEst = -1;
    for (int tau = tauMin; tau <= tauMax; tau++)
    {
      if (mDiff[tau] < kThreshold)
      {
        while (tau + 1 <= tauMax && mDiff[tau + 1] < mDiff[tau])
          tau++;
        tauEst = tau;
        break;
      }
    }
    if (tauEst == -1)
      return -1.0f; // no confident period

    // 4) Parabolic interpolation for sub-sample accuracy.
    float betterTau = static_cast<float>(tauEst);
    if (tauEst > tauMin && tauEst < tauMax)
    {
      const float s0 = mDiff[tauEst - 1];
      const float s1 = mDiff[tauEst];
      const float s2 = mDiff[tauEst + 1];
      const float denom = (2.0f * s1 - s2 - s0);
      if (std::fabs(denom) > 1e-9f)
        betterTau = tauEst + (s2 - s0) / (2.0f * denom);
    }

    if (betterTau <= 0.0f)
      return -1.0f;
    return static_cast<float>(mSampleRate / betterTau);
  }

  // Guitar/bass range plus headroom.
  static constexpr double kMinHz = 55.0; // A1
  static constexpr double kMaxHz = 1200.0; // above the highest fretted note
  static constexpr float kThreshold = 0.15f; // YIN aperiodicity threshold
  static constexpr double kSignalFloor = 1.0e-3; // RMS gate

  double mSampleRate = 48000.0;
  int mWindow = 2048;
  int mHopSize = 1024;
  int mWritePos = 0;
  int mFilled = 0;
  int mHop = 0;
  float mLastFreq = -1.0f;

  std::vector<float> mBuf; // ring buffer
  std::vector<float> mFrame; // linearized analysis frame
  std::vector<float> mDiff; // YIN difference / CMND buffer
};
