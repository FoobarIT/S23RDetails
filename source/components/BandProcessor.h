#pragma once
#include <juce_dsp/juce_dsp.h>

class BandProcessor
{
public:
    BandProcessor() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void setCrossoverFrequencies(float f1, float f2, float f3);

    void process(const juce::AudioBuffer<float>& input,
        juce::AudioBuffer<float>& low,
        juce::AudioBuffer<float>& midLow,
        juce::AudioBuffer<float>& midHigh,
        juce::AudioBuffer<float>& high);

private:
    float sampleRate = 44100.0f;
    float freq1 = 200.0f;
    float freq2 = 1000.0f;
    float freq3 = 5000.0f;

    bool isPrepared = false;

    // 12 filtres : 2 par crossover (low/high) 2 canaux  3 crossovers
    juce::dsp::IIR::Filter<float> lowPass1[2];   // Freq1
    juce::dsp::IIR::Filter<float> highPass1[2];

    juce::dsp::IIR::Filter<float> lowPass2[2];   // Freq2
    juce::dsp::IIR::Filter<float> highPass2[2];

    juce::dsp::IIR::Filter<float> lowPass3[2];   // Freq3
    juce::dsp::IIR::Filter<float> highPass3[2];
};