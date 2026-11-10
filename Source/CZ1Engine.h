/*
  ==============================================================================
    CZ1Engine.h
    Casio CZ-1 Phase Distortion Sound Engine
    Based on NEC uPD933 emulation from MAME (Devin Acker)
  ==============================================================================
*/

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "CZ1FactoryPresets.h"

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

    void loadPreset(int presetIndex);
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    const char* getPresetName(int presetIndex) const;
    const char* getPresetSlot(int presetIndex) const;
    const CZ1PresetDef& getPresetDef(int presetIndex) const;
    const CZ1PresetDef& getCurrentPresetDef() const;
    static const char* getWaveformName(uint8_t waveIndex);
    static const char* getWindowName(uint8_t windowIndex);
    static const char* getLineModeName(int8_t lineMode);
    static constexpr int NUM_PRESETS = 64;

    // Render 1 stereo sample frame at native 40,000 Hz
    void renderNativeSample(float& outLeft, float& outRight);

    void setChorusEnabled(bool enabled) { chorusEnabled = enabled; }
    bool isChorusEnabled() const { return chorusEnabled; }

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

    void writeDcaStep(Upd933Chip& chip, int vnum, const EnvelopeDef& envDef, int stage, float velScale, int keyFollow, int note, bool isReleased);
    void writeDcwStep(Upd933Chip& chip, int vnum, const EnvelopeDef& envDef, int stage, float velScale, int keyFollow, int note, bool isReleased);
    void writeDcoStep(Upd933Chip& chip, int vnum, const EnvelopeDef& envDef, int stage, float velScale, int keyFollow, int note, bool isReleased);
    void triggerVoiceRelease(NoteVoice& v);
    void applyPresetWaveforms();

    void initTables();
    void processEnvelopeSequencer();
    int  findVoiceToAllocate(int midiNote);

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

    // Stereo chorus delay line
    static constexpr int CHORUS_BUF_SIZE = 1024;
    float chorusBuffer[CHORUS_BUF_SIZE] = {0.0f};
    int   chorusWritePos = 0;
    float chorusPhase = 0.0f;

    int   currentPresetIndex = 0;
};
