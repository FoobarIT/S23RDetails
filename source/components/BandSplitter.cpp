#include "BandSplitter.h"

BandSplitterComponent::BandSplitterComponent()
{
    setInterceptsMouseClicks(true, false);
}

void BandSplitterComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::transparentBlack);

    // Couleurs des bandes
    juce::Colour colours[] = {
        juce::Colour::fromRGBA(255, 60, 60, 80),     // Rouge semi-transparent
        juce::Colour::fromRGBA(255, 165, 0, 80),     // Orange
        juce::Colour::fromRGBA(50, 205, 50, 80),     // Vert
        juce::Colour::fromRGBA(70, 130, 180, 80)     // Bleu
    };

    float x0 = 0.0f;
    float x1 = frequencyToX(bandFrequencies[0]);
    float x2 = frequencyToX(bandFrequencies[1]);
    float x3 = frequencyToX(bandFrequencies[2]);
    float x4 = (float)getWidth();

    g.setColour(colours[0]);
    g.fillRect(x0, 0.0f, x1 - x0, (float)getHeight());

    g.setColour(colours[1]);
    g.fillRect(x1, 0.0f, x2 - x1, (float)getHeight());

    g.setColour(colours[2]);
    g.fillRect(x2, 0.0f, x3 - x2, (float)getHeight());

    g.setColour(colours[3]);
    g.fillRect(x3, 0.0f, x4 - x3, (float)getHeight());

    // Les s	rateurs
    g.setColour(juce::Colours::darkcyan);
    for (auto freq : bandFrequencies)
    {
        auto x = frequencyToX(freq);
        g.drawLine(x, 0.0f, x, (float)getHeight(), 2.0f);
    }

    // Affichage des frequences dynamiquement
    g.setFont(12.0f);
    g.setColour(juce::Colours::white);
    for (int i = 0; i < bandFrequencies.size(); ++i)
    {
        auto freq = bandFrequencies[i];
        auto x = frequencyToX(freq);
        g.drawText(juce::String((int)freq) + " Hz", (int)x - 25, getHeight() - 20, 50, 18, juce::Justification::centred);
    }
}

void BandSplitterComponent::mouseDown(const juce::MouseEvent& e)
{
    auto mouseX = (float)e.x;

    for (int i = 0; i < bandFrequencies.size(); ++i)
    {
        auto x = frequencyToX(bandFrequencies[i]);
        if (std::abs(x - mouseX) < getSeparatorHitboxWidth())
        {
            draggingIndex = i;
            break;
        }
    }
}

void BandSplitterComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingIndex >= 0)
    {
        auto newFreq = xToFrequency((float)e.x);

        // Contrainte : ne pas d	passer les autres bandes
        if (draggingIndex > 0)
            newFreq = std::max(newFreq, bandFrequencies[draggingIndex - 1] + 10.0f);
        if (draggingIndex < bandFrequencies.size() - 1)
            newFreq = std::min(newFreq, bandFrequencies[draggingIndex + 1] - 10.0f);

        bandFrequencies[draggingIndex] = juce::jlimit(minFreq, maxFreq, newFreq);
        repaint();

        // Notifie le plugin
        if (onFrequenciesChanged)
            onFrequenciesChanged(bandFrequencies[0], bandFrequencies[1], bandFrequencies[2]);
    }
}

float BandSplitterComponent::frequencyToX(float freq) const
{
    auto norm = std::log10(freq / minFreq) / std::log10(maxFreq / minFreq);
    return norm * getWidth();
}

float BandSplitterComponent::xToFrequency(float x) const
{
    auto norm = juce::jlimit(0.0f, 1.0f, x / getWidth());
    return minFreq * std::pow(maxFreq / minFreq, norm);
}