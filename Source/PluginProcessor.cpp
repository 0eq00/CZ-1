/*
  ==============================================================================
    PluginProcessor.cpp
    Casio CZ-1 Phase Distortion Synthesizer Audio Processor Implementation
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cstring>

namespace
{
    void serializeEnvelope (juce::XmlElement& parent, const char* tagName, const EnvelopeDef& env)
    {
        auto* envEl = parent.createNewChildElement (tagName);
        envEl->setAttribute ("end", static_cast<int> (env.end));
        envEl->setAttribute ("sustain", static_cast<int> (env.sustain));
        for (int s = 0; s < 8; ++s)
        {
            auto* stEl = envEl->createNewChildElement ("Step");
            stEl->setAttribute ("index", s);
            stEl->setAttribute ("rate", static_cast<int> (env.steps[s].rate));
            stEl->setAttribute ("level", static_cast<int> (env.steps[s].level));
            stEl->setAttribute ("sustain", env.steps[s].sustain);
        }
    }

    void deserializeEnvelope (const juce::XmlElement* envEl, EnvelopeDef& env)
    {
        if (envEl == nullptr)
            return;

        env.end = static_cast<uint8_t> (std::clamp (envEl->getIntAttribute ("end", env.end), 0, 7));
        env.sustain = static_cast<int8_t> (std::clamp (envEl->getIntAttribute ("sustain", env.sustain), -1, 7));

        for (int i = 0; i < envEl->getNumChildElements(); ++i)
        {
            const auto* stEl = envEl->getChildElement (i);
            if (stEl != nullptr && stEl->hasTagName ("Step"))
            {
                const int s = stEl->getIntAttribute ("index", -1);
                if (s >= 0 && s < 8)
                {
                    env.steps[s].rate    = static_cast<uint8_t> (std::clamp (stEl->getIntAttribute ("rate", env.steps[s].rate), 0, 127));
                    env.steps[s].level   = static_cast<uint8_t> (std::clamp (stEl->getIntAttribute ("level", env.steps[s].level), 0, 127));
                    env.steps[s].sustain = stEl->getBoolAttribute ("sustain", env.steps[s].sustain);
                }
            }
        }
    }

    void serializeLine (juce::XmlElement& parent, int index, const PresetLine& line)
    {
        auto* lineEl = parent.createNewChildElement ("Line");
        lineEl->setAttribute ("index", index);
        lineEl->setAttribute ("rawWord", static_cast<int> (line.rawWord));
        lineEl->setAttribute ("wave1", static_cast<int> (line.wave1));
        lineEl->setAttribute ("wave2", static_cast<int> (line.wave2));
        lineEl->setAttribute ("window", static_cast<int> (line.window));
        lineEl->setAttribute ("modulation", static_cast<int> (line.modulation));
        lineEl->setAttribute ("lineLevel", static_cast<int> (line.lineLevel));
        lineEl->setAttribute ("dcaKeyFollow", static_cast<int> (line.dcaKeyFollow));
        lineEl->setAttribute ("dcwKeyFollow", static_cast<int> (line.dcwKeyFollow));
        lineEl->setAttribute ("dcaVelocity", static_cast<int> (line.dcaVelocity));
        lineEl->setAttribute ("dcwVelocity", static_cast<int> (line.dcwVelocity));
        lineEl->setAttribute ("dcoVelocity", static_cast<int> (line.dcoVelocity));

        serializeEnvelope (*lineEl, "DCO", line.dco);
        serializeEnvelope (*lineEl, "DCW", line.dcw);
        serializeEnvelope (*lineEl, "DCA", line.dca);
    }

    void deserializeLine (const juce::XmlElement* lineEl, PresetLine& line)
    {
        if (lineEl == nullptr)
            return;

        line.rawWord      = static_cast<uint16_t> (std::clamp (lineEl->getIntAttribute ("rawWord", line.rawWord), 0, 65535));
        line.wave1        = static_cast<uint8_t>  (std::clamp (lineEl->getIntAttribute ("wave1", line.wave1), 0, 7));
        line.wave2        = static_cast<uint8_t>  (std::clamp (lineEl->getIntAttribute ("wave2", line.wave2), 0, 7));
        line.window       = static_cast<uint8_t>  (std::clamp (lineEl->getIntAttribute ("window", line.window), 0, 7));
        line.modulation   = static_cast<uint8_t>  (std::clamp (lineEl->getIntAttribute ("modulation", line.modulation), 0, 7));
        line.lineLevel    = static_cast<uint8_t>  (std::clamp (lineEl->getIntAttribute ("lineLevel", line.lineLevel), 0, 15));
        line.dcaKeyFollow = static_cast<uint8_t>  (std::clamp (lineEl->getIntAttribute ("dcaKeyFollow", line.dcaKeyFollow), 0, 9));
        line.dcwKeyFollow = static_cast<uint8_t>  (std::clamp (lineEl->getIntAttribute ("dcwKeyFollow", line.dcwKeyFollow), 0, 9));
        line.dcaVelocity  = static_cast<uint8_t>  (std::clamp (lineEl->getIntAttribute ("dcaVelocity", line.dcaVelocity), 0, 15));
        line.dcwVelocity  = static_cast<uint8_t>  (std::clamp (lineEl->getIntAttribute ("dcwVelocity", line.dcwVelocity), 0, 15));
        line.dcoVelocity  = static_cast<uint8_t>  (std::clamp (lineEl->getIntAttribute ("dcoVelocity", line.dcoVelocity), 0, 15));

        deserializeEnvelope (lineEl->getChildByName ("DCO"), line.dco);
        deserializeEnvelope (lineEl->getChildByName ("DCW"), line.dcw);
        deserializeEnvelope (lineEl->getChildByName ("DCA"), line.dca);
    }

    void serializeCurrentPatch (juce::XmlElement& root, const CZ1Engine& engine)
    {
        const auto& preset = engine.getCurrentPresetDef();
        const auto& vib    = engine.getCurrentVibrato();
        const auto& det    = engine.getCurrentDetune();

        auto* patchEl = root.createNewChildElement ("CurrentPatch");
        patchEl->setAttribute ("index", engine.getCurrentPresetIndex());
        patchEl->setAttribute ("slot", juce::String (preset.slot));
        patchEl->setAttribute ("name", juce::String (preset.name));
        patchEl->setAttribute ("lineMode", static_cast<int> (preset.lineMode));
        patchEl->setAttribute ("octave", static_cast<int> (preset.octave));
        patchEl->setAttribute ("detuneSemitones", static_cast<double> (preset.detuneSemitones));

        auto* vibEl = patchEl->createNewChildElement ("Vibrato");
        vibEl->setAttribute ("wave", vib.wave);
        vibEl->setAttribute ("delay", vib.delay);
        vibEl->setAttribute ("rate", vib.rate);
        vibEl->setAttribute ("depth", vib.depth);

        auto* detEl = patchEl->createNewChildElement ("Detune");
        detEl->setAttribute ("sign", det.sign);
        detEl->setAttribute ("oct", det.oct);
        detEl->setAttribute ("note", det.note);
        detEl->setAttribute ("fine", det.fine);

        serializeLine (*patchEl, 0, preset.lines[0]);
        serializeLine (*patchEl, 1, preset.lines[1]);
    }

    void deserializeCurrentPatch (const juce::XmlElement* patchEl, CZ1Engine& engine)
    {
        if (patchEl == nullptr)
            return;

        const int index = patchEl->getIntAttribute ("index", engine.getCurrentPresetIndex());
        CZ1PresetDef preset = engine.getCurrentPresetDef();
        CZ1Engine::VibratoParams vib = engine.getCurrentVibrato();
        CZ1Engine::DetuneParams  det = engine.getCurrentDetune();

        if (patchEl->hasAttribute ("slot"))
        {
            const auto slotStr = patchEl->getStringAttribute ("slot");
            std::memset (preset.slot, 0, sizeof (preset.slot));
            std::strncpy (preset.slot, slotStr.toRawUTF8(), sizeof (preset.slot) - 1);
        }

        if (patchEl->hasAttribute ("name"))
        {
            const auto nameStr = patchEl->getStringAttribute ("name");
            std::memset (preset.name, 0, sizeof (preset.name));
            std::strncpy (preset.name, nameStr.toRawUTF8(), sizeof (preset.name) - 1);
        }

        preset.lineMode = static_cast<int8_t> (std::clamp (patchEl->getIntAttribute ("lineMode", preset.lineMode), 0, 3));
        preset.octave   = static_cast<int8_t> (std::clamp (patchEl->getIntAttribute ("octave", preset.octave), -1, 1));
        preset.detuneSemitones = static_cast<float> (patchEl->getDoubleAttribute ("detuneSemitones", preset.detuneSemitones));

        if (const auto* vibEl = patchEl->getChildByName ("Vibrato"))
        {
            vib.wave  = std::clamp (vibEl->getIntAttribute ("wave", vib.wave), 1, 4);
            vib.delay = std::clamp (vibEl->getIntAttribute ("delay", vib.delay), 0, 99);
            vib.rate  = std::clamp (vibEl->getIntAttribute ("rate", vib.rate), 0, 99);
            vib.depth = std::clamp (vibEl->getIntAttribute ("depth", vib.depth), 0, 99);
        }

        if (const auto* detEl = patchEl->getChildByName ("Detune"))
        {
            det.sign = (detEl->getIntAttribute ("sign", det.sign) < 0) ? -1 : 1;
            det.oct  = std::clamp (detEl->getIntAttribute ("oct", det.oct), 0, 3);
            det.note = std::clamp (detEl->getIntAttribute ("note", det.note), 0, 11);
            det.fine = std::clamp (detEl->getIntAttribute ("fine", det.fine), 0, 60);
        }

        for (int i = 0; i < patchEl->getNumChildElements(); ++i)
        {
            const auto* lineEl = patchEl->getChildElement (i);
            if (lineEl != nullptr && lineEl->hasTagName ("Line"))
            {
                const int lIdx = lineEl->getIntAttribute ("index", -1);
                if (lIdx == 0 || lIdx == 1)
                    deserializeLine (lineEl, preset.lines[lIdx]);
            }
        }

        engine.restorePresetAt (index, preset, vib, det);
    }
}

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

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"chorusDepth", 1},
        "Chorus Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.4f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"dcw1DepthOffset", 1},
        "DCW1 Depth Offset",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"dcw2DepthOffset", 1},
        "DCW2 Depth Offset",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f),
        0.0f));

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
    volumeParam          = parameters.getRawParameterValue ("volume");
    chorusParam          = parameters.getRawParameterValue ("chorus");
    chorusDepthParam     = parameters.getRawParameterValue ("chorusDepth");
    dcw1DepthOffsetParam = parameters.getRawParameterValue ("dcw1DepthOffset");
    dcw2DepthOffsetParam = parameters.getRawParameterValue ("dcw2DepthOffset");
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
    const juce::ScopedLock sl (engineLock);
    return std::max (CZ1Engine::MAX_PROGRAMS, engine.getPresetCount());
}

int CZ1AudioProcessor::getCurrentProgram()
{
    const juce::ScopedLock sl (engineLock);
    return engine.getCurrentPresetIndex();
}

void CZ1AudioProcessor::setCurrentProgram (int index)
{
    const juce::ScopedLock sl (engineLock);
    if (index < 0 || index >= engine.getPresetCount())
        return;
    if (index == engine.getCurrentPresetIndex())
        return;
    engine.loadPreset (index);
    ++stateRevision;
}

const juce::String CZ1AudioProcessor::getProgramName (int index)
{
    const juce::ScopedLock sl (engineLock);
    if (index < 0)
        return {};
    if (index < engine.getPresetCount())
        return juce::String (engine.getPresetSlot (index)) + ": " + juce::String (engine.getPresetName (index));

    if (index < 208)
    {
        const char bankLetter = static_cast<char> ('A' + (index / 8));
        const int toneNum = (index % 8) + 1;
        return juce::String::charToString (bankLetter) + "-" + juce::String (toneNum) + ": ---";
    }
    return "Slot " + juce::String (index + 1) + ": ---";
}

void CZ1AudioProcessor::changeProgramName (int, const juce::String&)
{
}

void CZ1AudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    const juce::ScopedLock sl (engineLock);
    resampler.prepare (CZ1Engine::NATIVE_RATE, sampleRate);
    engine.reset();
}

void CZ1AudioProcessor::releaseResources()
{
    const juce::ScopedLock sl (engineLock);
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
    const juce::ScopedLock sl (engineLock);
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update parameters
    if (chorusParam != nullptr)
        engine.setChorusEnabled (*chorusParam > 0.5f);
    if (chorusDepthParam != nullptr)
        engine.setChorusDepth (*chorusDepthParam);
    if (dcw1DepthOffsetParam != nullptr)
        engine.setDcw1DepthOffset (dcw1DepthOffsetParam->load());
    if (dcw2DepthOffsetParam != nullptr)
        engine.setDcw2DepthOffset (dcw2DepthOffsetParam->load());

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
        else if (msg.isController())
        {
            engine.handleControlChange (msg.getControllerNumber(), msg.getControllerValue());
        }
        else if (msg.isPitchWheel())
        {
            const float bendNorm = static_cast<float> (msg.getPitchWheelValue() - 8192) / 8192.0f;
            engine.setPitchBend (bendNorm);
        }
        else if (msg.isChannelPressure())
        {
            const float atNorm = static_cast<float> (msg.getChannelPressureValue()) / 127.0f;
            engine.setAfterTouch (atNorm);
        }
        else if (msg.isAftertouch())
        {
            const float atNorm = static_cast<float> (msg.getAfterTouchValue()) / 127.0f;
            engine.setAfterTouch (atNorm);
        }
        else if (msg.isProgramChange())
        {
            engine.loadPreset (msg.getProgramChangeNumber());
        }
        else if (msg.isAllNotesOff())
        {
            engine.allNotesOff();
        }
        else if (msg.isSysEx())
        {
            if (engine.loadSyxData (msg.getSysExData(), static_cast<size_t> (msg.getSysExDataSize()), "SysEx Dump"))
            {
                updateHostDisplay();
            }
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
    auto root = std::make_unique<juce::XmlElement> ("CZ1State");
    root->setAttribute ("version", 2);

    // 1. APVTS Parameters (Master Volume & Stereo Chorus)
    {
        auto state = parameters.copyState();
        if (auto apvtsXml = state.createXml())
            root->addChildElement (apvtsXml.release());
    }

    const juce::ScopedLock sl (engineLock);

    const float curVol = (volumeParam != nullptr) ? volumeParam->load() : 0.85f;
    const bool curChorus = (chorusParam != nullptr) ? (*chorusParam > 0.5f) : engine.isChorusEnabled();
    const float curChorusDepth = (chorusDepthParam != nullptr) ? chorusDepthParam->load() : engine.getChorusDepth();
    const float curDcw1Offset = (dcw1DepthOffsetParam != nullptr) ? dcw1DepthOffsetParam->load() : engine.getDcw1DepthOffset();
    const float curDcw2Offset = (dcw2DepthOffsetParam != nullptr) ? dcw2DepthOffsetParam->load() : engine.getDcw2DepthOffset();
    root->setAttribute ("masterVolume", static_cast<double> (curVol));
    root->setAttribute ("chorusEnabled", curChorus);
    root->setAttribute ("chorusDepth", static_cast<double> (curChorusDepth));
    root->setAttribute ("dcw1DepthOffset", static_cast<double> (curDcw1Offset));
    root->setAttribute ("dcw2DepthOffset", static_cast<double> (curDcw2Offset));

    // 2. EFFECT SECTION & OVERALL ON/OFF switches
    const auto& eff = engine.getEffectSettings();
    auto* effEl = root->createNewChildElement ("EffectSettings");
    effEl->setAttribute ("modulationOn",       eff.modulationOn);
    effEl->setAttribute ("portamentoOn",       eff.portamentoOn);
    effEl->setAttribute ("glideOn",            eff.glideOn);
    effEl->setAttribute ("soloOn",             eff.soloOn);
    effEl->setAttribute ("portamentoSweep",    eff.portamentoSweep);
    effEl->setAttribute ("portamentoTime",     eff.portamentoTime);
    effEl->setAttribute ("glideNote",          eff.glideNote);
    effEl->setAttribute ("glideTime",          eff.glideTime);
    effEl->setAttribute ("bendRange",          eff.bendRange);
    effEl->setAttribute ("modWheelDepth",      eff.modWheelDepth);
    effEl->setAttribute ("modAfterTouchDepth", eff.modAfterTouchDepth);
    effEl->setAttribute ("ampAfterTouchRange", eff.ampAfterTouchRange);

    // 3. Bank & Selected Preset Program
    const int curIdx = engine.getCurrentPresetIndex();
    const bool isInitBank = engine.isInitBankExcept (curIdx);
    auto* bankEl = root->createNewChildElement ("Bank");
    bankEl->setAttribute ("name", juce::String (engine.getBankName()));
    bankEl->setAttribute ("format", juce::String (engine.getBankFormat()));
    bankEl->setAttribute ("currentPresetIndex", curIdx);
    bankEl->setAttribute ("isInitBank", isInitBank);
    if (!isInitBank)
    {
        const auto syxBytes = engine.exportBankToSyx();
        if (!syxBytes.empty())
        {
            juce::MemoryBlock syxBlock (syxBytes.data(), syxBytes.size());
            bankEl->setAttribute ("syxBase64", syxBlock.toBase64Encoding());
        }
    }

    // 4. PARAMETER SECTION (Current Patch full lossless state)
    serializeCurrentPatch (*root, engine);

    copyXmlToBinary (*root, destData);
}

void CZ1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState == nullptr)
        return;

    const bool isNewRoot    = xmlState->hasTagName ("CZ1State");
    const bool isLegacyRoot = xmlState->hasTagName (parameters.state.getType());
    if (!isNewRoot && !isLegacyRoot)
        return;

    // 1. Restore APVTS Parameters (Master Volume & Stereo Chorus)
    juce::XmlElement* apvtsXml = isNewRoot
        ? xmlState->getChildByName (parameters.state.getType())
        : xmlState.get();

    bool restoredVolumeFromApvts = false;
    bool restoredChorusFromApvts = false;
    bool restoredChorusDepthFromApvts = false;
    bool restoredDcw1OffsetFromApvts = false;
    bool restoredDcw2OffsetFromApvts = false;

    if (apvtsXml != nullptr)
    {
        for (int i = 0; i < apvtsXml->getNumChildElements(); ++i)
        {
            const auto* child = apvtsXml->getChildElement (i);
            if (child != nullptr && child->hasTagName ("PARAM"))
            {
                const auto id = child->getStringAttribute ("id");
                if (id == "volume") restoredVolumeFromApvts = true;
                if (id == "chorus") restoredChorusFromApvts = true;
                if (id == "chorusDepth") restoredChorusDepthFromApvts = true;
                if (id == "dcw1DepthOffset") restoredDcw1OffsetFromApvts = true;
                if (id == "dcw2DepthOffset") restoredDcw2OffsetFromApvts = true;
            }
        }
        parameters.replaceState (juce::ValueTree::fromXml (*apvtsXml));
    }

    if (!restoredVolumeFromApvts && xmlState->hasAttribute ("masterVolume"))
    {
        if (auto* p = parameters.getParameter ("volume"))
        {
            const float v = std::clamp (static_cast<float> (xmlState->getDoubleAttribute ("masterVolume", 0.85)), 0.0f, 1.0f);
            p->setValueNotifyingHost (p->convertTo0to1 (v));
        }
    }

    if (!restoredChorusFromApvts && xmlState->hasAttribute ("chorusEnabled"))
    {
        if (auto* p = parameters.getParameter ("chorus"))
        {
            const bool c = xmlState->getBoolAttribute ("chorusEnabled", true);
            p->setValueNotifyingHost (c ? 1.0f : 0.0f);
        }
    }

    if (!restoredChorusDepthFromApvts)
    {
        if (auto* p = parameters.getParameter ("chorusDepth"))
        {
            const float d = xmlState->hasAttribute ("chorusDepth")
                ? std::clamp (static_cast<float> (xmlState->getDoubleAttribute ("chorusDepth", 0.4)), 0.0f, 1.0f)
                : 0.4f;
            p->setValueNotifyingHost (p->convertTo0to1 (d));
        }
    }

    if (!restoredDcw1OffsetFromApvts)
    {
        if (auto* p = parameters.getParameter ("dcw1DepthOffset"))
        {
            const float o1 = xmlState->hasAttribute ("dcw1DepthOffset")
                ? std::clamp (static_cast<float> (xmlState->getDoubleAttribute ("dcw1DepthOffset", 0.0)), -1.0f, 1.0f)
                : 0.0f;
            p->setValueNotifyingHost (p->convertTo0to1 (o1));
        }
    }

    if (!restoredDcw2OffsetFromApvts)
    {
        if (auto* p = parameters.getParameter ("dcw2DepthOffset"))
        {
            const float o2 = xmlState->hasAttribute ("dcw2DepthOffset")
                ? std::clamp (static_cast<float> (xmlState->getDoubleAttribute ("dcw2DepthOffset", 0.0)), -1.0f, 1.0f)
                : 0.0f;
            p->setValueNotifyingHost (p->convertTo0to1 (o2));
        }
    }

    {
        const juce::ScopedLock sl (engineLock);

        // Immediately sync Stereo Chorus, Depth & DCW Depth Offsets into DSP engine
        if (chorusParam != nullptr)
            engine.setChorusEnabled (*chorusParam > 0.5f);
        else if (xmlState->hasAttribute ("chorusEnabled"))
            engine.setChorusEnabled (xmlState->getBoolAttribute ("chorusEnabled", true));

        if (chorusDepthParam != nullptr)
            engine.setChorusDepth (*chorusDepthParam);
        else if (xmlState->hasAttribute ("chorusDepth"))
            engine.setChorusDepth (static_cast<float> (xmlState->getDoubleAttribute ("chorusDepth", 0.4)));

        if (dcw1DepthOffsetParam != nullptr)
            engine.setDcw1DepthOffset (dcw1DepthOffsetParam->load());
        else if (xmlState->hasAttribute ("dcw1DepthOffset"))
            engine.setDcw1DepthOffset (static_cast<float> (xmlState->getDoubleAttribute ("dcw1DepthOffset", 0.0)));

        if (dcw2DepthOffsetParam != nullptr)
            engine.setDcw2DepthOffset (dcw2DepthOffsetParam->load());
        else if (xmlState->hasAttribute ("dcw2DepthOffset"))
            engine.setDcw2DepthOffset (static_cast<float> (xmlState->getDoubleAttribute ("dcw2DepthOffset", 0.0)));

        // 2. Restore EFFECT SECTION & OVERALL ON/OFF switches
        const juce::XmlElement* effEl = isNewRoot ? xmlState->getChildByName ("EffectSettings") : xmlState.get();
        if (effEl != nullptr)
        {
            auto eff = engine.getEffectSettings();
            eff.modulationOn       = effEl->getBoolAttribute ("modulationOn", eff.modulationOn);
            eff.portamentoOn       = effEl->getBoolAttribute ("portamentoOn", eff.portamentoOn);
            eff.glideOn            = effEl->getBoolAttribute ("glideOn", eff.glideOn);
            eff.soloOn             = effEl->getBoolAttribute ("soloOn", eff.soloOn);
            eff.portamentoSweep    = effEl->getIntAttribute ("portamentoSweep", eff.portamentoSweep);
            eff.portamentoTime     = effEl->getIntAttribute ("portamentoTime", eff.portamentoTime);
            eff.glideNote          = effEl->getIntAttribute ("glideNote", eff.glideNote);
            eff.glideTime          = effEl->getIntAttribute ("glideTime", eff.glideTime);
            eff.bendRange          = effEl->getIntAttribute ("bendRange", eff.bendRange);
            eff.modWheelDepth      = effEl->getIntAttribute ("modWheelDepth", eff.modWheelDepth);
            eff.modAfterTouchDepth = effEl->getIntAttribute ("modAfterTouchDepth", eff.modAfterTouchDepth);
            eff.ampAfterTouchRange = effEl->getIntAttribute ("ampAfterTouchRange", eff.ampAfterTouchRange);
            engine.setEffectSettings (eff);
        }

        // 3. Restore Bank & Selected Preset Program
        if (const auto* bankEl = xmlState->getChildByName ("Bank"))
        {
            const juce::String bankName   = bankEl->getStringAttribute ("name", "Init Bank (64 Slots)");
            const juce::String bankFormat = bankEl->getStringAttribute ("format", "CZ-1 (144 bytes)");
            const int savedIdx = bankEl->getIntAttribute ("currentPresetIndex", 0);

            bool loadedCustom = false;
            if (bankEl->hasAttribute ("syxBase64"))
            {
                juce::MemoryBlock syxBlock;
                if (syxBlock.fromBase64Encoding (bankEl->getStringAttribute ("syxBase64")) && syxBlock.getSize() >= 128)
                {
                    if (engine.loadSyxData (static_cast<const uint8_t*> (syxBlock.getData()),
                                            syxBlock.getSize(),
                                            bankName.toStdString()))
                    {
                        engine.setBankMetadata (bankName.toStdString(), bankFormat.toStdString());
                        loadedCustom = true;
                    }
                }
            }

            if (!loadedCustom)
                engine.resetInitBank();

            engine.loadPreset (savedIdx);
        }

        // 4. Restore PARAMETER SECTION (Current Patch)
        if (const auto* patchEl = xmlState->getChildByName ("CurrentPatch"))
        {
            deserializeCurrentPatch (patchEl, engine);
        }
    }

    ++stateRevision;
}

bool CZ1AudioProcessor::loadSyxFile (const juce::File& file)
{
    juce::MemoryBlock mb;
    if (file.loadFileAsData (mb))
    {
        return loadSyxData (mb.getData(), mb.getSize(), file.getFileName());
    }
    return false;
}

bool CZ1AudioProcessor::loadSyxData (const void* data, size_t size, const juce::String& name)
{
    bool ok = false;
    {
        const juce::ScopedLock sl (engineLock);
        ok = engine.loadSyxData (static_cast<const uint8_t*> (data), size, name.toStdString());
    }
    if (ok)
    {
        ++stateRevision;
        juce::AudioProcessor::ChangeDetails details;
        details.programChanged = true;
        details.parameterInfoChanged = true;
        details.nonParameterStateChanged = true;
        updateHostDisplay (details);
    }
    return ok;
}

bool CZ1AudioProcessor::saveSyxBankToFile (const juce::File& file)
{
    // Enforce strict no-overwrite policy: only create brand-new files
    if (file == juce::File() || file.exists())
        return false;

    std::vector<uint8_t> syxBytes;
    {
        const juce::ScopedLock sl (engineLock);
        syxBytes = engine.exportBankToSyx();
    }

    if (syxBytes.empty() || file.exists())
        return false;

    juce::FileOutputStream stream (file);
    if (!stream.openedOk())
        return false;

    if (!stream.write (syxBytes.data(), syxBytes.size()))
        return false;

    stream.flush();
    return !stream.getStatus().failed();
}

void CZ1AudioProcessor::resetInitBank()
{
    {
        const juce::ScopedLock sl (engineLock);
        engine.resetInitBank();
    }
    ++stateRevision;
    juce::AudioProcessor::ChangeDetails details;
    details.programChanged = true;
    details.parameterInfoChanged = true;
    details.nonParameterStateChanged = true;
    updateHostDisplay (details);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CZ1AudioProcessor();
}
