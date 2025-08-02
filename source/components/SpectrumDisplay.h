#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <mutex>

class SpectrumDisplay : public juce::Component, private juce::Timer
{
public:
    SpectrumDisplay();

    void setBufferToDisplay(const juce::AudioBuffer<float>* bufferToUse, std::mutex* mutexToUse);

    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;
    void computeFFT();

    const juce::AudioBuffer<float>* scopeBuffer = nullptr;
    std::mutex* scopeMutex = nullptr;

    static constexpr int fftOrder = 9;
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT forwardFFT{ fftOrder };
    std::array<float, fftSize * 2> fftData{};
    std::array<float, fftSize / 2> magnitudes{};
};

