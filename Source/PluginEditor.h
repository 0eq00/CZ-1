/*
  ==============================================================================
    PluginEditor.h
    Casio CZ-1 Phase Distortion Synthesizer Audio Processor Editor
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class CZ1AudioProcessorEditor : public juce::AudioProcessorEditor,
                                private juce::Timer
{
public:
    CZ1AudioProcessorEditor (CZ1AudioProcessor&);
    ~CZ1AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    CZ1AudioProcessor& audioProcessor;

    juce::MidiKeyboardComponent keyboardComponent;

    juce::ToggleButton chorusButton   { "Stereo Chorus" };
    juce::ComboBox     presetSelector;
    juce::Label        presetLabel;
    juce::Slider       volumeSlider;
    juce::Label        volumeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chorusAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CZ1AudioProcessorEditor)
};
