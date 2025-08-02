#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class CustomSlider : public juce::LookAndFeel_V4
{
public:
	CustomSlider()
	{
		setColour(juce::Slider::thumbColourId, juce::Colours::white);
		setColour(juce::Slider::trackColourId, juce::Colour(90, 90, 90));
		setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
		setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::deepskyblue);
	}

	void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override 
	{
        const float radius = juce::jmin(width / 2.0f, height / 2.0f) - 4.0f;
        const float centreX = x + width * 0.5f;
        const float centreY = y + height * 0.5f;
        const float rx = centreX - radius;
        const float ry = centreY - radius;
        const float rw = radius * 2.0f;

        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Fond
        g.setColour(findColour(juce::Slider::rotarySliderOutlineColourId));
        g.fillEllipse(rx, ry, rw, rw);

        // Remplissage
        g.setColour(findColour(juce::Slider::rotarySliderFillColourId));
        juce::Path path;
        path.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, angle, 0.8f);
        g.fillPath(path);

        // Pointeur
        juce::Path p;
        float pointerLength = radius * 0.7f;
        float pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        g.setColour(findColour(juce::Slider::thumbColourId));
        g.fillPath(p, juce::AffineTransform::rotation(angle).translated(centreX, centreY));
	}
};