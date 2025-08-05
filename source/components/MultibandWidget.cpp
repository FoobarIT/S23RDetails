#include "MultibandWidget.h"
#include <cmath>

MultibandWidget::MultibandWidget()
{
    startTimerHz (60); // pour update FFT et repaint

    setInterceptsMouseClicks (true, true);
    setBufferedToImage (true);
    setPaintingIsUnclipped (true);
}

void MultibandWidget::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = static_cast<float> (spec.sampleRate);

    for (int ch = 0; ch < 2; ++ch)
    {
        lowPass1[ch].reset();
        highPass1[ch].reset();
        lowPass2[ch].reset();
        highPass2[ch].reset();
        lowPass3[ch].reset();
        highPass3[ch].reset();

        lowPass1[ch].prepare (spec);
        highPass1[ch].prepare (spec);
        lowPass2[ch].prepare (spec);
        highPass2[ch].prepare (spec);
        lowPass3[ch].prepare (spec);
        highPass3[ch].prepare (spec);
    }

    setCrossoverFrequencies (bandFrequencies[0], bandFrequencies[1], bandFrequencies[2]);
    isPrepared = true;
}

void MultibandWidget::setCrossoverFrequencies (float f1, float f2, float f3)
{
    bandFrequencies[0] = f1;
    bandFrequencies[1] = f2;
    bandFrequencies[2] = f3;

    for (int ch = 0; ch < 2; ++ch)
    {
        *lowPass1[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, f1);
        *highPass1[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, f1);

        *lowPass2[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, f2);
        *highPass2[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, f2);

        *lowPass3[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, f3);
        *highPass3[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, f3);
    }
}

void MultibandWidget::process (const juce::AudioBuffer<float>& input,
    juce::AudioBuffer<float>& low,
    juce::AudioBuffer<float>& midLow,
    juce::AudioBuffer<float>& midHigh,
    juce::AudioBuffer<float>& high)
{
    jassert (isPrepared);
    const int numSamples = input.getNumSamples();

    low.setSize (2, numSamples);
    midLow.setSize (2, numSamples);
    midHigh.setSize (2, numSamples);
    high.setSize (2, numSamples);

    low.makeCopyOf (input);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto block = juce::dsp::AudioBlock<float> (low).getSingleChannelBlock (ch);
        lowPass1[ch].process (juce::dsp::ProcessContextReplacing<float> (block));
    }

    high.makeCopyOf (input);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto block = juce::dsp::AudioBlock<float> (high).getSingleChannelBlock (ch);
        highPass3[ch].process (juce::dsp::ProcessContextReplacing<float> (block));
    }

    juce::AudioBuffer<float> mid;
    mid.makeCopyOf (input);
    for (int ch = 0; ch < 2; ++ch)
    {
        float* m = mid.getWritePointer (ch);
        const float* l = low.getReadPointer (ch);
        const float* h = high.getReadPointer (ch);
        for (int i = 0; i < numSamples; ++i)
            m[i] -= (l[i] + h[i]);
    }
    midLow.makeCopyOf (mid);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto block = juce::dsp::AudioBlock<float> (midLow).getSingleChannelBlock (ch);
        lowPass2[ch].process (juce::dsp::ProcessContextReplacing<float> (block));
    }

    midHigh.makeCopyOf (mid);
    for (int ch = 0; ch < 2; ++ch)
    {
        float* mh = midHigh.getWritePointer (ch);
        const float* ml = midLow.getReadPointer (ch);
        for (int i = 0; i < numSamples; ++i)
            mh[i] -= ml[i];
    }
}

void MultibandWidget::setBufferToDisplay (const juce::AudioBuffer<float>* bufferToUse, std::mutex* mutexToUse)
{
    scopeBuffer = bufferToUse;
    scopeMutex = mutexToUse;
}

void MultibandWidget::timerCallback()
{
    computeFFT();
    repaint();
}

void MultibandWidget::computeFFT()
{
    if (scopeBuffer == nullptr || scopeMutex == nullptr)
        return;

    std::scoped_lock lock (*scopeMutex);

    if (scopeBuffer->getNumSamples() < fftSize)
        return;

    auto* channelData = scopeBuffer->getReadPointer (0);

    std::fill (fftData.begin(), fftData.end(), 0.0f);
    std::copy (channelData, channelData + fftSize, fftData.begin());

    juce::dsp::WindowingFunction<float> window (fftSize, juce::dsp::WindowingFunction<float>::hann, false);
    window.multiplyWithWindowingTable (fftData.data(), fftSize);

    forwardFFT.performFrequencyOnlyForwardTransform (fftData.data());

    for (int i = 0; i < fftSize / 2; ++i)
    {
        auto db = juce::Decibels::gainToDecibels (fftData[i]);
        magnitudes[i] = juce::jmap (db, -100.0f, 0.0f, 0.0f, 1.0f);
    }
}

void MultibandWidget::paint (juce::Graphics& g)
{
    drawBackgroundAndShadow (g);
    drawBands (g);
    drawSeparators (g);
    drawFrequencies (g);
    drawSpectrum (g);
}

void MultibandWidget::drawBackgroundAndShadow (juce::Graphics& g)
{
    const int cornerSize = 12;
    juce::Rectangle<int> bounds = getLocalBounds().reduced (8);
    juce::DropShadow shadow (juce::Colours::black.withAlpha (0.5f), 10, { 0, 4 });
    juce::Path shadowPath;
    shadowPath.addRoundedRectangle (bounds.toFloat(), (float) cornerSize);
    shadow.drawForPath (g, shadowPath);

    g.setColour (juce::Colours::white.withAlpha (0.1f));
    g.drawRoundedRectangle (bounds.toFloat(), (float) cornerSize, 1.5f);
}

void MultibandWidget::drawBands (juce::Graphics& g)
{
    juce::Colour colours[] = {
        juce::Colour::fromRGBA (255, 60, 60, 80), // Rouge
        juce::Colour::fromRGBA (255, 165, 0, 80), // Orange
        juce::Colour::fromRGBA (50, 205, 50, 80), // Vert
        juce::Colour::fromRGBA (70, 130, 180, 80) // Bleu
    };

    float x0 = 0.0f;
    float x1 = frequencyToX (bandFrequencies[0]);
    float x2 = frequencyToX (bandFrequencies[1]);
    float x3 = frequencyToX (bandFrequencies[2]);
    float x4 = (float) getWidth();

    g.setColour (colours[0]);
    g.fillRect (x0, 0.0f, x1 - x0, (float) getHeight());

    g.setColour (colours[1]);
    g.fillRect (x1, 0.0f, x2 - x1, (float) getHeight());

    g.setColour (colours[2]);
    g.fillRect (x2, 0.0f, x3 - x2, (float) getHeight());

    g.setColour (colours[3]);
    g.fillRect (x3, 0.0f, x4 - x3, (float) getHeight());
}

void MultibandWidget::drawSeparators (juce::Graphics& g)
{
    g.setColour (juce::Colours::darkcyan);
    for (auto freq : bandFrequencies)
    {
        auto x = frequencyToX (freq);
        g.drawLine (x, 0.0f, x, (float) getHeight(), 2.0f);
    }
}

void MultibandWidget::drawFrequencies (juce::Graphics& g)
{
    g.setFont (12.0f);
    g.setColour (juce::Colours::white);
    for (int i = 0; i < bandFrequencies.size(); ++i)
    {
        auto freq = bandFrequencies[i];
        auto x = frequencyToX (freq);
        g.drawText (juce::String ((int) freq) + " Hz",
            (int) x - 25,
            getHeight() - 20,
            50,
            18,
            juce::Justification::centred);
    }
}

void MultibandWidget::drawSpectrum (juce::Graphics& g)
{
    juce::Path spectrumPath;
    auto width = getWidth();
    auto height = getHeight();

    spectrumPath.startNewSubPath (0.f, (float) height);
    for (int i = 0; i < (int) magnitudes.size(); ++i)
    {
        float normX = (float) i / (float) magnitudes.size();
        float x = normX * width;
        float y = juce::jmap (magnitudes[i], 0.0f, 1.0f, (float) height, 0.0f);

        if (i == 0)
            spectrumPath.startNewSubPath (x, y);
        else
            spectrumPath.lineTo (x, y);
    }
    g.setColour (juce::Colours::white);
    g.strokePath (spectrumPath, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
}

void MultibandWidget::mouseDown (const juce::MouseEvent& e)
{
    float mouseX = (float) e.x;

    for (int i = 0; i < bandFrequencies.size(); ++i)
    {
        float x = frequencyToX (bandFrequencies[i]);
        if (std::abs (x - mouseX) < getSeparatorHitboxWidth())
        {
            draggingIndex = i;
            break;
        }
    }
}

void MultibandWidget::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingIndex < 0)
        return;

    float newFreq = xToFrequency ((float) e.x);

    // Contraintes pour ne pas dépasser les autres séparateurs
    if (draggingIndex > 0)
        newFreq = std::max (newFreq, bandFrequencies[draggingIndex - 1] + 10.0f);
    if (draggingIndex < (int) bandFrequencies.size() - 1)
        newFreq = std::min (newFreq, bandFrequencies[draggingIndex + 1] - 10.0f);

    bandFrequencies[draggingIndex] = juce::jlimit (20.0f, 20000.0f, newFreq);
    setCrossoverFrequencies (bandFrequencies[0], bandFrequencies[1], bandFrequencies[2]);

    if (onFrequenciesChanged)
        onFrequenciesChanged (bandFrequencies[0], bandFrequencies[1], bandFrequencies[2]);

    repaint();
}

float MultibandWidget::frequencyToX (float freq) const
{
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
    float norm = std::log10 (freq / minFreq) / std::log10 (maxFreq / minFreq);
    return norm * getWidth();
}

float MultibandWidget::xToFrequency (float x) const
{
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
    float norm = juce::jlimit (0.0f, 1.0f, x / getWidth());
    return minFreq * std::pow (maxFreq / minFreq, norm);
}

int MultibandWidget::getSeparatorHitboxWidth() const
{
    return 8;
}
