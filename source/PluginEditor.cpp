#include "PluginEditor.h"
#include "CustomSlider.h"
#include "StereoScope.h"
#include "components/BandProcessor.h"
#include "components/BandSplitter.h"
#include "components/SpectrumDisplay.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor(p), processorRef (p)
{
    //juce::ignoreUnused (processorRef);

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
    spectrum.setBufferToDisplay (&processorRef.getScopeBuffer(), &processorRef.getScopeBufferMutex());
    //addAndMakeVisible (inspectButton);
    addAndMakeVisible (spectrum);
    addAndMakeVisible (bandSplitter);


    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 400);

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
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto area = getLocalBounds();
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    auto helloWorld = juce::String ("Hello from ") + PRODUCT_NAME_WITHOUT_VERSION + " v" VERSION + " running in " + CMAKE_BUILD_TYPE;
    g.drawText (helloWorld, area.removeFromTop (150), juce::Justification::centred, false);
}

void PluginEditor::resized()
{
    // layout the positions of your child components here
    auto area = getLocalBounds();
    area.removeFromBottom (50);
    inspectButton.setBounds (getLocalBounds().withSizeKeepingCentre (100, 50));

    int sliderSize = 80;
    int y = 300;

    widthSlider1.setBounds (30, y, sliderSize, sliderSize);
    widthSlider2.setBounds (130, y, sliderSize, sliderSize);
    widthSlider3.setBounds (230, y, sliderSize, sliderSize);
    widthSlider4.setBounds (330, y, sliderSize, sliderSize);
    // StereoScope a droit
    stereoScope.setBounds (200, 100, 180, 180);
    spectrum.setBounds (10, 10, getWidth() - 20, 100);
    bandSplitter.setBounds (5, 10, getWidth(), 100);
}

void PluginEditor::timerCallback()
{
    std::scoped_lock<std::mutex> lock (audioProcessor.getScopeBufferMutex());
    stereoScope.setAudioBuffer (&audioProcessor.getScopeBuffer());
    spectrum.setBufferToDisplay (&audioProcessor.getScopeBuffer(), &audioProcessor.getScopeBufferMutex());
    repaint();
}