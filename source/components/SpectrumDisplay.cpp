#include <juce_gui_extra/juce_gui_extra.h>
#include "SpectrumDisplay.h"

SpectrumDisplay::SpectrumDisplay()
{
    startTimerHz(60);
}

void SpectrumDisplay::setBufferToDisplay(const juce::AudioBuffer<float>* bufferToUse, std::mutex* mutexToUse)
{
    scopeBuffer = bufferToUse;
    scopeMutex = mutexToUse;
}

void SpectrumDisplay::timerCallback()
{
    computeFFT();
    repaint();
}

void SpectrumDisplay::computeFFT()
{
    if (scopeBuffer == nullptr || scopeMutex == nullptr)
        return;

    std::scoped_lock lock(*scopeMutex);

    if (scopeBuffer->getNumSamples() < fftSize)
        return;

    auto* channelData = scopeBuffer->getReadPointer(0);

    // Copy and zero the FFT buffer (real and imag)
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    std::copy(channelData, channelData + fftSize, fftData.begin());

    juce::dsp::WindowingFunction<float> window(fftSize, juce::dsp::WindowingFunction<float>::hann, false);
    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

    for (int i = 0; i < fftSize / 2; ++i)
    {
        auto db = juce::Decibels::gainToDecibels(fftData[i]);
        magnitudes[i] = juce::jmap(db, -100.0f, 0.0f, 0.0f, 1.0f);
    }
}

void SpectrumDisplay::paint(juce::Graphics& g)
{
    //g.fillAll(juce::Colours::black.withAlpha);

    g.setColour(juce::Colours::white);

    auto width = getWidth();
    auto height = getHeight();

    juce::Path spectrumPath;
    spectrumPath.startNewSubPath(0, height);

    for (int i = 0; i < (int)magnitudes.size(); ++i)
    {
        float normX = (float)i / (float)magnitudes.size();
        float x = normX * width;
        float y = juce::jmap(magnitudes[i], 0.0f, 1.0f, (float)height, 0.0f);
        //spectrumPath.lineTo(x, y);
        if (i == 0)
            spectrumPath.startNewSubPath(x, y);
        else
            spectrumPath.lineTo(x, y);
    }

    /*spectrumPath.lineTo(width, height);
    spectrumPath.closeSubPath();

    g.fillPath(spectrumPath);*/
    g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
}