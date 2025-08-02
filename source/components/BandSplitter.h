#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class BandSplitterComponent : public juce::Component
{
public:
    BandSplitterComponent();

    // Callback pour notifier le processor (ou autre)
    std::function<void(float, float, float)> onFrequenciesChanged;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

private:
    std::vector<float> bandFrequencies{ 200.0f, 1000.0f, 5000.0f }; // en Hz
    int draggingIndex = -1;

    // Constantes de plage fr	entielle
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;

    // Helpers
    float frequencyToX(float freq) const;
    float xToFrequency(float x) const;

    float getSeparatorHitboxWidth() const { return 8.0f; }
};
