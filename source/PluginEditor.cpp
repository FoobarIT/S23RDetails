#include "PluginEditor.h"
#include "CustomSlider.h"
#include "StereoScope.h"
#include "components/MultibandWidget.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor(p), processorRef (p)
{

    for (auto* s : { &widthSlider1, &widthSlider2, &widthSlider3, &widthSlider4 })
    {
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        s->setLookAndFeel (&customSlider);
        addAndMakeVisible (*s);
    }

    widthAttachment1 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "WIDTH1", widthSlider1);
    widthAttachment2 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "WIDTH2", widthSlider2);
    widthAttachment3 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "WIDTH3", widthSlider3);
    widthAttachment4 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "WIDTH4", widthSlider4);


    addAndMakeVisible(stereoScope);
    
    multibandWidget.setBufferToDisplay (&processorRef.getScopeBuffer(), &processorRef.getScopeBufferMutex());
    addAndMakeVisible (multibandWidget);


    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (800, 400);

    startTimer (60);
}

PluginEditor::~PluginEditor()
{
    widthSlider1.setLookAndFeel (nullptr);
    widthSlider2.setLookAndFeel (nullptr);
    widthSlider3.setLookAndFeel (nullptr);
    widthSlider4.setLookAndFeel (nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    (void) g;
    g.fillAll (juce::Colour::fromRGB(13, 19, 33));
}

void PluginEditor::resized()
{
    // layout the positions of your child components here
    auto area = getLocalBounds();

    int sliderSize = 80;
    int y = 300;

    widthSlider1.setBounds (30, y, sliderSize, sliderSize);
    widthSlider2.setBounds (130, y, sliderSize, sliderSize);
    widthSlider3.setBounds (230, y, sliderSize, sliderSize);
    widthSlider4.setBounds (330, y, sliderSize, sliderSize);
    // StereoScope a droit
    stereoScope.setBounds (200, 100, 180, 180);
    multibandWidget.setBounds (10, 10, getWidth() - 20, 140);

}

void PluginEditor::timerCallback()
{
    std::scoped_lock<std::mutex> lock (audioProcessor.getScopeBufferMutex());
    stereoScope.setAudioBuffer (&audioProcessor.getScopeBuffer());
    multibandWidget.setBufferToDisplay (&audioProcessor.getScopeBuffer(), &audioProcessor.getScopeBufferMutex());
    repaint();
}