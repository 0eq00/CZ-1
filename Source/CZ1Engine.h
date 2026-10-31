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

    struct NoteVoice
    {
        int  midiNote = -1;
        int  velocity = 0;
        bool active = false;
        int  pairIndex = 0;
        int  v1 = 2;
        int  v2 = 7;

        int  dcaStage = 0;
        int  dcwStage = 0;
        int  dcoStage = 0;
        bool isReleased = false;
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

        void reset();
        void writeReg(uint8_t reg, uint16_t val);
        int16_t renderVoiceSample(int vnum, const uint16_t* cosine, const uint16_t* volumeTable);
        int16_t renderChipSample(const uint16_t* cosine, const uint16_t* volumeTable);
        void updatePitchStep(int vnum, const uint32_t* pitchTable, const uint16_t* pitchFine);
    };

    CZ1Engine();
    ~CZ1Engine() = default;

    void reset();
    void noteOn(int midiNote, int velocity = 81);
    void noteOff(int midiNote, int velocity = 66);
    void allNotesOff();

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

    void initTables();
    void processEnvelopeSequencer();

    uint16_t cosineTable[0x800] = {0};
    uint32_t pitchTable[0x80] = {0};
    uint16_t pitchFine[0x200] = {0};
    uint16_t volumeTable[0x200] = {0};

    Upd933Chip chip0;
    Upd933Chip chip1;

    static constexpr int NUM_VOICE_PAIRS = 4;
    const std::pair<int, int> voicePairs[NUM_VOICE_PAIRS] = {
        {2, 7}, // Primary dual-line pair from MAME CZ-1 log
        {0, 5},
        {1, 4},
        {3, 6}
    };

    std::array<NoteVoice, NUM_VOICE_PAIRS> allocatedVoices;

    bool chorusEnabled = true;
};
