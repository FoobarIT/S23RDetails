#pragma once

#include <array>
#include <functional>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <mutex>
#include <vector>

class MultibandWidget : public juce::Component,
                        private juce::Timer
{
public:
    MultibandWidget();

    // Prépare le processeur audio (à appeler avant process)
    void prepare (const juce::dsp::ProcessSpec& spec);

    // Traite l'input en 4 bandes
    void process (const juce::AudioBuffer<float>& input,
        juce::AudioBuffer<float>& low,
        juce::AudioBuffer<float>& midLow,
        juce::AudioBuffer<float>& midHigh,
        juce::AudioBuffer<float>& high);

    // Récupérer le buffer pour visualisation (optionnel)
    void setBufferToDisplay (const juce::AudioBuffer<float>* bufferToUse, std::mutex* mutexToUse);

    // Callback quand les fréquences changent (f1, f2, f3)
    std::function<void (float, float, float)> onFrequenciesChanged;

    // Component overrides
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

private:
    // Band splitting frequencies
    std::vector<float> bandFrequencies { 200.f, 1000.f, 5000.f };
    int draggingIndex = -1;

    // Traitement audio
    float sampleRate = 44100.f;
    bool isPrepared = false;

    juce::dsp::IIR::Filter<float> lowPass1[2]; // freq1
    juce::dsp::IIR::Filter<float> highPass1[2];
    juce::dsp::IIR::Filter<float> lowPass2[2]; // freq2
    juce::dsp::IIR::Filter<float> highPass2[2];
    juce::dsp::IIR::Filter<float> lowPass3[2]; // freq3
    juce::dsp::IIR::Filter<float> highPass3[2];

    void setCrossoverFrequencies (float f1, float f2, float f3);

    // FFT display
    const juce::AudioBuffer<float>* scopeBuffer = nullptr;
    std::mutex* scopeMutex = nullptr;

    static constexpr int fftOrder = 9;
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT forwardFFT { fftOrder };
    std::array<float, fftSize * 2> fftData {};
    std::array<float, fftSize / 2> magnitudes {};

    void timerCallback() override;
    void computeFFT();

    // Helper UI freq <> position
    float frequencyToX (float freq) const;
    float xToFrequency (float x) const;
    float getSeparatorHitboxWidth() const { return 8.0f; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultibandWidget)
};
