/*
  ==============================================================================
    CZ1Engine.h
    Casio CZ-1 Phase Distortion Sound Engine
    Based on NEC uPD933 emulation from MAME (Devin Acker)
  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "CZ1SysExParser.h"

class CZ1Engine
{
public:
    static constexpr uint32_t NATIVE_RATE = 40000;
    static constexpr unsigned NOTE_SHIFT = 9;
    static constexpr unsigned PITCH_SHIFT = 20;
    static constexpr unsigned PITCH_FINE_SHIFT = 12;
    static constexpr unsigned VOLUME_SHIFT = 12;
    static constexpr unsigned ENV_DCA_SHIFT = 16;
    static constexpr unsigned ENV_DCW_SHIFT = 16;
    static constexpr unsigned ENV_DCO_SHIFT = 11;

    struct Envelope
    {
        uint8_t  direction = 0; // 0 = up, 1 = down
        uint8_t  sustain = 1;   // 1 = sustain level held
        uint8_t  irq = 0;
        uint32_t rate = 0;
        uint32_t target = 0;
        uint32_t current = 0;

        void update();
    };

    struct Voice
    {
        uint8_t  wave[2] = {0, 0};
        uint8_t  window = 0;
        uint8_t  ringMod = 0;
        uint8_t  pitchMod = 0;
        uint8_t  muteOther = 0;
        uint16_t pitch = 0; // 7.9 fixed point
        uint32_t position = 0;
        uint32_t pitchStep = 0;
        uint16_t dcwLimit = 0;
        int16_t  pmLevel = 0;

        void reset();
    };

    struct VoicePairDef
    {
        int chipIndex = 0; // 0 or 1
        int v1 = 7;        // Line 1 voice (odd)
        int v2 = 2;        // Line 2 voice (even)
    };

    struct LineVoiceState
    {
        int dcaStage = 0;
        int dcwStage = 0;
        int dcoStage = 0;
        bool finished = false;
    };

    struct NoteVoice
    {
        int  midiNote = -1;
        int  line1Note = 60;
        int  line2Note = 60;
        int  velocity = 0;
        bool active = false;
        int  pairIndex = 0;
        int  chipIndex = 0;
        int  v1 = 7;
        int  v2 = 2;

        int  dcaStage = 0;
        int  dcwStage = 0;
        int  dcoStage = 0;
        LineVoiceState line1;
        LineVoiceState line2;

        bool isReleased = false;
        bool keyHeld = false;
        int  keyRateOffset = 0;
        uint64_t noteOnCounter = 0;
        float vibratoPhase = 0.0f;
        int   vibratoDelayCounter = 0;
        int   currentVibPitchOffset = 0;

        float currentPitchUnits = static_cast<float>(0x6a00);
        float targetPitchUnits  = static_cast<float>(0x6a00);
        float pitchGlideStep    = 0.0f;
    };

    struct Upd933Chip
    {
        std::array<Voice, 8>    voices;
        std::array<Envelope, 8> dca;
        std::array<Envelope, 8> dcw;
        std::array<Envelope, 8> dco;
        std::array<uint16_t, 256> regs = {0};

        uint32_t sampleCount = 0;
        int16_t  lastSample = 0;
        uint32_t noiseLfsr = 0x12345678;

        void reset();
        void writeReg(uint8_t reg, uint16_t val);
        int16_t renderVoiceSample(int vnum, const uint16_t* cosine, const uint16_t* volumeTable, const uint32_t* pitchTable, const uint16_t* pitchFine);
        float renderChipSample(const uint16_t* cosine, const uint16_t* volumeTable, const uint32_t* pitchTable, const uint16_t* pitchFine);
        void updatePitchStep(int vnum, const uint32_t* pitchTable, const uint16_t* pitchFine);
    };

    CZ1Engine();
    ~CZ1Engine() = default;

    void reset();
    void noteOn(int midiNote, int velocity = 81);
    void noteOff(int midiNote, int velocity = 66);
    void allNotesOff();

    void setSustainPedal(bool down);
    bool isSustainPedalDown() const { return sustainPedal; }
    void handleControlChange(int controller, int value);

    using VibratoParams = CZSysEx::VibratoDef;

    struct DetuneParams
    {
        int sign = 1;  // +1 or -1
        int oct = 0;   // 0..3
        int note = 0;  // 0..11
        int fine = 0;  // 0..60
    };

    struct EffectSettings
    {
        bool  modulationOn = false;
        bool  portamentoOn = false;
        bool  glideOn = false;
        bool  soloOn = false;
        int   portamentoSweep = 0;     // 0 = fixed time, 1 = variable time
        int   portamentoTime = 30;     // 0..99
        int   glideNote = -2;          // -12..+12 semitones
        int   glideTime = 30;          // 0..99
        int   bendRange = 3;           // 0..12 semitones
        int   modWheelDepth = 50;      // 0..99
        int   modAfterTouchDepth = 50; // 0..99
        int   ampAfterTouchRange = 0;  // 0..15
        float pitchBend = 0.0f;        // -1.0..+1.0
        float modWheel = 0.0f;         // 0.0..1.0
        float afterTouch = 0.0f;       // 0.0..1.0
    };

    void setEffectSettings(const EffectSettings& settings);
    const EffectSettings& getEffectSettings() const { return effectSettings; }
    void setModulationOn(bool on);
    void setPortamentoOn(bool on);
    void setGlideOn(bool on);
    void setSoloOn(bool on);
    void setPitchBend(float bendNorm);
    void setModWheel(float wheelNorm);
    void setAfterTouch(float atNorm);

    void loadPreset(int presetIndex);
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    int getPresetCount() const { return static_cast<int>(activePresets.size()); }
    const char* getPresetName(int presetIndex) const;
    const char* getPresetSlot(int presetIndex) const;
    const CZ1PresetDef& getPresetDef(int presetIndex) const;
    const CZ1PresetDef& getCurrentPresetDef() const;
    CZ1PresetDef& getCurrentPresetMutable();
    const VibratoParams& getCurrentVibrato() const { return currentVibrato; }
    VibratoParams& getCurrentVibratoMutable() { return currentVibrato; }
    const DetuneParams& getCurrentDetune() const { return currentDetune; }
    DetuneParams& getCurrentDetuneMutable() { return currentDetune; }
    void updateCurrentPresetLive();
    void restorePresetAt(int index, const CZ1PresetDef& preset, const VibratoParams& vib, const DetuneParams& det);
    void setBankMetadata(const std::string& name, const std::string& format);
    bool isInitBankExcept(int skipIndex) const;

    const std::string& getBankName() const { return currentBankName; }
    const std::string& getBankFormat() const { return currentBankFormat; }

    bool loadSyxData(const uint8_t* data, size_t size, const std::string& bankName = "bank.syx");
    void resetInitBank();
    std::vector<uint8_t> exportBankToSyx() const;

    static const char* getWaveformName(uint8_t waveIndex);
    static const char* getWindowName(uint8_t windowIndex);
    static const char* getLineModeName(int8_t lineMode);
    static constexpr size_t DEFAULT_BANK_SIZE = 64;
    static constexpr int MAX_PROGRAMS = 128;
    static constexpr int NUM_PRESETS = 128;

    // Render 1 stereo sample frame at native 40,000 Hz
    void renderNativeSample(float& outLeft, float& outRight);

    void setChorusEnabled(bool enabled) { chorusEnabled = enabled; }
    bool isChorusEnabled() const { return chorusEnabled; }
    void setChorusDepth(float depth) { chorusDepth = std::clamp(depth, 0.0f, 1.0f); }
    float getChorusDepth() const { return chorusDepth; }

    int getActiveVoiceCount() const;

private:
    static uint32_t envRate(uint8_t data)
    {
        return static_cast<uint32_t>((8 | (data & 7)) << (data >> 3));
    }

    static int getKeyRateOffset(int midiNote);
    static uint8_t getDcaAttackTarget(int velocity);
    static uint8_t getDcaSustainTarget(int velocity);
    static uint8_t getDcaRelease1Target(int velocity);
    static uint8_t getDcwAttackTarget(int velocity);
    static uint8_t getDcwDecay1Target(int velocity);
    static uint8_t getDcoAttackTarget(int velocity);

    void writeDcaStep(Upd933Chip& chip, int vnum, const EnvelopeDef& envDef, int stage, int dcaDrop, int keyFollow, int note, bool isReleased);
    void writeDcwStep(Upd933Chip& chip, int vnum, const EnvelopeDef& envDef, int stage, int dcwDrop, int keyFollow, int note, bool isReleased);
    void writeDcoStep(Upd933Chip& chip, int vnum, const EnvelopeDef& envDef, int stage, float velScale, int keyFollow, int note, bool isReleased);
    void triggerVoiceRelease(NoteVoice& v);
    void applyPresetWaveforms(bool resetEnvelopes = true);
    void syncDetuneAndVibratoFromPreset();
    static int   getLineLevelAttenuation(uint8_t lineLevel);
    static float getVelocityAttenuationNorm(int velocity);
    static int   getDcaVelocityDrop(int velocity, uint8_t dcaVelocity, uint8_t lineLevel);
    static int   getDcwVelocityDrop(int velocity, uint8_t dcwVelocity);
    static float getDcoVelocityScale(int velocity, uint8_t dcoVelocity);

    void triggerVoiceNoteOn(int midiNote, int velocity, bool wasLegato);
    static float getDurationSecondsForTime(int timeVal);
    void initTables();
    void processEnvelopeSequencer();
    int  findVoiceToAllocate(int midiNote);
    float readBbdLine(int lineIndex, float delaySamples) const;

    uint16_t cosineTable[0x800] = {0};
    uint32_t pitchTable[0x80] = {0};
    uint16_t pitchFine[0x200] = {0};
    uint16_t volumeTable[0x200] = {0};

    Upd933Chip chip0;
    Upd933Chip chip1;

    static constexpr int NUM_VOICE_PAIRS = 8;
    const VoicePairDef voicePairs[NUM_VOICE_PAIRS] = {
        // Interleaved dual-line voice pairs alternating between Chip 0 and Chip 1 for load balancing
        {0, 7, 2}, // Pair 0 (Chip 0)
        {1, 7, 2}, // Pair 1 (Chip 1)
        {0, 1, 4}, // Pair 2 (Chip 0)
        {1, 1, 4}, // Pair 3 (Chip 1)
        {0, 3, 6}, // Pair 4 (Chip 0)
        {1, 3, 6}, // Pair 5 (Chip 1)
        {0, 5, 0}, // Pair 6 (Chip 0)
        {1, 5, 0}  // Pair 7 (Chip 1)
    };

    std::array<NoteVoice, NUM_VOICE_PAIRS> allocatedVoices;
    uint64_t globalNoteCounter = 0;

    bool chorusEnabled = true;
    bool sustainPedal = false;

    // Previous Stereo Chorus delay line (kept commented out as requested)
    /*
    static constexpr int CHORUS_BUF_SIZE = 1024;
    float chorusBuffer[CHORUS_BUF_SIZE] = {0.0f};
    int   chorusWritePos = 0;
    float chorusPhase = 0.0f;
    */

    // Analog BBD Stereo Chorus (MN3209 x 3 parallel 256-stage BBD ensemble)
    static constexpr int BBD_NUM_LINES = 3;
    static constexpr int BBD_BUF_SIZE = 512;
    static constexpr int BBD_BUF_MASK = 511;
    std::array<std::array<float, BBD_BUF_SIZE>, BBD_NUM_LINES> bbdBuffers{};
    int   bbdWritePos = 0;
    float bbdSlowPhase = 0.0f; // ~0.56 Hz LFO
    float bbdFastPhase = 0.0f; // ~7.14 Hz LFO
    float bbdPreLpfState = 0.0f;
    float bbdPostLpfL = 0.0f;
    float bbdPostLpfR = 0.0f;
    float chorusDepth = 0.4f;

    std::vector<CZ1PresetDef> activePresets;
    std::vector<CZSysEx::VibratoDef> presetVibratos;
    std::string currentBankName = "Init Bank (64 Slots)";
    std::string currentBankFormat = "CZ-1 (144 bytes)";
    int   currentPresetIndex = 0;
    VibratoParams  currentVibrato;
    DetuneParams   currentDetune;
    EffectSettings effectSettings;

    std::vector<int> heldNotes;
    float lastPortamentoPitchUnits = static_cast<float>(0x6a00);
    bool  hasLastPortamentoPitch = false;
    float smoothedAtGain = 1.0f;
};
