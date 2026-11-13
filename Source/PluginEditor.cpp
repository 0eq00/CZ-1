/*
  ==============================================================================
    PluginEditor.cpp
    Casio CZ-1 Phase Distortion Synthesizer Audio Processor Editor Implementation
    Reproduces Casio CZ-1 DATA ENTRY SECTION (p.8) & PARAMETER SECTION (p.9)
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
    static const char* const kAllowedNameChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-/ ";

    juce::String pad16 (const juce::String& s)
    {
        return s.substring (0, 16).paddedRight (' ', 16);
    }

    juce::String fmt2 (int v)
    {
        v = std::clamp (v, 0, 99);
        return juce::String (v).paddedLeft ('0', 2);
    }

    int sysexToDcaRate (uint8_t b)   { return (b == 0) ? 0 : (b >= 119 ? 99 : std::min (99, (b * 99) / 119 + 1)); }
    uint8_t dcaRateToSysex (int r)   { r = std::clamp (r, 0, 99); return (r == 0) ? 0 : (r >= 99 ? 119 : static_cast<uint8_t> (std::clamp ((r * 119) / 99, 1, 118))); }
    int sysexToDcaLevel (uint8_t b)  { return (b == 0) ? 0 : std::clamp (static_cast<int> (b) - 28, 0, 99); }
    uint8_t dcaLevelToSysex (int l)  { l = std::clamp (l, 0, 99); return (l == 0) ? 0 : static_cast<uint8_t> (std::clamp (l + 28, 29, 127)); }

    int sysexToDcwRate (uint8_t b)   { return (b <= 8) ? 0 : (b >= 127 ? 99 : std::min (99, ((b - 8) * 99) / 119 + 1)); }
    uint8_t dcwRateToSysex (int r)   { r = std::clamp (r, 0, 99); return (r == 0) ? 8 : (r >= 99 ? 127 : static_cast<uint8_t> (std::clamp (8 + (r * 119) / 99, 9, 126))); }
    int sysexToDcwLevel (uint8_t b)  { return (b == 0) ? 0 : std::min (99, (b * 99) / 127 + 1); }
    uint8_t dcwLevelToSysex (int l)  { l = std::clamp (l, 0, 99); return (l == 0) ? 0 : (l >= 99 ? 127 : static_cast<uint8_t> (std::clamp ((l * 127) / 99, 1, 126))); }

    int sysexToDcoRate (uint8_t b)   { return (b == 0) ? 0 : (b >= 127 ? 99 : std::min (99, (b * 99) / 127 + 1)); }
    uint8_t dcoRateToSysex (int r)   { r = std::clamp (r, 0, 99); return (r == 0) ? 0 : (r >= 99 ? 127 : static_cast<uint8_t> (std::clamp ((r * 127) / 99, 1, 126))); }
    int sysexToDcoLevel (uint8_t b)  { return (b > 63) ? std::clamp (static_cast<int> (b) - 4, 0, 99) : std::clamp (static_cast<int> (b), 0, 99); }
    uint8_t dcoLevelToSysex (int l)  { l = std::clamp (l, 0, 99); return (l >= 64) ? static_cast<uint8_t> (std::min (103, l + 4)) : static_cast<uint8_t> (l); }

    int chipWaveToPanelWave (uint8_t chipWave, uint8_t windowIdx)
    {
        switch (chipWave & 7)
        {
            case 0: return 1;
            case 1: return 2;
            case 2: return 3;
            case 4: return 4;
            case 5: return 5;
            case 6:
                if (windowIdx == 2) return 7;
                if (windowIdx == 3) return 8;
                return 6;
            default: return 1;
        }
    }

    void panelWaveToChip (int panelWave, uint8_t& outWave, uint8_t& outWin, bool& outIsReso)
    {
        outIsReso = false;
        switch (panelWave)
        {
            case 1: outWave = 0; outWin = 0; break;
            case 2: outWave = 1; outWin = 0; break;
            case 3: outWave = 2; outWin = 0; break;
            case 4: outWave = 4; outWin = 0; break;
            case 5: outWave = 5; outWin = 0; break;
            case 6: outWave = 6; outWin = 1; outIsReso = true; break;
            case 7: outWave = 6; outWin = 2; outIsReso = true; break;
            case 8: outWave = 6; outWin = 3; outIsReso = true; break;
            default: outWave = 0; outWin = 0; break;
        }
    }

    void updateLineRawWord (PresetLine& line, int w1Panel, int w2Panel, bool ringMod, bool noiseMod)
    {
        uint8_t c1 = 0, win1 = 0, c2 = 0, win2 = 0;
        bool r1 = false, r2 = false;
        panelWaveToChip (w1Panel, c1, win1, r1);
        const bool hasW2 = (w2Panel > 0);
        if (hasW2)
            panelWaveToChip (w2Panel, c2, win2, r2);
        else
            c2 = c1;

        const uint8_t win = r1 ? win1 : (r2 ? win2 : 0);
        const uint16_t modBits = ringMod ? 0x20 : (noiseMod ? 0x18 : 0x00);

        line.wave1 = c1;
        line.wave2 = c2;
        line.window = win;
        line.modulation = ringMod ? 4 : (noiseMod ? 3 : 0);
        line.rawWord = static_cast<uint16_t> (
            ((c1 & 7) << 13) |
            (((hasW2 ? c2 : 0) & 7) << 10) |
            (hasW2 ? (1 << 9) : 0) |
            ((win & 7) << 6) |
            modBits);
    }

    EnvelopeDef createInitEnv (int envKind) // 0=DCO, 1=DCW, 2=DCA
    {
        EnvelopeDef e{};
        e.end = 7;
        e.sustain = 0;
        for (int i = 0; i < 8; ++i)
        {
            if (envKind == 0)
            {
                e.steps[i].rate = dcoRateToSysex (50);
                e.steps[i].level = 0;
            }
            else if (envKind == 1)
            {
                e.steps[i].rate = dcwRateToSysex (i == 0 ? 99 : 50);
                e.steps[i].level = dcwLevelToSysex (i == 0 ? 99 : 0);
            }
            else
            {
                e.steps[i].rate = dcaRateToSysex (i == 0 ? 99 : 50);
                e.steps[i].level = dcaLevelToSysex (i == 0 ? 99 : 0);
            }
            e.steps[i].sustain = (i == 0);
        }
        return e;
    }

    const char* const kLineKeyLabels[8] = {
        "WAVE\nFORM", "ENV", "KEY\nFOLLOW", "ENV", "KEY\nFOLLOW", "ENV", "LEVEL", "VELOCITY"
    };
}

CZ1AudioProcessorEditor::CZ1AudioProcessorEditor (CZ1AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      keyboardComponent (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setSize (1020, 640);
    setWantsKeyboardFocus (true);

    // Keyboard (Full 88-key A0..C8 piano range)
    keyboardComponent.setAvailableRange (21, 108);
    keyboardComponent.setKeyWidth (18.8f);
    addAndMakeVisible (keyboardComponent);

    // Chorus toggle
    chorusButton.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    chorusButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xff00e5ff));
    chorusButton.setWantsKeyboardFocus (false);
    addAndMakeVisible (chorusButton);
    chorusAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        p.getVTS(), "chorus", chorusButton);

    // Preset Selector
    presetLabel.setText ("PRESET TONE", juce::dontSendNotification);
    presetLabel.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    presetLabel.setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff));
    addAndMakeVisible (presetLabel);

    presetSelector.onChange = [this]()
    {
        const int selected = presetSelector.getSelectedItemIndex();
        if (selected >= 0)
        {
            audioProcessor.setCurrentProgram (selected);
            currentScreen = ParamScreen::Default;
            copySrcLine = -1;
            syncKeyboardFocusForScreen();
            repaint();
        }
    };
    presetSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff101319));
    presetSelector.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    presetSelector.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff00e5ff));
    presetSelector.setWantsKeyboardFocus (false);
    addAndMakeVisible (presetSelector);

    // Load .SYX & Reset ROM
    loadSyxButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff003b46));
    loadSyxButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff00e5ff));
    loadSyxButton.setWantsKeyboardFocus (false);
    loadSyxButton.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Select Casio CZ SysEx File (.syx, .bin)",
            juce::File::getSpecialLocation (juce::File::userHomeDirectory),
            "*.syx;*.bin;*.SYX;*.BIN");

        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile() && audioProcessor.loadSyxFile (file))
            {
                refreshPresetList();
                currentScreen = ParamScreen::Default;
                syncKeyboardFocusForScreen();
                repaint();
            }
        });
    };
    addAndMakeVisible (loadSyxButton);

    saveSyxButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff064e3b));
    saveSyxButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff6ee7b7));
    saveSyxButton.setWantsKeyboardFocus (false);
    saveSyxButton.onClick = [this]()
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
        juce::File defaultFile = dir.getChildFile ("CZ1_Bank.syx");
        for (int n = 1; n < 1000 && defaultFile.exists(); ++n)
            defaultFile = dir.getChildFile ("CZ1_Bank_" + juce::String (n).paddedLeft ('0', 2) + ".syx");

        fileChooser = std::make_unique<juce::FileChooser> (
            "Save Casio CZ SysEx Bank as New File (.syx)",
            defaultFile,
            "*.syx;*.SYX");

        const auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file == juce::File())
                return;

            if (!file.hasFileExtension (".syx") && !file.hasFileExtension (".SYX") && !file.hasFileExtension (".bin"))
                file = file.withFileExtension (".syx");

            // Strictly prohibit overwriting any existing file
            if (file.exists())
            {
                flashLine1 = "   SAVE ERROR   ";
                flashLine2 = "FILE EXISTS!    ";
                flashTicks = 45;
                repaint();
                juce::AlertWindow::showMessageBoxAsync (
                    juce::MessageBoxIconType::WarningIcon,
                    "Save .SYX",
                    "Cannot overwrite an existing file. Please specify a new file name.");
                return;
            }

            if (audioProcessor.saveSyxBankToFile (file))
            {
                flashLine1 = "  BANK SAVED!   ";
                flashLine2 = file.getFileName().toUpperCase().substring (0, 16);
                flashTicks = 35;
            }
            else
            {
                flashLine1 = "   SAVE ERROR   ";
                flashLine2 = "WRITE FAILED!   ";
                flashTicks = 35;
            }
            repaint();
        });
    };
    addAndMakeVisible (saveSyxButton);

    resetRomButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff232832));
    resetRomButton.setColour (juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    resetRomButton.setWantsKeyboardFocus (false);
    resetRomButton.onClick = [this]()
    {
        audioProcessor.resetFactoryPresets();
        refreshPresetList();
        currentScreen = ParamScreen::Default;
        syncKeyboardFocusForScreen();
        repaint();
    };
    addAndMakeVisible (resetRomButton);

    // Master Volume
    volumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 20);
    volumeSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff00e5ff));
    volumeSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff00838f));
    volumeSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    volumeSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff101319));
    volumeSlider.setWantsKeyboardFocus (false);
    addAndMakeVisible (volumeSlider);
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        p.getVTS(), "volume", volumeSlider);

    volumeLabel.setText ("VOLUME", juce::dontSendNotification);
    volumeLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    volumeLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (volumeLabel);

    // DATA ENTRY SECTION buttons
    auto setupDataBtn = [this] (juce::TextButton& b, std::function<void()> fn)
    {
        styleButton (b, false);
        b.onClick = std::move (fn);
        addAndMakeVisible (b);
    };

    setupDataBtn (valDownBtn,  [this]() { adjustValue (-1); });
    setupDataBtn (valUpBtn,    [this]() { adjustValue (1); });
    setupDataBtn (curLeftBtn,  [this]() { moveCursor (-1); });
    setupDataBtn (curRightBtn, [this]() { moveCursor (1); });
    setupDataBtn (pageDownBtn, [this]() { changePage (-1); });
    setupDataBtn (pageUpBtn,   [this]() { changePage (1); });
    setupDataBtn (sustainBtn,  [this]() { toggleSustainPoint(); });
    setupDataBtn (endBtn,      [this]() { toggleEndPoint(); });

    homeLcdBtn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff14242b));
    homeLcdBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff00e5ff));
    homeLcdBtn.setWantsKeyboardFocus (false);
    homeLcdBtn.onClick = [this]()
    {
        currentScreen = ParamScreen::Default;
        copySrcLine = -1;
        syncKeyboardFocusForScreen();
        repaint();
    };
    addAndMakeVisible (homeLcdBtn);

    // PARAMETER SECTION top & side buttons
    setupDataBtn (nameBtn,       [this]() { onGlobalParamClicked (ParamScreen::Name); });
    setupDataBtn (lineSelectBtn, [this]() { onLineSelectClicked(); });
    setupDataBtn (ringBtn,       [this]() { onRingClicked(); });
    setupDataBtn (noiseBtn,      [this]() { onNoiseClicked(); });
    setupDataBtn (copyBtn,       [this]() { onCopyClicked(); });
    setupDataBtn (initBtn,       [this]() { onInitClicked(); });
    setupDataBtn (vibratoBtn,    [this]() { onGlobalParamClicked (ParamScreen::Vibrato); });
    setupDataBtn (octaveBtn,     [this]() { onGlobalParamClicked (ParamScreen::Octave); });
    setupDataBtn (detuneBtn,     [this]() { onGlobalParamClicked (ParamScreen::Detune); });

    // PARAMETER SECTION Line 1 & Line 2 8-button arrays
    for (int i = 0; i < LP_Count; ++i)
    {
        const LineParamKind kind = static_cast<LineParamKind> (i);
        line1Btns[i].setButtonText (kLineKeyLabels[i]);
        styleButton (line1Btns[i], false);
        line1Btns[i].onClick = [this, kind]() { onLineParamClicked (0, kind); };
        addAndMakeVisible (line1Btns[i]);

        line2Btns[i].setButtonText (kLineKeyLabels[i]);
        styleButton (line2Btns[i], false);
        line2Btns[i].onClick = [this, kind]() { onLineParamClicked (1, kind); };
        addAndMakeVisible (line2Btns[i]);
    }

    refreshPresetList();
    startTimerHz (30);
}

CZ1AudioProcessorEditor::~CZ1AudioProcessorEditor()
{
    stopTimer();
}

void CZ1AudioProcessorEditor::styleButton (juce::TextButton& btn, bool active)
{
    btn.setWantsKeyboardFocus (false);
    btn.setColour (juce::TextButton::buttonColourId, active ? juce::Colour (0xff27303f) : juce::Colour (0xff1a1f28));
    btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff2e3a4d));
    btn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe2e8f0));
    btn.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
}

void CZ1AudioProcessorEditor::syncKeyboardFocusForScreen()
{
    if (currentScreen == ParamScreen::Name)
    {
        keyboardComponent.setWantsKeyboardFocus (false);
        if (isShowing())
            grabKeyboardFocus();
    }
    else
    {
        keyboardComponent.setWantsKeyboardFocus (true);
    }
}

void CZ1AudioProcessorEditor::mouseDown (const juce::MouseEvent&)
{
    if (currentScreen == ParamScreen::Name && isShowing())
        grabKeyboardFocus();
}

bool CZ1AudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    if (currentScreen != ParamScreen::Name)
        return false;

    if (key == juce::KeyPress::leftKey)     { moveCursor (-1); return true; }
    if (key == juce::KeyPress::rightKey)    { moveCursor (1); return true; }
    if (key == juce::KeyPress::upKey)       { adjustValue (1); return true; }
    if (key == juce::KeyPress::downKey)     { adjustValue (-1); return true; }
    if (key == juce::KeyPress::homeKey)     { cursorField = 0; repaint(); return true; }
    if (key == juce::KeyPress::endKey)      { cursorField = 15; repaint(); return true; }
    if (key == juce::KeyPress::returnKey || key == juce::KeyPress::escapeKey)
    {
        currentScreen = ParamScreen::Default;
        syncKeyboardFocusForScreen();
        repaint();
        return true;
    }

    auto readPaddedName = [] (const CZ1PresetDef& p, char outBuf[17])
    {
        std::memset (outBuf, ' ', 16);
        const size_t len = std::min<size_t> (16, std::strlen (p.name));
        for (size_t i = 0; i < len; ++i)
            outBuf[i] = static_cast<char> (std::toupper (static_cast<unsigned char> (p.name[i])));
        outBuf[16] = '\0';
    };

    auto commitNameBuffer = [this] (CZ1PresetDef& p, const char inBuf[17])
    {
        char trimmed[17] = {};
        std::memcpy (trimmed, inBuf, 16);
        int lastNonSpace = 15;
        while (lastNonSpace >= 0 && trimmed[lastNonSpace] == ' ')
            --lastNonSpace;
        trimmed[lastNonSpace + 1] = '\0';
        std::memset (p.name, 0, sizeof (p.name));
        std::strncpy (p.name, trimmed, sizeof (p.name) - 1);
        refreshPresetList();
        repaint();
    };

    if (key == juce::KeyPress::backspaceKey)
    {
        const juce::ScopedLock sl (audioProcessor.getEngineLock());
        auto& preset = audioProcessor.getEngine().getCurrentPresetMutable();
        char buf[17] = {};
        readPaddedName (preset, buf);
        int idx = std::clamp (cursorField, 0, 15);
        if (idx > 0 && buf[idx] == ' ')
            --idx;
        buf[idx] = ' ';
        cursorField = idx;
        commitNameBuffer (preset, buf);
        return true;
    }

    if (key == juce::KeyPress::deleteKey)
    {
        const juce::ScopedLock sl (audioProcessor.getEngineLock());
        auto& preset = audioProcessor.getEngine().getCurrentPresetMutable();
        char buf[17] = {};
        readPaddedName (preset, buf);
        const int idx = std::clamp (cursorField, 0, 15);
        buf[idx] = ' ';
        commitNameBuffer (preset, buf);
        return true;
    }

    const juce::juce_wchar ch = key.getTextCharacter();
    if (ch >= 32 && ch <= 126)
    {
        const char upper = static_cast<char> (std::toupper (static_cast<int> (ch)));
        if (std::strchr (kAllowedNameChars, upper) != nullptr)
        {
            const juce::ScopedLock sl (audioProcessor.getEngineLock());
            auto& preset = audioProcessor.getEngine().getCurrentPresetMutable();
            char buf[17] = {};
            readPaddedName (preset, buf);
            const int idx = std::clamp (cursorField, 0, 15);
            buf[idx] = upper;
            cursorField = std::min (15, idx + 1);
            commitNameBuffer (preset, buf);
            return true;
        }
    }

    return false;
}

void CZ1AudioProcessorEditor::refreshPresetList()
{
    presetSelector.clear (juce::dontSendNotification);
    const int count = audioProcessor.getEngine().getPresetCount();
    for (int i = 0; i < count; ++i)
    {
        presetSelector.addItem (
            juce::String (audioProcessor.getEngine().getPresetSlot (i)) + "  " +
            juce::String (audioProcessor.getEngine().getPresetName (i)), i + 1);
    }
    presetSelector.setSelectedId (audioProcessor.getCurrentProgram() + 1, juce::dontSendNotification);
    lastPresetCount = count;
    lastProgramIndex = audioProcessor.getCurrentProgram();
    lastBankName = juce::String (audioProcessor.getEngine().getBankName());
    presetLabel.setText ("PRESET (" + juce::String (count) + " TONES · " + juce::String (audioProcessor.getEngine().getBankFormat()) + ")", juce::dontSendNotification);
}

void CZ1AudioProcessorEditor::timerCallback()
{
    const int curPresetCount = audioProcessor.getEngine().getPresetCount();
    const juce::String curBankName (audioProcessor.getEngine().getBankName());
    const int curProgram = audioProcessor.getCurrentProgram();

    if (curPresetCount != lastPresetCount || curBankName != lastBankName)
        refreshPresetList();
    else if (curProgram != lastProgramIndex)
    {
        lastProgramIndex = curProgram;
        presetSelector.setSelectedId (curProgram + 1, juce::dontSendNotification);
    }

    if (flashTicks > 0)
    {
        --flashTicks;
        if (flashTicks == 0) repaint();
    }

    // Hold-to-repeat for VALUE - / +
    const int dirDown = valDownBtn.isDown() ? -1 : (valUpBtn.isDown() ? 1 : 0);
    if (dirDown != 0)
    {
        if (holdValueDir != dirDown)
        {
            holdValueDir = dirDown;
            holdTickCount = 0;
        }
        else
        {
            holdTickCount++;
            if (holdTickCount > 10 && (holdTickCount % 2 == 0))
            {
                adjustValue (dirDown);
            }
        }
    }
    else
    {
        holdValueDir = 0;
        holdTickCount = 0;
    }

    repaint (32, 134, 316, 100); // Keep LCD responsive for voices/flash
}

// ==============================================================================
// Hardware Parameter Logic (Matches Casio CZ-1 Owner's Manual p.8-35)
// ==============================================================================

void CZ1AudioProcessorEditor::adjustValue (int delta)
{
    const juce::ScopedLock sl (audioProcessor.getEngineLock());
    auto& engine = audioProcessor.getEngine();
    auto& preset = engine.getCurrentPresetMutable();
    auto& line = preset.lines[activeLine];

    if (currentScreen == ParamScreen::Chorus)
    {
        if (auto* p = audioProcessor.getVTS().getParameter ("chorus"))
        {
            const bool next = !engine.isChorusEnabled();
            p->setValueNotifyingHost (next ? 1.0f : 0.0f);
        }
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::Name)
    {
        const int idx = std::clamp (cursorField, 0, 15);
        char buf[17] = {};
        std::memset (buf, ' ', 16);
        const size_t len = std::min<size_t> (16, std::strlen (preset.name));
        for (size_t i = 0; i < len; ++i)
            buf[i] = static_cast<char> (std::toupper (static_cast<unsigned char> (preset.name[i])));

        const char* found = std::strchr (kAllowedNameChars, buf[idx]);
        const int nChars = static_cast<int> (std::strlen (kAllowedNameChars));
        int pos = (found != nullptr) ? static_cast<int> (found - kAllowedNameChars) : (nChars - 1);
        pos = (pos + delta + nChars) % nChars;
        buf[idx] = kAllowedNameChars[pos];
        buf[16] = '\0';
        std::strncpy (preset.name, buf, sizeof (preset.name) - 1);
        refreshPresetList();
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::Waveform)
    {
        const bool hasW2 = ((line.rawWord >> 9) & 1) != 0;
        int w1 = chipWaveToPanelWave (line.wave1, line.window);
        int w2 = hasW2 ? chipWaveToPanelWave (line.wave2, line.window) : 0;

        if (cursorField == 0)
        {
            w1 += delta;
            if (w1 > 8) w1 = 1;
            if (w1 < 1) w1 = 8;
            if (w1 >= 6 && w2 >= 6) w2 = 0;
        }
        else
        {
            for (int t = 0; t < 9; ++t)
            {
                w2 += delta;
                if (w2 > 8) w2 = 0;
                if (w2 < 0) w2 = 8;
                if (w1 >= 6 && w2 >= 6) continue;
                break;
            }
        }

        const bool isRing = (line.rawWord >> 5) & 1;
        const bool isNoise = ((line.rawWord >> 3) & 3) == 3;
        updateLineRawWord (line, w1, w2, isRing, isNoise);
        audioProcessor.notifyLiveEdit();
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::DcoEnv || currentScreen == ParamScreen::DcwEnv || currentScreen == ParamScreen::DcaEnv)
    {
        EnvelopeDef& env = (currentScreen == ParamScreen::DcoEnv) ? line.dco
                         : (currentScreen == ParamScreen::DcwEnv) ? line.dcw
                         : line.dca;
        const int endStep = std::clamp (static_cast<int> (env.end) + 1, 1, 8);
        const int stIdx = std::clamp (envStep, 1, endStep) - 1;
        auto& st = env.steps[stIdx];

        if (cursorField == 0)
        {
            int r = (currentScreen == ParamScreen::DcaEnv) ? sysexToDcaRate (st.rate)
                  : (currentScreen == ParamScreen::DcwEnv) ? sysexToDcwRate (st.rate)
                  : sysexToDcoRate (st.rate);
            r = std::clamp (r + delta, 0, 99);
            st.rate = (currentScreen == ParamScreen::DcaEnv) ? dcaRateToSysex (r)
                    : (currentScreen == ParamScreen::DcwEnv) ? dcwRateToSysex (r)
                    : dcoRateToSysex (r);
        }
        else
        {
            if (stIdx == endStep - 1)
            {
                st.level = 0;
                return;
            }
            int l = (currentScreen == ParamScreen::DcaEnv) ? sysexToDcaLevel (st.level)
                  : (currentScreen == ParamScreen::DcwEnv) ? sysexToDcwLevel (st.level)
                  : sysexToDcoLevel (st.level);
            l = std::clamp (l + delta, 0, 99);
            st.level = (currentScreen == ParamScreen::DcaEnv) ? dcaLevelToSysex (l)
                     : (currentScreen == ParamScreen::DcwEnv) ? dcwLevelToSysex (l)
                     : dcoLevelToSysex (l);
        }
        audioProcessor.notifyLiveEdit();
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::DcwKeyFollow)
    {
        line.dcwKeyFollow = static_cast<uint8_t> (std::clamp (static_cast<int> (line.dcwKeyFollow) + delta, 0, 9));
        audioProcessor.notifyLiveEdit();
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::DcaKeyFollow)
    {
        line.dcaKeyFollow = static_cast<uint8_t> (std::clamp (static_cast<int> (line.dcaKeyFollow) + delta, 0, 9));
        audioProcessor.notifyLiveEdit();
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::Level)
    {
        int curLvl = (line.lineLevel == 0) ? 15 : std::clamp (static_cast<int> (line.lineLevel), 1, 15);
        line.lineLevel = static_cast<uint8_t> (std::clamp (curLvl + delta, 1, 15));
        audioProcessor.notifyLiveEdit();
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::Velocity)
    {
        if (cursorField == 0)
            line.dcaVelocity = static_cast<uint8_t> (std::clamp (static_cast<int> (line.dcaVelocity) + delta, 0, 15));
        else if (cursorField == 1)
            line.dcwVelocity = static_cast<uint8_t> (std::clamp (static_cast<int> (line.dcwVelocity) + delta, 0, 15));
        else
            line.dcoVelocity = static_cast<uint8_t> (std::clamp (static_cast<int> (line.dcoVelocity) + delta, 0, 15));
        audioProcessor.notifyLiveEdit();
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::Vibrato)
    {
        auto& vib = engine.getCurrentVibratoMutable();
        if (cursorField == 0)
        {
            vib.wave += delta;
            if (vib.wave > 4) vib.wave = 1;
            if (vib.wave < 1) vib.wave = 4;
        }
        else if (cursorField == 1) vib.delay = std::clamp (vib.delay + delta, 0, 99);
        else if (cursorField == 2) vib.rate  = std::clamp (vib.rate + delta, 0, 99);
        else                       vib.depth = std::clamp (vib.depth + delta, 0, 99);
        audioProcessor.notifyLiveEdit();
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::Octave)
    {
        preset.octave = static_cast<int8_t> (std::clamp (static_cast<int> (preset.octave) + delta, -1, 1));
        audioProcessor.notifyLiveEdit();
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::Detune)
    {
        auto& det = engine.getCurrentDetuneMutable();
        if (cursorField == 0)      det.sign = (delta > 0) ? 1 : -1;
        else if (cursorField == 1) det.oct  = std::clamp (det.oct + delta, 0, 3);
        else if (cursorField == 2) det.note = std::clamp (det.note + delta, 0, 11);
        else                       det.fine = std::clamp (det.fine + delta, 0, 60);
        audioProcessor.notifyLiveEdit();
        repaint();
    }
}

void CZ1AudioProcessorEditor::moveCursor (int dir)
{
    if (currentScreen == ParamScreen::InitWait)
    {
        if (dir > 0)
        {
            const juce::ScopedLock sl (audioProcessor.getEngineLock());
            auto& engine = audioProcessor.getEngine();
            auto& preset = engine.getCurrentPresetMutable();
            preset.lineMode = 0;
            preset.octave = 0;
            engine.getCurrentDetuneMutable() = { 1, 0, 0, 0 };
            engine.getCurrentVibratoMutable() = { 1, 0, 50, 0 };
            for (int l = 0; l < 2; ++l)
            {
                auto& line = preset.lines[l];
                updateLineRawWord (line, 1, 0, false, false);
                line.dcwKeyFollow = 0;
                line.dcaKeyFollow = 0;
                line.lineLevel = 15;
                line.dcaVelocity = 0;
                line.dcwVelocity = 0;
                line.dcoVelocity = 0;
                line.dco = createInitEnv (0);
                line.dcw = createInitEnv (1);
                line.dca = createInitEnv (2);
            }
            audioProcessor.notifyLiveEdit();
            currentScreen = ParamScreen::Default;
            flashLine1 = "   INITIALIZE   ";
            flashLine2 = "  COMPLETED !   ";
            flashTicks = 28;
        }
        else
        {
            currentScreen = ParamScreen::Default;
        }
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::CopyWaitSrc || currentScreen == ParamScreen::CopyWaitDst)
    {
        if (dir < 0)
        {
            copySrcLine = -1;
            currentScreen = ParamScreen::Default;
            repaint();
        }
        return;
    }

    int maxField = 0;
    switch (currentScreen)
    {
        case ParamScreen::Name:     maxField = 15; break;
        case ParamScreen::Waveform:
        case ParamScreen::DcoEnv:
        case ParamScreen::DcwEnv:
        case ParamScreen::DcaEnv:   maxField = 1;  break;
        case ParamScreen::Velocity: maxField = 2;  break;
        case ParamScreen::Vibrato:
        case ParamScreen::Detune:   maxField = 3;  break;
        default: break;
    }

    if (maxField == 0) return;
    cursorField += dir;
    if (cursorField < 0) cursorField = maxField;
    if (cursorField > maxField) cursorField = 0;
    repaint();
}

void CZ1AudioProcessorEditor::changePage (int dir)
{
    if (currentScreen == ParamScreen::Default)
    {
        currentScreen = ParamScreen::Chorus;
        repaint();
        return;
    }
    if (currentScreen == ParamScreen::Chorus)
    {
        currentScreen = ParamScreen::Default;
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::DcoEnv || currentScreen == ParamScreen::DcwEnv || currentScreen == ParamScreen::DcaEnv)
    {
        const auto& line = audioProcessor.getEngine().getCurrentPresetDef().lines[activeLine];
        const EnvelopeDef& env = (currentScreen == ParamScreen::DcoEnv) ? line.dco
                               : (currentScreen == ParamScreen::DcwEnv) ? line.dcw
                               : line.dca;
        const int endStep = std::clamp (static_cast<int> (env.end) + 1, 1, 8);
        if (dir > 0) envStep = std::min (endStep, envStep + 1);
        else         envStep = std::max (1, envStep - 1);
        repaint();
    }
}

void CZ1AudioProcessorEditor::toggleSustainPoint()
{
    if (currentScreen != ParamScreen::DcoEnv && currentScreen != ParamScreen::DcwEnv && currentScreen != ParamScreen::DcaEnv)
        return;

    const juce::ScopedLock sl (audioProcessor.getEngineLock());
    auto& line = audioProcessor.getEngine().getCurrentPresetMutable().lines[activeLine];
    EnvelopeDef& env = (currentScreen == ParamScreen::DcoEnv) ? line.dco
                     : (currentScreen == ParamScreen::DcwEnv) ? line.dcw
                     : line.dca;
    const int endStep = std::clamp (static_cast<int> (env.end) + 1, 1, 8);
    const int st = std::clamp (envStep, 1, endStep) - 1;

    if (env.sustain == st)
    {
        env.sustain = -1;
        for (int i = 0; i < 8; ++i) env.steps[i].sustain = false;
    }
    else
    {
        env.sustain = static_cast<int8_t> (st);
        for (int i = 0; i < 8; ++i) env.steps[i].sustain = (i == st);
    }
    audioProcessor.notifyLiveEdit();
    repaint();
}

void CZ1AudioProcessorEditor::toggleEndPoint()
{
    if (currentScreen != ParamScreen::DcoEnv && currentScreen != ParamScreen::DcwEnv && currentScreen != ParamScreen::DcaEnv)
        return;

    const juce::ScopedLock sl (audioProcessor.getEngineLock());
    auto& line = audioProcessor.getEngine().getCurrentPresetMutable().lines[activeLine];
    EnvelopeDef& env = (currentScreen == ParamScreen::DcoEnv) ? line.dco
                     : (currentScreen == ParamScreen::DcwEnv) ? line.dcw
                     : line.dca;
    const int st = std::clamp (envStep, 1, 8) - 1;

    if (env.end == st)
    {
        if (st < 7)
        {
            env.end = 7;
            env.steps[7].level = 0;
        }
    }
    else
    {
        env.end = static_cast<uint8_t> (st);
        env.steps[st].level = 0;
        if (env.sustain > st)
        {
            env.sustain = -1;
            for (int i = 0; i < 8; ++i) env.steps[i].sustain = false;
        }
    }
    audioProcessor.notifyLiveEdit();
    repaint();
}

void CZ1AudioProcessorEditor::onLineParamClicked (int lineIdx, LineParamKind kind)
{
    auto screenForKind = [] (LineParamKind k) -> ParamScreen
    {
        switch (k)
        {
            case LP_Waveform: return ParamScreen::Waveform;
            case LP_DcoEnv:   return ParamScreen::DcoEnv;
            case LP_DcwKf:    return ParamScreen::DcwKeyFollow;
            case LP_DcwEnv:   return ParamScreen::DcwEnv;
            case LP_DcaKf:    return ParamScreen::DcaKeyFollow;
            case LP_DcaEnv:   return ParamScreen::DcaEnv;
            case LP_Level:    return ParamScreen::Level;
            case LP_Velocity: return ParamScreen::Velocity;
            default:          return ParamScreen::Default;
        }
    };

    if (currentScreen == ParamScreen::InitWait)
    {
        const juce::ScopedLock sl (audioProcessor.getEngineLock());
        auto& line = audioProcessor.getEngine().getCurrentPresetMutable().lines[lineIdx];
        switch (kind)
        {
            case LP_Waveform: updateLineRawWord (line, 1, 0, (line.rawWord >> 5) & 1, ((line.rawWord >> 3) & 3) == 3); break;
            case LP_DcoEnv:   line.dco = createInitEnv (0); break;
            case LP_DcwKf:    line.dcwKeyFollow = 0; break;
            case LP_DcwEnv:   line.dcw = createInitEnv (1); break;
            case LP_DcaKf:    line.dcaKeyFollow = 0; break;
            case LP_DcaEnv:   line.dca = createInitEnv (2); break;
            case LP_Level:    line.lineLevel = 15; break;
            case LP_Velocity: line.dcaVelocity = line.dcwVelocity = line.dcoVelocity = 0; break;
            default: break;
        }
        audioProcessor.notifyLiveEdit();
        activeLine = lineIdx;
        envStep = 1;
        cursorField = 0;
        currentScreen = screenForKind (kind);
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::CopyWaitSrc)
    {
        copySrcLine = lineIdx;
        copySrcKind = kind;
        currentScreen = ParamScreen::CopyWaitDst;
        repaint();
        return;
    }

    if (currentScreen == ParamScreen::CopyWaitDst && copySrcLine != -1)
    {
        if (copySrcLine != lineIdx && copySrcKind == kind)
        {
            const juce::ScopedLock sl (audioProcessor.getEngineLock());
            auto& lines = audioProcessor.getEngine().getCurrentPresetMutable().lines;
            const auto& src = lines[copySrcLine];
            auto& dst = lines[lineIdx];
            switch (kind)
            {
                case LP_Waveform:
                {
                    const bool isRing = (dst.rawWord >> 5) & 1;
                    const bool isNoise = ((dst.rawWord >> 3) & 3) == 3;
                    const bool hasW2 = ((src.rawWord >> 9) & 1) != 0;
                    updateLineRawWord (dst, chipWaveToPanelWave (src.wave1, src.window), hasW2 ? chipWaveToPanelWave (src.wave2, src.window) : 0, isRing, isNoise);
                    break;
                }
                case LP_DcoEnv:   dst.dco = src.dco; break;
                case LP_DcwKf:    dst.dcwKeyFollow = src.dcwKeyFollow; break;
                case LP_DcwEnv:   dst.dcw = src.dcw; break;
                case LP_DcaKf:    dst.dcaKeyFollow = src.dcaKeyFollow; break;
                case LP_DcaEnv:   dst.dca = src.dca; break;
                case LP_Level:    dst.lineLevel = src.lineLevel; break;
                case LP_Velocity: dst.dcaVelocity = src.dcaVelocity; dst.dcwVelocity = src.dcwVelocity; dst.dcoVelocity = src.dcoVelocity; break;
                default: break;
            }
            audioProcessor.notifyLiveEdit();
            copySrcLine = -1;
            activeLine = lineIdx;
            envStep = 1;
            cursorField = 0;
            currentScreen = screenForKind (kind);
            repaint();
            return;
        }
        copySrcLine = lineIdx;
        copySrcKind = kind;
        repaint();
        return;
    }

    const ParamScreen target = screenForKind (kind);
    if (currentScreen == target && activeLine == lineIdx)
    {
        currentScreen = ParamScreen::Default;
    }
    else
    {
        activeLine = lineIdx;
        envStep = 1;
        cursorField = 0;
        currentScreen = target;
    }
    syncKeyboardFocusForScreen();
    repaint();
}

void CZ1AudioProcessorEditor::onGlobalParamClicked (ParamScreen target)
{
    if (currentScreen == ParamScreen::InitWait)
    {
        if (target == ParamScreen::Name)
        {
            flashLine1 = "   INITIALIZE   ";
            flashLine2 = "NAME UNCHANGED! ";
            flashTicks = 25;
            repaint();
            return;
        }
        const juce::ScopedLock sl (audioProcessor.getEngineLock());
        auto& engine = audioProcessor.getEngine();
        if (target == ParamScreen::Vibrato) engine.getCurrentVibratoMutable() = { 1, 0, 50, 0 };
        if (target == ParamScreen::Octave)  engine.getCurrentPresetMutable().octave = 0;
        if (target == ParamScreen::Detune)  engine.getCurrentDetuneMutable() = { 1, 0, 0, 0 };
        audioProcessor.notifyLiveEdit();
        cursorField = 0;
        currentScreen = target;
        repaint();
        return;
    }

    copySrcLine = -1;
    if (currentScreen == target)
    {
        currentScreen = ParamScreen::Default;
    }
    else
    {
        cursorField = (target == ParamScreen::Detune) ? 1 : 0;
        currentScreen = target;
    }
    syncKeyboardFocusForScreen();
    repaint();
}

void CZ1AudioProcessorEditor::onLineSelectClicked()
{
    const juce::ScopedLock sl (audioProcessor.getEngineLock());
    auto& preset = audioProcessor.getEngine().getCurrentPresetMutable();
    // Manual p.10: 1 (0) -> 2 (1) -> 1+2' (3) -> 1+1' (2) -> 1 (0)
    int next = 0;
    if      (preset.lineMode == 0) next = 1;
    else if (preset.lineMode == 1) next = 3;
    else if (preset.lineMode == 3) next = 2;
    else                           next = 0;

    preset.lineMode = static_cast<int8_t> (next);
    if (next == 0 || next == 1)
    {
        auto& l1 = preset.lines[0];
        const bool hasW2 = ((l1.rawWord >> 9) & 1) != 0;
        updateLineRawWord (l1, chipWaveToPanelWave (l1.wave1, l1.window), hasW2 ? chipWaveToPanelWave (l1.wave2, l1.window) : 0, false, false);
    }
    audioProcessor.notifyLiveEdit();
    repaint();
}

void CZ1AudioProcessorEditor::onRingClicked()
{
    const juce::ScopedLock sl (audioProcessor.getEngineLock());
    auto& preset = audioProcessor.getEngine().getCurrentPresetMutable();
    if (preset.lineMode != 2 && preset.lineMode != 3)
    {
        flashLine1 = "RING MODULATION ";
        flashLine2 = "SET 1+2' OR 1+1'";
        flashTicks = 28;
        repaint();
        return;
    }
    auto& l1 = preset.lines[0];
    const bool curRing = ((l1.rawWord >> 5) & 1) != 0;
    const bool hasW2 = ((l1.rawWord >> 9) & 1) != 0;
    updateLineRawWord (l1, chipWaveToPanelWave (l1.wave1, l1.window), hasW2 ? chipWaveToPanelWave (l1.wave2, l1.window) : 0, !curRing, false);
    audioProcessor.notifyLiveEdit();
    repaint();
}

void CZ1AudioProcessorEditor::onNoiseClicked()
{
    const juce::ScopedLock sl (audioProcessor.getEngineLock());
    auto& preset = audioProcessor.getEngine().getCurrentPresetMutable();
    if (preset.lineMode != 2 && preset.lineMode != 3)
    {
        flashLine1 = "NOISE MODULATION";
        flashLine2 = "SET 1+2' OR 1+1'";
        flashTicks = 28;
        repaint();
        return;
    }
    auto& l1 = preset.lines[0];
    const bool curNoise = ((l1.rawWord >> 3) & 3) == 3;
    const bool hasW2 = ((l1.rawWord >> 9) & 1) != 0;
    updateLineRawWord (l1, chipWaveToPanelWave (l1.wave1, l1.window), hasW2 ? chipWaveToPanelWave (l1.wave2, l1.window) : 0, false, !curNoise);
    audioProcessor.notifyLiveEdit();
    repaint();
}

void CZ1AudioProcessorEditor::onCopyClicked()
{
    copySrcLine = -1;
    currentScreen = (currentScreen == ParamScreen::CopyWaitSrc || currentScreen == ParamScreen::CopyWaitDst)
                  ? ParamScreen::Default : ParamScreen::CopyWaitSrc;
    syncKeyboardFocusForScreen();
    repaint();
}

void CZ1AudioProcessorEditor::onInitClicked()
{
    copySrcLine = -1;
    currentScreen = (currentScreen == ParamScreen::InitWait) ? ParamScreen::Default : ParamScreen::InitWait;
    syncKeyboardFocusForScreen();
    repaint();
}

CZ1AudioProcessorEditor::LcdDisplayState CZ1AudioProcessorEditor::buildLcdState() const
{
    if (flashTicks > 0)
        return { pad16 (flashLine1), pad16 (flashLine2), -1, 0, 0 };

    const auto& engine = audioProcessor.getEngine();
    const auto& preset = engine.getCurrentPresetDef();
    const auto& line = preset.lines[activeLine];
    const auto& vib = engine.getCurrentVibrato();
    const auto& det = engine.getCurrentDetune();

    switch (currentScreen)
    {
        case ParamScreen::Default:
            return { pad16 ("*INTERNAL* " + juce::String (preset.slot)), pad16 (juce::String (preset.name).toUpperCase()), -1, 0, 0 };

        case ParamScreen::Chorus:
            return { pad16 ("     CHORUS"), pad16 (engine.isChorusEnabled() ? "      ON" : "      OFF"), -1, 0, 0 };

        case ParamScreen::Name:
            return { pad16 ("*NAME*     " + juce::String (preset.slot)), pad16 (juce::String (preset.name).toUpperCase()), 1, std::clamp (cursorField, 0, 15), 1 };

        case ParamScreen::Waveform:
        {
            const int w1 = chipWaveToPanelWave (line.wave1, line.window);
            const bool hasW2 = ((line.rawWord >> 9) & 1) != 0;
            const int w2 = hasW2 ? chipWaveToPanelWave (line.wave2, line.window) : 0;
            return { pad16 ("WAVE FORM L" + juce::String (activeLine + 1)),
                     pad16 ("FIRST=" + juce::String (w1) + " SECOND=" + juce::String (w2)),
                     1, (cursorField == 0) ? 6 : 15, 1 };
        }

        case ParamScreen::DcoEnv:
        case ParamScreen::DcwEnv:
        case ParamScreen::DcaEnv:
        {
            const juce::String prefix = (currentScreen == ParamScreen::DcoEnv) ? "PITCH"
                                      : (currentScreen == ParamScreen::DcwEnv) ? "WAVE "
                                      : "AMP  ";
            const EnvelopeDef& env = (currentScreen == ParamScreen::DcoEnv) ? line.dco
                                   : (currentScreen == ParamScreen::DcwEnv) ? line.dcw
                                   : line.dca;
            const int endStep = std::clamp (static_cast<int> (env.end) + 1, 1, 8);
            const int curSt = std::clamp (envStep, 1, endStep);
            const auto& st = env.steps[curSt - 1];
            const juce::String tag = (curSt == endStep) ? "END" : ((env.sustain == curSt - 1) ? "SUS" : "***");
            const int r = (currentScreen == ParamScreen::DcaEnv) ? sysexToDcaRate (st.rate)
                        : (currentScreen == ParamScreen::DcwEnv) ? sysexToDcwRate (st.rate)
                        : sysexToDcoRate (st.rate);
            const int l = (curSt == endStep) ? 0
                        : ((currentScreen == ParamScreen::DcaEnv) ? sysexToDcaLevel (st.level)
                        : (currentScreen == ParamScreen::DcwEnv) ? sysexToDcwLevel (st.level)
                        : sysexToDcoLevel (st.level));
            return { pad16 (prefix + " STEP" + juce::String (curSt) + "  " + tag),
                     pad16 ("RATE=" + fmt2 (r) + " LEVEL=" + fmt2 (l)),
                     1, (cursorField == 0) ? 5 : 14, 2 };
        }

        case ParamScreen::DcwKeyFollow:
            return { pad16 ("  KEY FOLLOW L" + juce::String (activeLine + 1)), pad16 ("WAVE RANGE=" + juce::String (line.dcwKeyFollow)), 1, 11, 1 };

        case ParamScreen::DcaKeyFollow:
            return { pad16 ("  KEY FOLLOW L" + juce::String (activeLine + 1)), pad16 ("AMP  RANGE=" + juce::String (line.dcaKeyFollow)), 1, 11, 1 };

        case ParamScreen::Level:
        {
            const int curLvl = (line.lineLevel == 0) ? 15 : std::clamp (static_cast<int> (line.lineLevel), 1, 15);
            return { pad16 ("LINE" + juce::String (activeLine + 1) + " VOLUME"), pad16 ("    LEVEL=" + fmt2 (curLvl)), 1, 10, 2 };
        }

        case ParamScreen::Velocity:
        {
            const int row = (cursorField == 0) ? 0 : 1;
            const int st = (cursorField == 0) ? 13 : ((cursorField == 1) ? 5 : 14);
            return { pad16 ("VELOCITY AMP=" + fmt2 (line.dcaVelocity)),
                     pad16 ("WAVE=" + fmt2 (line.dcwVelocity) + " PITCH=" + fmt2 (line.dcoVelocity)),
                     row, st, 2 };
        }

        case ParamScreen::Vibrato:
        {
            const int row = (cursorField <= 1) ? 0 : 1;
            const int st = (cursorField == 0) ? 5 : ((cursorField == 1) ? 14 : ((cursorField == 2) ? 5 : 14));
            const int len = (cursorField == 0) ? 1 : 2;
            return { pad16 ("WAVE=" + juce::String (vib.wave) + "  DELAY=" + fmt2 (vib.delay)),
                     pad16 ("RATE=" + fmt2 (vib.rate) + " DEPTH=" + fmt2 (vib.depth)),
                     row, st, len };
        }

        case ParamScreen::Octave:
        {
            const juce::String s = (preset.octave > 0) ? "+1" : ((preset.octave < 0) ? "-1" : "+0");
            return { pad16 ("    OCTAVE"), pad16 ("   RANGE=" + s), 1, 9, 2 };
        }

        case ParamScreen::Detune:
        {
            const juce::String sChar = (det.sign >= 0) ? "+" : "-";
            const int row = (cursorField <= 1) ? 0 : 1;
            const int st = (cursorField == 0) ? 8 : ((cursorField == 1) ? 15 : ((cursorField == 2) ? 5 : 13));
            const int len = (cursorField <= 1) ? 1 : 2;
            return { pad16 ("DETUNE (" + sChar + ") OCT=" + juce::String (det.oct)),
                     pad16 ("NOTE=" + fmt2 (det.note) + " FINE=" + fmt2 (det.fine)),
                     row, st, len };
        }

        case ParamScreen::CopyWaitSrc:
            return { pad16 ("     COPY"), pad16 ("SELECT PARAMETER"), -1, 0, 0 };

        case ParamScreen::CopyWaitDst:
            return { pad16 ("COPY LINE" + juce::String (copySrcLine + 1) + "->LINE" + juce::String (copySrcLine == 0 ? 2 : 1)),
                     pad16 ("SELECT DEST KEY"), -1, 0, 0 };

        case ParamScreen::InitWait:
            return { pad16 ("   INITIALIZE"), pad16 ("PARAM? / YES=ALL"), -1, 0, 0 };
    }

    return { pad16 ("CASIO CZ-1"), pad16 (""), -1, 0, 0 };
}

void CZ1AudioProcessorEditor::drawLed (juce::Graphics& g, juce::Rectangle<int> bounds, bool lit, juce::Colour litColour)
{
    g.setColour (lit ? litColour : juce::Colour (0xff2c171a));
    g.fillRect (bounds);
    g.setColour (lit ? litColour.brighter (0.4f) : juce::Colour (0xff432b2e));
    g.drawRect (bounds, 1);
}

// ==============================================================================
// Paint & Resized
// ==============================================================================

bool CZ1AudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (f.endsWithIgnoreCase (".syx") || f.endsWithIgnoreCase (".bin"))
            return true;
    return false;
}

void CZ1AudioProcessorEditor::fileDragEnter (const juce::StringArray&, int, int) { isDraggingFile = true; repaint(); }
void CZ1AudioProcessorEditor::fileDragExit (const juce::StringArray&) { isDraggingFile = false; repaint(); }

void CZ1AudioProcessorEditor::filesDropped (const juce::StringArray& files, int, int)
{
    isDraggingFile = false;
    for (const auto& f : files)
    {
        if (f.endsWithIgnoreCase (".syx") || f.endsWithIgnoreCase (".bin"))
        {
            if (audioProcessor.loadSyxFile (juce::File (f)))
            {
                refreshPresetList();
                currentScreen = ParamScreen::Default;
                syncKeyboardFocusForScreen();
                break;
            }
        }
    }
    repaint();
}

void CZ1AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff12141a));

    // Signature CZ-1 Top Accent Stripes
    g.setColour (juce::Colour (0xffd32f2f));
    g.fillRect (0, 0, getWidth(), 4);
    g.setColour (juce::Colour (0xff0288d1));
    g.fillRect (0, 4, getWidth(), 3);

    // Header Logo
    g.setColour (juce::Colour (0xff00e5ff));
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawText ("CZ-1", 20, 26, 80, 30, juce::Justification::left);

    // Header Panel Separator
    g.setColour (juce::Colour (0xff262b36));
    g.drawLine (20.0f, 84.0f, static_cast<float> (getWidth() - 20), 84.0f, 1.0f);

    // ---- DATA ENTRY SECTION PANEL (Left: x=20..360, y=94..492) ----
    const juce::Rectangle<int> dataPanel (20, 94, 340, 398);
    g.setColour (juce::Colour (0xff171a22));
    g.fillRoundedRectangle (dataPanel.toFloat(), 6.0f);
    g.setColour (juce::Colour (0xff2a303d));
    g.drawRoundedRectangle (dataPanel.toFloat(), 6.0f, 1.0f);

    g.setColour (juce::Colours::black);
    g.fillRect (32, 104, 110, 18);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("DATA ENTRY", 32, 104, 110, 18, juce::Justification::centred);

    // 16x2 Backlit LCD Frame
    const juce::Rectangle<int> lcdBezel (32, 134, 316, 98);
    g.setColour (juce::Colour (0xff0a0d12));
    g.fillRoundedRectangle (lcdBezel.toFloat(), 4.0f);
    g.setColour (juce::Colour (0xff3a4252));
    g.drawRoundedRectangle (lcdBezel.toFloat(), 4.0f, 1.0f);

    g.setColour (juce::Colour (0xff94a3b8));
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    g.drawText ("DIGITAL SYNTHESIZER", 40, 138, 300, 14, juce::Justification::centred);

    const juce::Rectangle<int> lcdGlass (42, 155, 296, 66);
    g.setColour (juce::Colour (0xff001a1e));
    g.fillRoundedRectangle (lcdGlass.toFloat(), 3.0f);
    g.setColour (juce::Colour (0xff0d4d52));
    g.drawRoundedRectangle (lcdGlass.toFloat(), 3.0f, 1.5f);

    // Draw 16x2 grid cells with cursor highlight
    const LcdDisplayState lcd = buildLcdState();
    const float cellW = (lcdGlass.getWidth() - 12.0f) / 16.0f;
    const float cellH = 26.0f;
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 16.0f, juce::Font::bold));

    for (int row = 0; row < 2; ++row)
    {
        const juce::String& rowText = (row == 0) ? lcd.line1 : lcd.line2;
        const float yPos = lcdGlass.getY() + 6.0f + row * 28.0f;
        for (int col = 0; col < 16; ++col)
        {
            const float xPos = lcdGlass.getX() + 6.0f + col * cellW;
            const bool isCursor = (lcd.cursorRow == row && col >= lcd.cursorStart && col < lcd.cursorStart + lcd.cursorLen);
            if (isCursor)
            {
                g.setColour (juce::Colour (0x3300e5ff));
                g.fillRect (xPos, yPos, cellW - 1.0f, cellH);
                g.setColour (juce::Colour (0xff80deea));
                g.fillRect (xPos, yPos + cellH - 2.0f, cellW - 1.0f, 2.0f);
            }
            g.setColour (isCursor ? juce::Colour (0xffccf7ff) : juce::Colour (0xff00e5ff));
            g.drawText (rowText.substring (col, col + 1),
                        static_cast<int> (xPos), static_cast<int> (yPos),
                        static_cast<int> (std::ceil (cellW)), static_cast<int> (cellH),
                        juce::Justification::centred);
        }
    }

    // Data Entry Group Labels under buttons
    g.setColour (juce::Colour (0xffcbd5e1));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("SAVE        VALUE        LOAD", 32, 316, 152, 16, juce::Justification::centred);
    g.drawText ("NO          CURSOR        YES", 196, 316, 152, 16, juce::Justification::centred);
    g.drawText ("PAGE", 32, 412, 152, 16, juce::Justification::centred);
    g.drawText ("ENV POINT", 196, 412, 152, 16, juce::Justification::centred);

    // Status line at bottom of Data Entry
    g.setColour (currentScreen == ParamScreen::Name ? juce::Colour (0xff00e5ff) : juce::Colour (0xff64748b));
    g.setFont (juce::FontOptions (10.0f));
    const juce::String statusText = (currentScreen == ParamScreen::Name)
        ? "TYPE KEYBOARD TO RENAME  ·  ENTER = DONE"
        : ("VOICES ACTIVE: " + juce::String (audioProcessor.getEngine().getActiveVoiceCount()) + " / 8 DUAL");
    g.drawText (statusText, 32, 464, 316, 18, juce::Justification::centred);

    // ---- PARAMETER SECTION PANEL (Right: x=372..1000, y=94..492) ----
    const juce::Rectangle<int> paramPanel (372, 94, 628, 398);
    g.setColour (juce::Colour (0xff171a22));
    g.fillRoundedRectangle (paramPanel.toFloat(), 6.0f);
    g.setColour (juce::Colour (0xff2a303d));
    g.drawRoundedRectangle (paramPanel.toFloat(), 6.0f, 1.0f);

    g.setColour (juce::Colours::black);
    g.fillRect (386, 104, 110, 18);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("PARAMETER", 386, 104, 110, 18, juce::Justification::centred);

    const auto& curPreset = audioProcessor.getEngine().getCurrentPresetDef();
    g.setColour (juce::Colour (0xff94a3b8));
    g.setFont (juce::FontOptions (10.5f));
    g.drawText (CZ1Engine::getLineModeName (curPreset.lineMode), 506, 104, 480, 18, juce::Justification::right);

    // LED indicators on PARAMETER Top Row (y=134)
    drawLed (g, { 406, 134, 24, 6 }, currentScreen == ParamScreen::Name);

    // LINE SELECT 4 LEDs: 1, 2, 1+2', 1+1'
    const int modes[4] = { 0, 1, 3, 2 };
    const char* modeLbls[4] = { "1", "2", "1+2'", "1+1'" };
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    for (int m = 0; m < 4; ++m)
    {
        const int lx = 468 + m * 21;
        drawLed (g, { lx, 134, 16, 6 }, curPreset.lineMode == modes[m]);
        g.setColour (juce::Colour (0xffcbd5e1));
        g.drawText (modeLbls[m], lx - 2, 141, 20, 10, juce::Justification::centred);
    }

    // MODULATION Box around RING & NOISE
    g.setColour (juce::Colour (0xff2f3746));
    g.drawRoundedRectangle (564.0f, 126.0f, 148.0f, 70.0f, 4.0f, 1.0f);
    g.setColour (juce::Colour (0xff94a3b8));
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    g.drawText ("MODULATION", 564, 127, 148, 12, juce::Justification::centred);

    const bool isRing  = ((curPreset.lines[0].rawWord >> 5) & 1) != 0;
    const bool isNoise = ((curPreset.lines[0].rawWord >> 3) & 3) == 3;
    drawLed (g, { 590, 141, 22, 6 }, isRing);
    drawLed (g, { 662, 141, 22, 6 }, isNoise);

    // COPY & INITIALIZE LEDs (amber)
    drawLed (g, { 754, 134, 24, 6 }, currentScreen == ParamScreen::CopyWaitSrc || currentScreen == ParamScreen::CopyWaitDst, juce::Colour (0xfffbbf24));
    drawLed (g, { 844, 134, 24, 6 }, currentScreen == ParamScreen::InitWait, juce::Colour (0xfffbbf24));

    // VIBRATO & OCTAVE LEDs
    drawLed (g, { 406, 242, 24, 6 }, currentScreen == ParamScreen::Vibrato);
    drawLed (g, { 406, 356, 24, 6 }, currentScreen == ParamScreen::Octave);

    // DCO 1 / DCW 1 / DCA 1 & Line 1 badge
    auto drawHeaderStrip = [&g] (int x, int y, const char* lineTag, bool active)
    {
        g.setColour (juce::Colour (0xff232833));
        g.fillRect (x, y, 106, 16);
        g.fillRect (x + 112, y, 106, 16);
        g.fillRect (x + 224, y, 218, 16);
        g.setColour (juce::Colour (0xffe2e8f0));
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText ("DCO " + juce::String (lineTag), x, y, 106, 16, juce::Justification::centred);
        g.drawText ("DCW " + juce::String (lineTag), x + 112, y, 106, 16, juce::Justification::centred);
        g.drawText ("DCA " + juce::String (lineTag), x + 224, y, 196, 16, juce::Justification::centred);
        g.setColour (active ? juce::Colour (0xffd32f2f) : juce::Colour (0xff374151));
        g.fillRect (x + 422, y + 1, 18, 14);
        g.setColour (juce::Colours::white);
        g.drawText (lineTag, x + 422, y + 1, 18, 14, juce::Justification::centred);
    };

    drawHeaderStrip (466, 224, "1", curPreset.lineMode != 1);
    drawHeaderStrip (466, 444, "2", curPreset.lineMode != 0);

    // LEDs for Line 1 & Line 2 8-button matrix
    auto screenForIdx = [] (int i) -> ParamScreen
    {
        switch (i)
        {
            case 0: return ParamScreen::Waveform;
            case 1: return ParamScreen::DcoEnv;
            case 2: return ParamScreen::DcwKeyFollow;
            case 3: return ParamScreen::DcwEnv;
            case 4: return ParamScreen::DcaKeyFollow;
            case 5: return ParamScreen::DcaEnv;
            case 6: return ParamScreen::Level;
            case 7: return ParamScreen::Velocity;
            default: return ParamScreen::Default;
        }
    };

    for (int i = 0; i < LP_Count; ++i)
    {
        const int bx = 466 + i * 56;
        const ParamScreen s = screenForIdx (i);
        drawLed (g, { bx + 14, 248, 22, 6 }, currentScreen == s && activeLine == 0);
        drawLed (g, { bx + 14, 344, 22, 6 }, currentScreen == s && activeLine == 1);
    }

    // DETUNE 1 & 2 LEDs
    drawLed (g, { 930, 260, 18, 6 }, currentScreen == ParamScreen::Detune || curPreset.lineMode == 2);
    drawLed (g, { 958, 260, 18, 6 }, currentScreen == ParamScreen::Detune || curPreset.lineMode == 3);
    g.setColour (juce::Colour (0xffcbd5e1));
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText ("1", 930, 267, 18, 12, juce::Justification::centred);
    g.drawText ("2", 958, 267, 18, 12, juce::Justification::centred);

    // Drag-and-drop overlay
    if (isDraggingFile)
    {
        g.setColour (juce::Colour (0xcc001f24));
        g.fillAll();
        g.setColour (juce::Colour (0xff00e5ff));
        g.drawRect (8, 8, getWidth() - 16, getHeight() - 16, 3);
        g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
        g.drawText ("DROP CASIO CZ .SYX FILE HERE", 0, getHeight() / 2 - 20, getWidth(), 30, juce::Justification::centred);
    }
}

void CZ1AudioProcessorEditor::resized()
{
    // Top Header Controls
    presetLabel.setBounds    (112, 16, 256, 18);
    presetSelector.setBounds (112, 36, 256, 28);
    loadSyxButton.setBounds  (376, 36, 86, 28);
    saveSyxButton.setBounds  (470, 36, 86, 28);
    resetRomButton.setBounds (564, 36, 86, 28);
    chorusButton.setBounds   (662, 36, 124, 28);
    volumeLabel.setBounds    (798, 16, 202, 16);
    volumeSlider.setBounds   (798, 36, 202, 28);

    // DATA ENTRY SECTION Buttons
    homeLcdBtn.setBounds  (282, 103, 66, 20);
    valDownBtn.setBounds  (32, 266, 72, 44);
    valUpBtn.setBounds    (112, 266, 72, 44);
    curLeftBtn.setBounds  (196, 266, 72, 44);
    curRightBtn.setBounds (276, 266, 72, 44);

    pageDownBtn.setBounds (32, 362, 72, 44);
    pageUpBtn.setBounds   (112, 362, 72, 44);
    sustainBtn.setBounds  (196, 362, 72, 44);
    endBtn.setBounds      (276, 362, 72, 44);

    // PARAMETER SECTION Top Row
    nameBtn.setBounds       (386, 148, 64, 42);
    lineSelectBtn.setBounds (466, 154, 86, 36);
    ringBtn.setBounds       (574, 152, 58, 36);
    noiseBtn.setBounds      (644, 152, 58, 36);
    copyBtn.setBounds       (728, 148, 76, 42);
    initBtn.setBounds       (818, 148, 76, 42);

    // PARAMETER SECTION Matrix
    vibratoBtn.setBounds (386, 256, 64, 52);
    octaveBtn.setBounds  (386, 370, 64, 52);

    for (int i = 0; i < LP_Count; ++i)
    {
        const int bx = 466 + i * 56;
        line1Btns[i].setBounds (bx, 260, 50, 52);
        line2Btns[i].setBounds (bx, 356, 50, 52);
    }

    detuneBtn.setBounds (922, 284, 62, 120);

    // Bottom MIDI Keyboard
    keyboardComponent.setBounds (20, 508, getWidth() - 40, 114);
}
