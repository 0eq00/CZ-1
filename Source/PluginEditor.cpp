/*
  ==============================================================================
    PluginEditor.cpp
    Casio CZ-1 Phase Distortion Synthesizer Audio Processor Editor Implementation
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

CZ1AudioProcessorEditor::CZ1AudioProcessorEditor (CZ1AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      keyboardComponent (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setSize (780, 480);

    // Keyboard
    keyboardComponent.setAvailableRange (36, 96);
    keyboardComponent.setKeyWidth (24.0f);
    addAndMakeVisible (keyboardComponent);

    // Chorus button
    chorusButton.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    chorusButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xff00e5ff));
    addAndMakeVisible (chorusButton);
    chorusAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        p.getVTS(), "chorus", chorusButton);

    // Preset Selector Dropdown
    presetLabel.setText ("PRESET TONE (64 ORIGINAL PATCHES)", juce::dontSendNotification);
    presetLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    presetLabel.setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff));
    addAndMakeVisible (presetLabel);

    for (int i = 0; i < CZ1Engine::NUM_PRESETS; ++i)
    {
        presetSelector.addItem (juce::String (p.getEngine().getPresetSlot (i)) + "  " + p.getEngine().getPresetName (i), i + 1);
    }
    presetSelector.setSelectedId (p.getCurrentProgram() + 1, juce::dontSendNotification);
    presetSelector.onChange = [this]()
    {
        const int selected = presetSelector.getSelectedItemIndex();
        if (selected >= 0)
        {
            audioProcessor.setCurrentProgram (selected);
            repaint();
        }
    };
    presetSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff101319));
    presetSelector.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    presetSelector.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff00e5ff));
    addAndMakeVisible (presetSelector);

    // Volume Slider
    volumeSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    volumeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    volumeSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff00e5ff));
    volumeSlider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    volumeSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    volumeSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff1a1d24));
    addAndMakeVisible (volumeSlider);
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        p.getVTS(), "volume", volumeSlider);

    volumeLabel.setText ("VOLUME", juce::dontSendNotification);
    volumeLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    volumeLabel.setJustificationType (juce::Justification::centred);
    volumeLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (volumeLabel);

    startTimerHz (30);
}

CZ1AudioProcessorEditor::~CZ1AudioProcessorEditor()
{
    stopTimer();
}

void CZ1AudioProcessorEditor::timerCallback()
{
    const int curProgram = audioProcessor.getCurrentProgram();
    if (presetSelector.getSelectedItemIndex() != curProgram)
    {
        presetSelector.setSelectedId (curProgram + 1, juce::dontSendNotification);
    }
    repaint (220, 24, 380, 110); // Repaint LCD region for voice & preset updates
}

void CZ1AudioProcessorEditor::paint (juce::Graphics& g)
{
    // Dark metallic chassis
    g.fillAll (juce::Colour (0xff16181d));

    // Vintage CZ-1 accent stripes
    g.setColour (juce::Colour (0xffd32f2f)); // Red accent line
    g.fillRect (0, 0, getWidth(), 4);
    g.setColour (juce::Colour (0xff0288d1)); // Blue accent line
    g.fillRect (0, 4, getWidth(), 3);

    // CZ-1 Badge
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("CASIO", 24, 20, 100, 28, juce::Justification::left);

    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffe0e0e0));
    g.drawText ("CZ-1", 116, 20, 100, 28, juce::Justification::left);

    g.setFont (juce::FontOptions (11.0f));
    g.setColour (juce::Colour (0xff9e9e9e));
    g.drawText ("DIGITAL SYNTHESIZER\nPHASE DISTORTION / NEC uPD933", 24, 52, 180, 30, juce::Justification::topLeft);

    // Vintage Backlit LCD Panel
    const juce::Rectangle<int> lcdArea (220, 24, 380, 110);
    g.setColour (juce::Colour (0xff001f24)); // Bezel
    g.fillRoundedRectangle (lcdArea.toFloat(), 6.0f);
    g.setColour (juce::Colour (0xff004d40));
    g.drawRoundedRectangle (lcdArea.toFloat(), 6.0f, 2.0f);

    // LCD Glow
    const int curPreset = audioProcessor.getCurrentProgram();
    const char* slot = audioProcessor.getEngine().getPresetSlot (curPreset);
    const char* name = audioProcessor.getEngine().getPresetName (curPreset);

    g.setColour (juce::Colour (0xff00e5ff));
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 17.0f, juce::Font::bold));
    g.drawText (juce::String (slot) + ": " + juce::String (name), lcdArea.getX() + 16, lcdArea.getY() + 14, 340, 22, juce::Justification::left);

    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
    g.drawText ("PHASE DISTORTION  |  AUTHENTIC MULTI-STAGE ENV", lcdArea.getX() + 16, lcdArea.getY() + 40, 340, 18, juce::Justification::left);

    const int activeVoices = audioProcessor.getEngine().getActiveVoiceCount();
    juce::String status = "VOICES: " + juce::String (activeVoices) + " / 8  |  VELOCITY SENSITIVE";
    g.drawText (status, lcdArea.getX() + 16, lcdArea.getY() + 60, 340, 18, juce::Justification::left);

    g.setColour (juce::Colour (0xff80deea));
    g.drawText ("DAC: 40.0 kHz  |  KAISER SINC RESAMPLER", lcdArea.getX() + 16, lcdArea.getY() + 82, 340, 16, juce::Justification::left);

    // Control Group Panel
    const juce::Rectangle<int> controlArea (20, 160, getWidth() - 40, 160);
    g.setColour (juce::Colour (0xff1e222b));
    g.fillRoundedRectangle (controlArea.toFloat(), 8.0f);
    g.setColour (juce::Colour (0xff2c313d));
    g.drawRoundedRectangle (controlArea.toFloat(), 8.0f, 1.5f);

    // Group Titles
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff00e5ff));
    g.drawText ("SYNTHESIZER CONFIGURATION", 40, 172, 300, 20, juce::Justification::left);
}

void CZ1AudioProcessorEditor::resized()
{
    const int panelY = 160;

    presetLabel.setBounds    (40, panelY + 38, 260, 20);
    presetSelector.setBounds (40, panelY + 62, 260, 32);

    chorusButton.setBounds   (330, panelY + 62, 160, 32);

    volumeSlider.setBounds (getWidth() - 140, panelY + 30, 80, 80);
    volumeLabel.setBounds  (getWidth() - 140, panelY + 115, 80, 20);

    keyboardComponent.setBounds (20, getHeight() - 130, getWidth() - 40, 110);
}
