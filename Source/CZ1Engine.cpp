/*
  ==============================================================================
    CZ1Engine.cpp
    Casio CZ-1 Phase Distortion Sound Engine Implementation
    Directly incorporates MAME uPD933 algorithms and the exact CZ-1
    initialization and note on/off envelope step sequence from hardware debug logs.
  ==============================================================================
*/

#include "CZ1Engine.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void CZ1Engine::Envelope::update()
{
    if (current != target)
    {
        if (!direction) // increasing
        {
            if (current > target || (target - current) <= rate)
                current = target;
            else
                current += rate;
        }
        else // decreasing
        {
            if (current < target || (current - target) <= rate)
                current = target;
            else
                current -= rate;
        }
    }

    if (!sustain && current == target)
    {
        irq = 1;
        sustain = 1;
    }
}

void CZ1Engine::Voice::reset()
{
    wave[0] = wave[1] = 0;
    window = 0;
    ringMod = 0;
    pitchMod = 0;
    muteOther = 0;
    pitch = 0;
    position = 0;
    pitchStep = 0;
    dcwLimit = 0;
    pmLevel = 0;
}

void CZ1Engine::Upd933Chip::reset()
{
    sampleCount = 0;
    lastSample = 0;
    regs.fill(0);
    for (int i = 0; i < 8; ++i)
    {
        voices[i].reset();
        dca[i] = Envelope();
        dcw[i] = Envelope();
        dco[i] = Envelope();
    }
}

void CZ1Engine::Upd933Chip::writeReg(uint8_t reg, uint16_t val)
{
    regs[reg] = val;
    const int vnum = reg & 7;
    Voice& voice = voices[vnum];
    Voice& modVoice = voices[(vnum + 6) & 7];

    switch (reg >> 3)
    {
    case 0x0: // DCA step (volume envelope)
    {
        Envelope& env = dca[vnum];
        env.direction = (val >> 15) & 1;
        env.rate      = CZ1Engine::envRate((val >> 8) & 0x7f);
        env.sustain   = (val >> 7) & 1;
        env.target    = (val & 0x7f) << (ENV_DCA_SHIFT + 2);
        env.irq       = 0;
        break;
    }
    case 0x2: // DCO step (pitch envelope)
    {
        Envelope& env = dco[vnum];
        env.direction = (val >> 15) & 1;
        env.rate      = CZ1Engine::envRate((val >> 8) & 0x7f);
        env.sustain   = (val >> 7) & 1;
        env.target    = (val & 0x3f) << (ENV_DCO_SHIFT + 5);
        if ((val >> 6) & 1)
            env.target <<= 5;
        env.irq       = 0;
        break;
    }
    case 0x4: // DCW step (waveform envelope)
    {
        Envelope& env = dcw[vnum];
        env.direction = (val >> 15) & 1;
        env.rate      = CZ1Engine::envRate((val >> 8) & 0x7f);
        env.sustain   = (val >> 7) & 1;
        env.target    = (val & 0x7f) << (ENV_DCW_SHIFT + 3);
        env.irq       = 0;
        break;
    }
    case 0xc: // Pitch (semitones in 7.9 fixed point)
        voice.pitch = val;
        break;
    case 0xd: // Waveform and modulation flags
        voice.wave[0] = (val >> 13) & 7;
        if ((val >> 9) & 1)
            voice.wave[1] = (val >> 10) & 7;
        else
            voice.wave[1] = voice.wave[0];
        voice.window = (val >> 6) & 7;
        if (!(vnum & 1))
        {
            modVoice.ringMod   = (val >> 5) & 1;
            modVoice.pitchMod  = (val >> 3) & 3;
            modVoice.muteOther = (val >> 2) & 1;
        }
        break;
    case 0x13: // Phase counter
        voice.position = (static_cast<uint32_t>(val) << (PITCH_SHIFT - 4));
        break;
    case 0x17: // Pitch modulator level
        if (vnum < 4)
            voices[vnum << 1].pmLevel = static_cast<int16_t>(val);
        break;
    default:
        break;
    }
}

void CZ1Engine::Upd933Chip::updatePitchStep(int vnum, const uint32_t* pitchTable, const uint16_t* pitchFine)
{
    Voice& voice = voices[vnum];
    const int32_t pitch = static_cast<int32_t>(voice.pitch + (dco[vnum].current >> ENV_DCO_SHIFT)) + voice.pmLevel;
    uint32_t step = 0;

    if (pitch > 0 && pitch < 0x10000)
    {
        const uint8_t note = (pitch >> NOTE_SHIFT) & 0x7f;
        const uint16_t fine = pitch & ((1 << NOTE_SHIFT) - 1);
        step = pitchTable[note];
        if (fine)
            step += static_cast<uint32_t>((step >> PITCH_FINE_SHIFT) * pitchFine[fine]);
    }

    voice.pitchStep = step;
    voice.dcwLimit = static_cast<uint16_t>(0x400 - std::min(0x400U, (step >> (PITCH_SHIFT - 2))));
}

int16_t CZ1Engine::Upd933Chip::renderVoiceSample(int vnum, const uint16_t* cosine, const uint16_t* volumeTable)
{
    Voice& voice = voices[vnum];
    int16_t sample = 0;

    const uint16_t pos = (voice.position >> PITCH_SHIFT) & 0x7ff;
    const uint8_t wave = (voice.position >> (PITCH_SHIFT + 11)) & 1;

    const uint16_t ldcw = std::min(static_cast<uint16_t>(dcw[vnum].current >> ENV_DCW_SHIFT), voice.dcwLimit);
    const uint16_t pivot = (0x400 > ldcw) ? (0x400 - ldcw) : 1;
    uint16_t phase = 0;
    uint16_t window = 0;

    // Apply transfer function (Phase Distortion)
    switch (voice.wave[wave] & 7)
    {
    case 0: // Sawtooth
        if (pos < pivot)
            phase = static_cast<uint16_t>(pos * 0x400 / pivot);
        else
            phase = static_cast<uint16_t>(0x400 + (pos - pivot) * 0x400 / (0x800 - pivot));
        break;
    case 1: // Square
        if ((pos & 0x3ff) < pivot)
            phase = static_cast<uint16_t>((pos & 0x3ff) * 0x400 / pivot);
        else
            phase = 0x3ff;
        phase |= (pos & 0x400);
        break;
    case 2: // Pulse
        if (pos < pivot * 2)
            phase = static_cast<uint16_t>(pos * 0x800 / (pivot * 2));
        else
            phase = 0x7ff;
        break;
    case 3: // Silent
        break;
    case 4: // Double sine
        if (pos < pivot)
            phase = static_cast<uint16_t>(pos * 0x800 / pivot);
        else
            phase = static_cast<uint16_t>((pos - pivot) * 0x800 / (0x800 - pivot));
        break;
    case 5: // Saw pulse (Default CZ-1 wave)
        if (pos < 0x400)
            phase = pos;
        else if (pos < (pivot + 0x400))
            phase = static_cast<uint16_t>(0x400 + (pos & 0x3ff) * 0x400 / pivot);
        else
            phase = 0x7ff;
        break;
    case 6: // Resonance
        phase = (pos + ((pos * ldcw) >> 6)) & 0x7ff;
        break;
    case 7: // Double pulse
        if ((pos & 0x3ff) < pivot)
            phase = static_cast<uint16_t>((pos & 0x3ff) * 0x400 / pivot);
        else
            phase = 0x7ff;
        break;
    }

    // Apply window function
    switch (voice.window & 7)
    {
    case 0: // None
        break;
    case 1: // Sawtooth
        window = pos;
        break;
    case 2: // Triangle
        window = static_cast<uint16_t>((pos & 0x3ff) * 2);
        if (pos < 0x400) window ^= 0x7fe;
        break;
    case 3: // Trapezoid
        if (pos >= 0x400) window = static_cast<uint16_t>((pos & 0x3ff) * 2);
        break;
    case 4: // Pulse
        if (pos < 0x400) window = static_cast<uint16_t>(pos * 2);
        else window = 0x7ff;
        break;
    default: // Double saw
        window = static_cast<uint16_t>((0x3ff ^ (pos & 0x3ff)) * 2);
        break;
    }

    sample = cosine[phase & 0x7ff];
    if (window)
        sample = static_cast<int16_t>((static_cast<int32_t>(sample) * (0x800 - window)) / 0x800);

    const uint16_t volume = dca[vnum].current >> ENV_DCA_SHIFT;
    const uint16_t volGain = (volume < 0x200) ? volumeTable[volume] : volumeTable[0x1ff];
    sample = static_cast<int16_t>(((static_cast<int32_t>(sample) * volGain) >> VOLUME_SHIFT) - (volGain / 2));

    if (voice.ringMod)
        sample = static_cast<int16_t>((static_cast<int32_t>(sample) * lastSample) / 0x1000);

    if (voice.muteOther)
        sample = static_cast<int16_t>(sample - lastSample);

    // Update envelopes
    dca[vnum].update();
    dcw[vnum].update();
    dco[vnum].update();

    if (!(sampleCount & 7))
    {
        switch (voice.pitchMod & 3)
        {
        default:
            voice.pmLevel = 0;
            break;
        case 2:
            voice.pmLevel = lastSample;
            break;
        case 3:
            voice.pmLevel = static_cast<int16_t>((rand() % 2) * (32 << NOTE_SHIFT));
            break;
        }
    }

    voice.position += voice.pitchStep;
    lastSample = sample;
    return sample;
}

int16_t CZ1Engine::Upd933Chip::renderChipSample(const uint16_t* cosine, const uint16_t* volumeTable)
{
    int32_t total = 0;
    static const int voiceMap[] = {5, 0, 7, 2, 1, 4, 3, 6};
    for (int v : voiceMap)
    {
        total += renderVoiceSample(v, cosine, volumeTable);
    }
    sampleCount++;
    return static_cast<int16_t>(std::clamp(total, -32768, 32767));
}

// -----------------------------------------------------------------------------
// CZ1Engine Main Class
// -----------------------------------------------------------------------------

CZ1Engine::CZ1Engine()
{
    initTables();
    reset();
}

void CZ1Engine::initTables()
{
    for (int i = 0; i < 0x800; ++i)
        cosineTable[i] = static_cast<uint16_t>(std::round(0xfff * (1.0 - std::cos(2.0 * M_PI * i / 0x7ff)) / 2.0));

    for (int i = 0; i < 0x80; ++i)
    {
        const double freq = 442.0 * std::pow(2.0, (i - 62) / 12.0);
        pitchTable[i] = static_cast<uint32_t>((1 << PITCH_SHIFT) * (freq * 0x800 / 40000.0));
    }

    for (int i = 0; i < 0x200; ++i)
        pitchFine[i] = static_cast<uint16_t>((1 << PITCH_FINE_SHIFT) * (std::pow(2.0, static_cast<double>(i) / (12.0 * 0x200)) - 1.0));

    for (int i = 1; i < 0x200; ++i)
        volumeTable[i] = static_cast<uint16_t>(std::pow(2 << VOLUME_SHIFT, static_cast<double>(i) / 0x1ff));
    volumeTable[0] = 0;
}

void CZ1Engine::reset()
{
    chip0.reset();
    chip1.reset();

    for (auto& v : allocatedVoices)
        v = NoteVoice();

    // Initial preset setup from MAME CZ-1 log:
    // Wave 5 (Saw-Pulse) for all voices, idle envelopes
    for (int v = 0; v < 8; ++v)
    {
        chip0.writeReg(0x68 + v, 0xa000);
        chip1.writeReg(0x68 + v, 0xa000);

        chip0.writeReg(0x00 + v, 0xff80); // DCA idle
        chip1.writeReg(0x00 + v, 0xff80);
        chip0.writeReg(0x20 + v, 0x7f80); // DCW idle
        chip1.writeReg(0x20 + v, 0x7f80);
        chip0.writeReg(0x10 + v, 0x7f80); // DCO idle
        chip1.writeReg(0x10 + v, 0x7f80);
    }
}

int CZ1Engine::getKeyRateOffset(int midiNote)
{
    // Authentic Casio CZ-1 Key Follow rate offsets across 88 keys
    // Base rate is calibrated around Middle C (MIDI Key 60)
    static constexpr int8_t OFFSET_TABLE[128] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1,
        2, 2,
        3, 3, 3, 3,
        4, 4,
        5, 5,
        6, 6,
        7, 7,
        8,
        9, 9,
        10,
        11, 11,
        12,
        13,
        14,
        15,
        16,
        17,
        19,
        20,
        21,
        23,
        24,
        26,
        27,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
        28, 28, 28, 28, 28, 28
    };

    if (midiNote < 0) return -1;
    if (midiNote > 127) return 28;
    return OFFSET_TABLE[midiNote];
}

int CZ1Engine::findVoiceToAllocate(int midiNote)
{
    // 1. Priority: If a voice is already assigned to this exact note (even if releasing), re-trigger it!
    for (int i = 0; i < NUM_VOICE_PAIRS; ++i)
    {
        if (allocatedVoices[i].active && allocatedVoices[i].midiNote == midiNote)
            return i;
    }

    // 2. Priority: Find an idle (completely inactive) voice
    for (int i = 0; i < NUM_VOICE_PAIRS; ++i)
    {
        if (!allocatedVoices[i].active)
            return i;
    }

    // 3. Priority: Steal a releasing voice with the lowest current DCA volume
    int bestReleasingIdx = -1;
    uint32_t lowestDcaLevel = 0xffffffff;
    for (int i = 0; i < NUM_VOICE_PAIRS; ++i)
    {
        if (allocatedVoices[i].isReleased)
        {
            const auto& voice = allocatedVoices[i];
            Upd933Chip& chip = (voice.chipIndex == 0) ? chip0 : chip1;
            uint32_t curLevel = chip.dca[voice.v1].current;
            if (curLevel < lowestDcaLevel)
            {
                lowestDcaLevel = curLevel;
                bestReleasingIdx = i;
            }
        }
    }
    if (bestReleasingIdx != -1)
        return bestReleasingIdx;

    // 4. Priority: If all voices are held and active, steal the oldest voice (LRU)
    int oldestIdx = 0;
    uint64_t oldestTime = 0xffffffffffffffffULL;
    for (int i = 0; i < NUM_VOICE_PAIRS; ++i)
    {
        if (allocatedVoices[i].noteOnCounter < oldestTime)
        {
            oldestTime = allocatedVoices[i].noteOnCounter;
            oldestIdx = i;
        }
    }
    return oldestIdx;
}

void CZ1Engine::noteOn(int midiNote, int velocity)
{
    const int idx = findVoiceToAllocate(midiNote);
    NoteVoice& voice = allocatedVoices[idx];

    voice.midiNote = midiNote;
    voice.velocity = velocity;
    voice.active = true;
    voice.pairIndex = idx;
    voice.chipIndex = voicePairs[idx].chipIndex;
    voice.v1 = voicePairs[idx].v1;
    voice.v2 = voicePairs[idx].v2;
    voice.dcaStage = 0;
    voice.dcwStage = 0;
    voice.dcoStage = 0;
    voice.isReleased = false;
    voice.noteOnCounter = ++globalNoteCounter;

    const int kOffset = getKeyRateOffset(midiNote);
    voice.keyRateOffset = kOffset;

    // Pitch determination (chromatic tracking from key 60 log baseline):
    // Key 60 log baseline: V2=0x51d8 (~40.92 semi), V7=0x5200 (41.00 semi)
    const int shift = (midiNote - 60) * 512;
    const uint16_t pitchV1 = static_cast<uint16_t>(std::clamp(0x51d8 + shift, 0, 0xffff));
    const uint16_t pitchV2 = static_cast<uint16_t>(std::clamp(0x5200 + shift, 0, 0xffff));

    const int v1 = voice.v1;
    const int v2 = voice.v2;
    Upd933Chip& chip = (voice.chipIndex == 0) ? chip0 : chip1;

    chip.writeReg(0x60 + v1, pitchV1);
    chip.writeReg(0x60 + v2, pitchV2);

    chip.updatePitchStep(v1, pitchTable, pitchFine);
    chip.updatePitchStep(v2, pitchTable, pitchFine);

    // Reset phase counters (0x0000 for Line 1, 0x0080 for Line 2 from log)
    chip.writeReg(0x98 + v1, 0x0000);
    chip.writeReg(0x98 + v2, 0x0080);

    // Step 1: DCA Attack (dir=0, rate=99+kOffset, sus=0, target=123)
    const uint8_t dcaAttackRate = static_cast<uint8_t>(std::clamp(99 + kOffset, 0, 127));
    const uint16_t dcaAttackWord = static_cast<uint16_t>((dcaAttackRate << 8) | 123);
    chip.writeReg(0x00 + v1, dcaAttackWord);
    chip.writeReg(0x00 + v2, dcaAttackWord);

    // Step 1: DCW Attack (dir=0, rate=98+kOffset, sus=0, target=112)
    const uint8_t dcwAttackRate = static_cast<uint8_t>(std::clamp(98 + kOffset, 0, 127));
    const uint16_t dcwAttackWord = static_cast<uint16_t>((dcwAttackRate << 8) | 112);
    chip.writeReg(0x20 + v1, dcwAttackWord);
    chip.writeReg(0x20 + v2, dcwAttackWord);

    // Step 1: DCO Attack (dir=0, rate=93+kOffset, sus=0, target=29)
    const uint8_t dcoAttackRate = static_cast<uint8_t>(std::clamp(93 + kOffset, 0, 127));
    const uint16_t dcoAttackWord = static_cast<uint16_t>((dcoAttackRate << 8) | 29);
    chip.writeReg(0x10 + v1, dcoAttackWord);
    chip.writeReg(0x10 + v2, dcoAttackWord);
}

void CZ1Engine::noteOff(int midiNote, int /*velocity*/)
{
    for (auto& v : allocatedVoices)
    {
        if (v.active && v.midiNote == midiNote && !v.isReleased)
        {
            v.isReleased = true;
            const int v1 = v.v1;
            const int v2 = v.v2;
            Upd933Chip& chip = (v.chipIndex == 0) ? chip0 : chip1;
            const int kOffset = v.keyRateOffset;

            // Step 1 of release from log:
            // DCA: dir=1, rate=77+kOffset, sus=0, target=103
            v.dcaStage = 2;
            const uint8_t dcaRel1Rate = static_cast<uint8_t>(std::clamp(77 + kOffset, 0, 127));
            const uint16_t dcaRel1Word = static_cast<uint16_t>((1 << 15) | (dcaRel1Rate << 8) | 103);
            chip.writeReg(0x00 + v1, dcaRel1Word);
            chip.writeReg(0x00 + v2, dcaRel1Word);

            // DCW: 0xa780 (dir=1, rate=39, sus=1, target=0)
            v.dcwStage = 3;
            chip.writeReg(0x20 + v1, 0xa780);
            chip.writeReg(0x20 + v2, 0xa780);

            // DCO: 0xd780 (dir=1, rate=87, sus=1, target=0)
            v.dcoStage = 2;
            chip.writeReg(0x10 + v1, 0xd780);
            chip.writeReg(0x10 + v2, 0xd780);
        }
    }
}

void CZ1Engine::allNotesOff()
{
    for (auto& v : allocatedVoices)
    {
        if (v.active)
            noteOff(v.midiNote);
    }
}

int CZ1Engine::getActiveVoiceCount() const
{
    int count = 0;
    for (const auto& v : allocatedVoices)
    {
        if (v.active) ++count;
    }
    return count;
}

void CZ1Engine::processEnvelopeSequencer()
{
    for (auto& v : allocatedVoices)
    {
        if (!v.active) continue;

        const int v1 = v.v1;
        const int v2 = v.v2;
        Upd933Chip& chip = (v.chipIndex == 0) ? chip0 : chip1;
        const int kOffset = v.keyRateOffset;

        // DCA envelope sequencer
        if (v.dcaStage == 0) // Attack -> Decay/Sustain
        {
            if (chip.dca[v1].current >= chip.dca[v1].target)
            {
                v.dcaStage = 1;
                // Step 2: Decay towards sustain (dir=1, rate=105+kOffset, sus=1, target=122)
                const uint8_t dcaDecayRate = static_cast<uint8_t>(std::clamp(105 + kOffset, 0, 127));
                const uint16_t dcaDecayWord = static_cast<uint16_t>((1 << 15) | (dcaDecayRate << 8) | (1 << 7) | 122);
                chip.writeReg(0x00 + v1, dcaDecayWord);
                chip.writeReg(0x00 + v2, dcaDecayWord);
            }
        }
        else if (v.dcaStage == 2) // Rel 1 -> Rel 2
        {
            if (chip.dca[v1].current <= chip.dca[v1].target)
            {
                v.dcaStage = 3;
                // Release 2: dir=1, rate=67+kOffset, sus=0, target=0
                const uint8_t dcaRel2Rate = static_cast<uint8_t>(std::clamp(67 + kOffset, 0, 127));
                const uint16_t dcaRel2Word = static_cast<uint16_t>((1 << 15) | (dcaRel2Rate << 8) | 0);
                chip.writeReg(0x00 + v1, dcaRel2Word);
                chip.writeReg(0x00 + v2, dcaRel2Word);
            }
        }
        else if (v.dcaStage == 3) // Rel 2 -> Idle
        {
            if (chip.dca[v1].current <= 0)
            {
                v.dcaStage = 4;
                v.active = false;
                v.isReleased = false;
                chip.writeReg(0x00 + v1, 0xff80);
                chip.writeReg(0x00 + v2, 0xff80);
            }
        }

        // DCW envelope sequencer
        if (v.dcwStage == 0) // Attack -> Decay 1
        {
            if (chip.dcw[v1].current >= chip.dcw[v1].target)
            {
                v.dcwStage = 1;
                // Decay 1: 0xb84f (dir=1, rate=56, sus=0, target=79)
                chip.writeReg(0x20 + v1, 0xb84f);
                chip.writeReg(0x20 + v2, 0xb84f);
            }
        }
        else if (v.dcwStage == 1) // Decay 1 -> Decay 2 / Sustain
        {
            if (chip.dcw[v1].current <= chip.dcw[v1].target)
            {
                v.dcwStage = 2;
                // Sustain: 0x34d1 (dir=0, rate=52, sus=1, target=81)
                chip.writeReg(0x20 + v1, 0x34d1);
                chip.writeReg(0x20 + v2, 0x34d1);
            }
        }

        // DCO envelope sequencer
        if (v.dcoStage == 0) // Attack -> Decay to base
        {
            if (chip.dco[v1].current >= chip.dco[v1].target)
            {
                v.dcoStage = 1;
                // Decay to base: 0xd780 (dir=1, rate=87, sus=1, target=0)
                chip.writeReg(0x10 + v1, 0xd780);
                chip.writeReg(0x10 + v2, 0xd780);
            }
        }
    }
}

void CZ1Engine::renderNativeSample(float& outLeft, float& outRight)
{
    processEnvelopeSequencer();

    const int16_t raw0 = chip0.renderChipSample(cosineTable, volumeTable);
    const int16_t raw1 = chip1.renderChipSample(cosineTable, volumeTable);

    const float f0 = static_cast<float>(raw0) / 32768.0f;
    const float f1 = static_cast<float>(raw1) / 32768.0f;

    const float mono = (f0 + f1) * 0.5f;

    if (chorusEnabled)
    {
        // Smooth authentic BBD/analog chorus emulation
        chorusPhase += 0.00018f; // ~0.7 Hz LFO
        if (chorusPhase > 2.0f * static_cast<float>(M_PI))
            chorusPhase -= 2.0f * static_cast<float>(M_PI);

        chorusBuffer[chorusWritePos] = mono;

        const float modL = 140.0f + 40.0f * std::sin(chorusPhase);
        const float modR = 140.0f + 40.0f * std::sin(chorusPhase + static_cast<float>(M_PI));

        const int readIdxL = (chorusWritePos - static_cast<int>(modL) + CHORUS_BUF_SIZE) % CHORUS_BUF_SIZE;
        const int readIdxR = (chorusWritePos - static_cast<int>(modR) + CHORUS_BUF_SIZE) % CHORUS_BUF_SIZE;

        const float dL = chorusBuffer[readIdxL];
        const float dR = chorusBuffer[readIdxR];

        chorusWritePos = (chorusWritePos + 1) % CHORUS_BUF_SIZE;

        outLeft  = mono * 0.72f + dL * 0.42f;
        outRight = mono * 0.72f + dR * 0.42f;
    }
    else
    {
        outLeft  = mono;
        outRight = mono;
    }
}
