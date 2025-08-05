#include "BandProcessor.h"

void BandProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = static_cast<float>(spec.sampleRate);

    for (int ch = 0; ch < 2; ++ch)
    {
        lowPass1[ch].reset();
        highPass1[ch].reset();
        lowPass2[ch].reset();
        highPass2[ch].reset();
        lowPass3[ch].reset();
        highPass3[ch].reset();

        lowPass1[ch].prepare(spec);
        highPass1[ch].prepare(spec);
        lowPass2[ch].prepare(spec);
        highPass2[ch].prepare(spec);
        lowPass3[ch].prepare(spec);
        highPass3[ch].prepare(spec);
    }

    setCrossoverFrequencies(freq1, freq2, freq3);
    isPrepared = true;
}

void BandProcessor::setCrossoverFrequencies(float f1, float f2, float f3)
{
    freq1 = f1;
    freq2 = f2;
    freq3 = f3;

    for (int ch = 0; ch < 2; ++ch)
    {
        *lowPass1[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq1);
        *highPass1[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, freq1);

        *lowPass2[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq2);
        *highPass2[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, freq2);

        *lowPass3[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq3);
        *highPass3[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, freq3);
    }
}

void BandProcessor::process(const juce::AudioBuffer<float>& input,
    juce::AudioBuffer<float>& low,
    juce::AudioBuffer<float>& midLow,
    juce::AudioBuffer<float>& midHigh,
    juce::AudioBuffer<float>& high)
{
    jassert(isPrepared);
    const int numSamples = input.getNumSamples();

    low.setSize(2, numSamples);
    midLow.setSize(2, numSamples);
    midHigh.setSize(2, numSamples);
    high.setSize(2, numSamples);

    low.makeCopyOf(input);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto block = juce::dsp::AudioBlock<float>(low).getSingleChannelBlock(ch);
        lowPass1[ch].process(juce::dsp::ProcessContextReplacing<float>(block));
    }

    high.makeCopyOf(input);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto block = juce::dsp::AudioBlock<float>(high).getSingleChannelBlock(ch);
        highPass3[ch].process(juce::dsp::ProcessContextReplacing<float>(block));
    }

    juce::AudioBuffer<float> mid;
    mid.makeCopyOf(input);
    for (int ch = 0; ch < 2; ++ch)
    {
        float* m = mid.getWritePointer(ch);
        const float* l = low.getReadPointer(ch);
        const float* h = high.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            m[i] -= (l[i] + h[i]);
    }
    midLow.makeCopyOf(mid);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto block = juce::dsp::AudioBlock<float>(midLow).getSingleChannelBlock(ch);
        lowPass2[ch].process(juce::dsp::ProcessContextReplacing<float>(block));
    }

    midHigh.makeCopyOf(mid);
    for (int ch = 0; ch < 2; ++ch)
    {
        float* mh = midHigh.getWritePointer(ch);
        const float* ml = midLow.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            mh[i] -= ml[i];
    }
}

