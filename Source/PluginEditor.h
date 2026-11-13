/*
  ==============================================================================
    PluginEditor.h
    Casio CZ-1 Phase Distortion Synthesizer Audio Processor Editor
    Includes faithful DATA ENTRY SECTION (p.8) & PARAMETER SECTION (p.9)
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

class CZ1AudioProcessorEditor : public juce::AudioProcessorEditor,
                                private juce::Timer,
                                public juce::FileDragAndDropTarget
{
public:
    CZ1AudioProcessorEditor (CZ1AudioProcessor&);
    ~CZ1AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    bool keyPressed (const juce::KeyPress& key) override;
    void mouseDown (const juce::MouseEvent& event) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    void refreshPresetList();

private:
    enum class ParamScreen
    {
        Default = 0,
        Chorus,
        Name,
        Waveform,
        DcoEnv,
        DcwKeyFollow,
        DcwEnv,
        DcaKeyFollow,
        DcaEnv,
        Level,
        Velocity,
        Vibrato,
        Octave,
        Detune,
        CopyWaitSrc,
        CopyWaitDst,
        InitWait
    };

    enum LineParamKind
    {
        LP_Waveform = 0,
        LP_DcoEnv,
        LP_DcwKf,
        LP_DcwEnv,
        LP_DcaKf,
        LP_DcaEnv,
        LP_Level,
        LP_Velocity,
        LP_Count
    };

    struct LcdDisplayState
    {
        juce::String line1;
        juce::String line2;
        int cursorRow = -1;
        int cursorStart = 0;
        int cursorLen = 0;
    };

    LcdDisplayState buildLcdState() const;
    void adjustValue (int delta);
    void moveCursor (int dir);
    void changePage (int dir);
    void toggleSustainPoint();
    void toggleEndPoint();
    void onLineParamClicked (int lineIdx, LineParamKind kind);
    void onGlobalParamClicked (ParamScreen target);
    void onLineSelectClicked();
    void onRingClicked();
    void onNoiseClicked();
    void onCopyClicked();
    void onInitClicked();
    void drawLed (juce::Graphics& g, juce::Rectangle<int> bounds, bool lit, juce::Colour litColour = juce::Colour (0xffef4444));
    void styleButton (juce::TextButton& btn, bool active = false);
    void syncKeyboardFocusForScreen();

    CZ1AudioProcessor& audioProcessor;
    juce::MidiKeyboardComponent keyboardComponent;

    // Top Header Controls
    juce::ToggleButton chorusButton   { "Stereo Chorus" };
    juce::ComboBox     presetSelector;
    juce::Label        presetLabel;
    juce::TextButton   loadSyxButton  { "Load .SYX" };
    juce::TextButton   saveSyxButton  { "Save .SYX" };
    juce::TextButton   resetRomButton { "Reset ROM" };
    juce::Slider       volumeSlider;
    juce::Label        volumeLabel;

    // DATA ENTRY SECTION (Manual p.8)
    juce::TextButton valDownBtn   { "-" };
    juce::TextButton valUpBtn     { "+" };
    juce::TextButton curLeftBtn   { "<" };
    juce::TextButton curRightBtn  { ">" };
    juce::TextButton pageDownBtn  { "DOWN" };
    juce::TextButton pageUpBtn    { "UP" };
    juce::TextButton sustainBtn   { "SUSTAIN" };
    juce::TextButton endBtn       { "END" };
    juce::TextButton homeLcdBtn   { "HOME" };

    // PARAMETER SECTION (Manual p.9)
    juce::TextButton nameBtn       { "NAME" };
    juce::TextButton lineSelectBtn { "LINE\nSELECT" };
    juce::TextButton ringBtn       { "RING" };
    juce::TextButton noiseBtn      { "NOISE" };
    juce::TextButton copyBtn       { "PARAM\nCOPY" };
    juce::TextButton initBtn       { "INIT" };
    juce::TextButton vibratoBtn    { "VIBRATO" };
    juce::TextButton octaveBtn     { "OCTAVE" };
    juce::TextButton detuneBtn     { "DETUNE" };

    std::array<juce::TextButton, LP_Count> line1Btns;
    std::array<juce::TextButton, LP_Count> line2Btns;

    // Editor Interactive State
    ParamScreen currentScreen = ParamScreen::Default;
    int activeLine = 0;      // 0 or 1
    int envStep = 1;         // 1..8
    int cursorField = 0;     // active sub-field
    int copySrcLine = -1;
    LineParamKind copySrcKind = LP_Waveform;

    // Hold-to-repeat state for VALUE -/+
    int holdValueDir = 0;    // -1, 0, +1
    int holdTickCount = 0;

    // Transient flash message timer ticks
    int flashTicks = 0;
    juce::String flashLine1;
    juce::String flashLine2;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chorusAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;

    std::unique_ptr<juce::FileChooser> fileChooser;
    bool isDraggingFile = false;
    int lastPresetCount = -1;
    int lastProgramIndex = -1;
    juce::String lastBankName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CZ1AudioProcessorEditor)
};
