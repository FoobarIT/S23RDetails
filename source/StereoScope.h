#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class StereoScope : public juce::Component, private juce::Timer
{
public:
    StereoScope()
    {
        setFramesPerSecond(60);
        startTimerHz(frameRate);
    }

    void setAudioBuffer(const juce::AudioBuffer<float>* buffer)
    {
        audioBuffer = buffer;
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced(10);

        // Définir la zone de l'arc
        float arcSize = juce::jmin(area.getWidth(), area.getHeight()) * 0.8f;
        juce::Rectangle<float> arcArea(
            area.getCentreX() - arcSize / 2.0f,
            area.getCentreY() - arcSize / 2.0f + 40.0f, 
            arcSize,
            arcSize);

        // Fond de l'arc - de gauche (180°) à droite (0°) en passant par le bas
        juce::Path arcPath;

        auto centre = arcArea.getCentre();
        float radius = arcArea.getWidth() / 2.0f;
        float thickness = 2.0f;

       
        g.setColour(juce::Colours::cyan);
        g.strokePath(arcPath, juce::PathStrokeType(thickness));


        g.drawLine(area.getCentreX(), area.getY(), area.getCentreX(), area.getBottom());
        g.drawLine(area.getX(), area.getCentreY(), area.getRight(), area.getCentreY());
        // Tracé dynamique
        for (size_t i = 0; i < trailFrames.size(); ++i)
        {
            float alpha = 1.0f - (float)i / (float)trailFrames.size();
            g.setColour(juce::Colours::cyan.withAlpha(alpha * 0.6f));

            for (auto& pt : trailFrames[i])
                g.fillEllipse(pt.x - 1.5f, pt.y - 1.5f, 3.0f, 3.0f);
        }

        // L et R
        g.setFont(14.0f);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawText("L", area.getX(), area.getBottom() - 20, 20, 20, juce::Justification::centredLeft);
        g.drawText("R", area.getRight() - 20, area.getBottom() - 20, 20, 20, juce::Justification::centredRight);
    }

private:
    const juce::AudioBuffer<float>* audioBuffer = nullptr;
    std::vector<std::vector<juce::Point<float>>> trailFrames;
    const int maxTrailLength = 20;
    int frameRate = 60;

    void setFramesPerSecond(int fps) { frameRate = fps; }

    void timerCallback() override
    {
        if (!audioBuffer || audioBuffer->getNumChannels() < 2)
            return;

        auto* left = audioBuffer->getReadPointer(0);
        auto* right = audioBuffer->getReadPointer(1);
        int numSamples = audioBuffer->getNumSamples();

        auto area = getLocalBounds().toFloat().reduced(10.0f);
        float centerX = area.getCentreX();
        float centerY = area.getCentreY();
        float radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;

        std::vector<juce::Point<float>> framePoints;

        int step = juce::jmax(1, numSamples / 512); // pour lisser sans perdre en densité

        for (int i = 0; i < numSamples; i += step)
        {
            float l = left[i];
            float r = right[i];

            float x = juce::jlimit(-1.0f, 1.0f, (l - r));   // balance stéréo (L-R)
            float y = juce::jlimit(-1.0f, 1.0f, (l + r));   // amplitude (L+R)

            float px = centerX + x * radius;
            float py = centerY - y * radius;

            framePoints.emplace_back(px, py);
        }

        trailFrames.insert(trailFrames.begin(), framePoints);
        if (trailFrames.size() > maxTrailLength)
            trailFrames.pop_back();

        repaint();
    }
};