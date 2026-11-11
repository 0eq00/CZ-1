/*
  ==============================================================================
    PluginProcessor.h
    Casio CZ-1 Phase Distortion Synthesizer Audio Processor
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CZ1Engine.h"
#include "CZ1Resampler.h"

class CZ1AudioProcessor : public juce::AudioProcessor
{
public:
    CZ1AudioProcessor();
    ~CZ1AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool loadSyxFile (const juce::File& file);
    bool loadSyxData (const void* data, size_t size, const juce::String& name);
    void resetFactoryPresets();

    CZ1Engine& getEngine() { return engine; }
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    juce::AudioProcessorValueTreeState& getVTS() { return parameters; }

private:
    CZ1Engine engine;
    CZ1Resampler resampler;
    juce::MidiKeyboardState keyboardState;

    juce::AudioProcessorValueTreeState parameters;

    std::atomic<float>* volumeParam = nullptr;
    std::atomic<float>* chorusParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CZ1AudioProcessor)
};
