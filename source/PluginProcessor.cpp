#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
#if !JucePlugin_IsMidiEffect
    #if !JucePlugin_IsSynth
              .withInput ("Input", juce::AudioChannelSet::stereo(), true)
    #endif
              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      apvts (*this, nullptr, "Parameters", createParameters())
{
    scopeBuffer.setSize (getTotalNumOutputChannels(), scopeBufferSize);
}
PluginProcessor::~PluginProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("WIDTH1", "Width Band 1", 0.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("WIDTH2", "Width Band 2", 0.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("WIDTH3", "Width Band 3", 0.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("WIDTH4", "Width Band 4", 0.0f, 2.0f, 1.0f));


    return { params.begin(), params.end() };
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec {};
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    multibandWidget.prepare (spec);

    const int numChannels = getTotalNumOutputChannels();

    const int circularBufferSize = scopeBufferSize * 2;

    circularBuffer.setSize (numChannels, circularBufferSize);
    circularBuffer.clear();

    scopeBuffer.setSize (numChannels, scopeBufferSize);
    scopeBuffer.clear();

    circularFifo.setTotalSize (circularBufferSize); 
}
//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}


void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
    #if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
#endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    (void) midiMessages;
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Initialisation des buffers pour chaque bande
    juce::AudioBuffer<float> low (numChannels, numSamples);
    juce::AudioBuffer<float> midLow (numChannels, numSamples);
    juce::AudioBuffer<float> midHigh (numChannels, numSamples);
    juce::AudioBuffer<float> high (numChannels, numSamples);

    low.clear();
    midLow.clear();
    midHigh.clear();
    high.clear();

    // Traitement du signal en 4 bandes
    multibandWidget.process (buffer, low, midLow, midHigh, high);

    // Récupération des paramètres width
    float width1 = *apvts.getRawParameterValue ("WIDTH1");
    float width2 = *apvts.getRawParameterValue ("WIDTH2");
    float width3 = *apvts.getRawParameterValue ("WIDTH3");
    float width4 = *apvts.getRawParameterValue ("WIDTH4");

    auto applyWidth = [&] (juce::AudioBuffer<float>& band, float width) {
        if (band.getNumChannels() < 2)
            return; // On ne peut pas faire de traitement mid/side sans 2 canaux

        auto* left = band.getWritePointer (0);
        auto* right = band.getWritePointer (1);
        int numSamplesBand = band.getNumSamples();

        for (int i = 0; i < numSamplesBand; ++i)
        {
            float mid = 0.5f * (left[i] + right[i]);
            float side = 0.5f * (left[i] - right[i]);
            side *= width;
            left[i] = mid + side;
            right[i] = mid - side;
        }
    };

    applyWidth (low, width1);
    applyWidth (midLow, width2);
    applyWidth (midHigh, width3);
    applyWidth (high, width4);

    // Remise à zéro du buffer principal avant addition des bandes
    buffer.clear();

    // Addition des bandes dans le buffer principal avec vérifications
    for (int ch = 0; ch < numChannels; ++ch)
    {
        if (ch < low.getNumChannels())
            buffer.addFrom (ch, 0, low, ch, 0, low.getNumSamples());

        if (ch < midLow.getNumChannels())
            buffer.addFrom (ch, 0, midLow, ch, 0, midLow.getNumSamples());

        if (ch < midHigh.getNumChannels())
            buffer.addFrom (ch, 0, midHigh, ch, 0, midHigh.getNumSamples());

        if (ch < high.getNumChannels())
            buffer.addFrom (ch, 0, high, ch, 0, high.getNumSamples());
    }

    // Gestion du buffer circulaire pour scope
    const int maxSamples = buffer.getNumSamples();
    const int totalFifoSize = circularFifo.getTotalSize();
    const int numSamplesToCopy = juce::jmin (totalFifoSize, maxSamples);

    {
        std::scoped_lock lock (scopeBufferMutex);

        if (circularFifo.getFreeSpace() >= numSamplesToCopy && numSamplesToCopy > 0)
        {
            int start1, size1, start2, size2;
            circularFifo.prepareToWrite (numSamplesToCopy, start1, size1, start2, size2);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                if (size1 > 0)
                    circularBuffer.copyFrom (ch, start1, buffer, ch, 0, size1);
                if (size2 > 0)
                    circularBuffer.copyFrom (ch, start2, buffer, ch, size1, size2);
            }
            circularFifo.finishedWrite (size1 + size2);
        }

        int start1, size1, start2, size2;
        circularFifo.prepareToRead (scopeBufferSize, start1, size1, start2, size2);

        for (int ch = 0; ch < scopeBuffer.getNumChannels(); ++ch)
        {
            if (size1 > 0)
                scopeBuffer.copyFrom (ch, 0, circularBuffer, ch, start1, size1);
            if (size2 > 0)
                scopeBuffer.copyFrom (ch, size1, circularBuffer, ch, start2, size2);
        }

        circularFifo.finishedRead (size1 + size2);
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}