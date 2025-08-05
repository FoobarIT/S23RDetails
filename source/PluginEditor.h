#pragma once

#include "BinaryData.h"
#include "PluginProcessor.h"
#include "StereoScope.h"
#include "CustomSlider.h"
#include "components/MultibandWidget.h"
#include "melatonin_inspector/melatonin_inspector.h"

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PluginProcessor& audioProcessor;
    PluginProcessor& processorRef;

    juce::Slider widthSlider1, widthSlider2, widthSlider3, widthSlider4;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment1, widthAttachment2, widthAttachment3, widthAttachment4;
    
    CustomSlider customSlider;
    StereoScope stereoScope;
    //SpectrumDisplay spectrum;
    //BandSplitterComponent bandSplitter;
    MultibandWidget multibandWidget;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};