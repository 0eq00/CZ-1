/*
  ==============================================================================
    PluginProcessor.cpp
    Casio CZ-1 Phase Distortion Synthesizer Audio Processor Implementation
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"volume", 1},
        "Master Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.85f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"chorus", 1},
        "Stereo Chorus",
        true));

    return { params.begin(), params.end() };
}

CZ1AudioProcessor::CZ1AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       parameters (*this, nullptr, juce::Identifier ("CZ1Params"), createParameterLayout())
#else
     : parameters (*this, nullptr, juce::Identifier ("CZ1Params"), createParameterLayout())
#endif
{
    volumeParam = parameters.getRawParameterValue ("volume");
    chorusParam = parameters.getRawParameterValue ("chorus");
}

CZ1AudioProcessor::~CZ1AudioProcessor()
{
}

const juce::String CZ1AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CZ1AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CZ1AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CZ1AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double CZ1AudioProcessor::getTailLengthSeconds() const
{
    return 0.5;
}

int CZ1AudioProcessor::getNumPrograms()
{
    return CZ1Engine::NUM_PRESETS;
}

int CZ1AudioProcessor::getCurrentProgram()
{
    return engine.getCurrentPresetIndex();
}

void CZ1AudioProcessor::setCurrentProgram (int index)
{
    engine.loadPreset (index);
}

const juce::String CZ1AudioProcessor::getProgramName (int index)
{
    return juce::String(engine.getPresetSlot(index)) + ": " + juce::String(engine.getPresetName(index));
}

void CZ1AudioProcessor::changeProgramName (int, const juce::String&)
{
}

void CZ1AudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    resampler.prepare (CZ1Engine::NATIVE_RATE, sampleRate);
    engine.reset();
}

void CZ1AudioProcessor::releaseResources()
{
    resampler.reset();
    engine.allNotesOff();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CZ1AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void CZ1AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update parameters
    if (chorusParam != nullptr)
        engine.setChorusEnabled (*chorusParam > 0.5f);

    const float masterVol = (volumeParam != nullptr) ? volumeParam->load() : 0.85f;

    // Process keyboard state and incoming MIDI
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    auto* leftChannel  = buffer.getWritePointer (0);
    auto* rightChannel = (totalNumOutputChannels > 1) ? buffer.getWritePointer (1) : nullptr;

    int currentSample = 0;
    const int numSamples = buffer.getNumSamples();

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        const int msgPosition = std::clamp (metadata.samplePosition, 0, numSamples - 1);

        // Render audio up to this MIDI message
        while (currentSample < msgPosition)
        {
            while (resampler.needsInput())
            {
                float srcL = 0.0f, srcR = 0.0f;
                engine.renderNativeSample (srcL, srcR);
                resampler.push (srcL, srcR);
            }

            float outL = 0.0f, outR = 0.0f;
            resampler.pop (outL, outR);

            leftChannel[currentSample] = outL * masterVol;
            if (rightChannel != nullptr)
                rightChannel[currentSample] = outR * masterVol;

            ++currentSample;
        }

        // Handle MIDI Message
        if (msg.isNoteOn())
        {
            engine.noteOn (msg.getNoteNumber(), msg.getVelocity());
        }
        else if (msg.isNoteOff())
        {
            engine.noteOff (msg.getNoteNumber(), msg.getVelocity());
        }
        else if (msg.isProgramChange())
        {
            engine.loadPreset (msg.getProgramChangeNumber());
        }
        else if (msg.isAllNotesOff())
        {
            engine.allNotesOff();
        }
    }

    // Render remaining audio in the block
    while (currentSample < numSamples)
    {
        while (resampler.needsInput())
        {
            float srcL = 0.0f, srcR = 0.0f;
            engine.renderNativeSample (srcL, srcR);
            resampler.push (srcL, srcR);
        }

        float outL = 0.0f, outR = 0.0f;
        resampler.pop (outL, outR);

        leftChannel[currentSample] = outL * masterVol;
        if (rightChannel != nullptr)
            rightChannel[currentSample] = outR * masterVol;

        ++currentSample;
    }
}

bool CZ1AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* CZ1AudioProcessor::createEditor()
{
    return new CZ1AudioProcessorEditor (*this);
}

void CZ1AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CZ1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CZ1AudioProcessor();
}
