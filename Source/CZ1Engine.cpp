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
    noiseLfsr = 0x12345678;
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
        voice.ringMod   = (val >> 5) & 1;
        voice.pitchMod  = (val >> 3) & 3;
        voice.muteOther = (val >> 2) & 1;
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

int16_t CZ1Engine::Upd933Chip::renderVoiceSample(int vnum, const uint16_t* cosine, const uint16_t* volumeTable, const uint32_t* pitchTable, const uint16_t* pitchFine)
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

    // Update envelopes and pitch modulation
    const uint32_t oldDco = dco[vnum].current;
    const int16_t oldPm = voice.pmLevel;

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
            noiseLfsr ^= noiseLfsr << 13;
            noiseLfsr ^= noiseLfsr >> 17;
            noiseLfsr ^= noiseLfsr << 5;
            voice.pmLevel = (noiseLfsr & 1) ? static_cast<int16_t>(32 << NOTE_SHIFT) : 0;
            break;
        }
    }

    if (((oldDco ^ dco[vnum].current) >> ENV_DCO_SHIFT) || oldPm != voice.pmLevel)
        updatePitchStep(vnum, pitchTable, pitchFine);

    voice.position += voice.pitchStep;
    lastSample = sample;
    return sample;
}

int16_t CZ1Engine::Upd933Chip::renderChipSample(const uint16_t* cosine, const uint16_t* volumeTable, const uint32_t* pitchTable, const uint16_t* pitchFine)
{
    int32_t total = 0;
    static const int voiceMap[] = {5, 0, 7, 2, 1, 4, 3, 6};
    for (int v : voiceMap)
    {
        total += renderVoiceSample(v, cosine, volumeTable, pitchTable, pitchFine);
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

    applyPresetWaveforms();
}

void CZ1Engine::applyPresetWaveforms()
{
    if (currentPresetIndex == 0)
    {
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
        return;
    }

    const auto& preset = CZ1_FACTORY_PRESETS[currentPresetIndex];
    const int lineMode = preset.lineMode;

    const uint16_t wordLine1 = preset.lines[0].rawWord;
    const uint16_t wordLine2 = (lineMode == 2) ? preset.lines[0].rawWord : preset.lines[1].rawWord;

    const bool isRingMod = (wordLine1 >> 5) & 1;
    const uint8_t pitchMod = (wordLine1 >> 3) & 3;
    const bool isNoiseMod = (pitchMod == 3);

    // Line 1: waveforms & window from wordLine1.
    // Line 1 is the melodic tonal carrier (rendered first in the pair).
    const uint16_t regWordV1 = static_cast<uint16_t>(wordLine1 & 0xffc0);

    // Line 2: waveforms & window from wordLine2.
    // Ring Modulation and Noise Modulation are applied on Line 2 (which modulates Line 1).
    const uint16_t regWordV2 = static_cast<uint16_t>((wordLine2 & 0xffc0) | (isRingMod ? 0x20 : 0) | (pitchMod == 2 ? 0x10 : 0) | (isNoiseMod ? 0x18 : 0) | (wordLine1 & 0x04));

    for (int i = 0; i < NUM_VOICE_PAIRS; ++i)
    {
        const int chipIdx = voicePairs[i].chipIndex;
        const int v1 = voicePairs[i].v1;
        const int v2 = voicePairs[i].v2;
        Upd933Chip& chip = (chipIdx == 0) ? chip0 : chip1;

        chip.writeReg(0x68 + v1, regWordV1);
        chip.writeReg(0x68 + v2, regWordV2);

        chip.writeReg(0x00 + v1, 0xff80);
        chip.writeReg(0x00 + v2, 0xff80);
        chip.writeReg(0x20 + v1, 0x7f80);
        chip.writeReg(0x20 + v2, 0x7f80);
        chip.writeReg(0x10 + v1, 0x7f80);
        chip.writeReg(0x10 + v2, 0x7f80);
    }
}

void CZ1Engine::writeDcaStep(Upd933Chip& chip, int vnum, const EnvelopeDef& envDef, int stage, float velScale, int keyFollow, int note, bool isReleased)
{
    uint32_t target = 0;
    uint32_t rate = 0;
    uint32_t sus = 0;

    if (stage >= envDef.end)
    {
        target = 0;
        const auto& step = envDef.steps[std::min(stage, 7)];
        rate = step.rate ? step.rate : 67;
        sus = 0;
    }
    else
    {
        const auto& step = envDef.steps[stage];
        target = static_cast<uint32_t>(std::clamp(static_cast<int>(std::round(step.level * velScale)), 0, 127));
        rate = step.rate;
        sus = (stage == envDef.sustain && !isReleased) ? 1 : 0;
    }

    // Authentic Casio CZ-1 Key Follow rate scaling:
    // In the uPD933 chip, every +8 in the rate register doubles the envelope speed.
    // Setting keyFollow = 9 provides full keyboard tracking, calibrated to hardware MAME debug log.
    // Base rate is calibrated around Middle C (MIDI Key 60), scaling from -1 at low keys up to +28 at key 108.
    if (keyFollow > 0)
    {
        const int baseOffset = getKeyRateOffset(note);
        const int rateOffset = static_cast<int>(std::round(baseOffset * (keyFollow / 9.0f)));
        rate = static_cast<uint32_t>(std::clamp(static_cast<int>(rate) + rateOffset, 0, 127));
    }
    rate = std::clamp(rate, 0u, 127u);

    const uint32_t currentVal = chip.dca[vnum].current >> (ENV_DCA_SHIFT + 2);
    const uint32_t dir = (target < currentVal) ? 1 : 0;
    const uint16_t regWord = static_cast<uint16_t>((dir << 15) | (rate << 8) | (sus << 7) | (target & 0x7f));
    chip.writeReg(0x00 + vnum, regWord);
}

void CZ1Engine::writeDcwStep(Upd933Chip& chip, int vnum, const EnvelopeDef& envDef, int stage, float velScale, int keyFollow, int note, bool isReleased)
{
    uint32_t target = 0;
    uint32_t rate = 0;
    uint32_t sus = 0;

    if (stage >= envDef.end)
    {
        target = 0;
        const auto& step = envDef.steps[std::min(stage, 7)];
        rate = step.rate;
        sus = isReleased ? 1 : 0;
    }
    else
    {
        const auto& step = envDef.steps[stage];
        target = static_cast<uint32_t>(std::clamp(static_cast<int>(std::round(step.level * velScale)), 0, 127));
        rate = step.rate;
        sus = (stage == envDef.sustain && !isReleased) ? 1 : 0;
    }

    if (keyFollow > 0)
    {
        // DCW rate scales mildly with key follow (approx. half the slope of DCA)
        const int baseOffset = getKeyRateOffset(note);
        const int dcwRateOffset = static_cast<int>(std::round(baseOffset * (keyFollow / 18.0f)));
        rate = static_cast<uint32_t>(std::clamp(static_cast<int>(rate) + dcwRateOffset, 0, 127));

        // DCW Key Follow reduces upper harmonics on high notes (approaches pure sine wave)
        if (note > 36)
        {
            const float rollOff = 1.0f - (keyFollow / 9.0f) * ((note - 36) / 96.0f) * 0.25f;
            target = static_cast<uint32_t>(std::clamp(static_cast<int>(std::round(target * std::max(0.65f, rollOff))), 0, 127));
        }
    }
    rate = std::clamp(rate, 0u, 127u);

    const uint32_t currentVal = chip.dcw[vnum].current >> (ENV_DCW_SHIFT + 3);
    const uint32_t dir = (target < currentVal) ? 1 : 0;
    const uint16_t regWord = static_cast<uint16_t>((dir << 15) | (rate << 8) | (sus << 7) | (target & 0x7f));
    chip.writeReg(0x20 + vnum, regWord);
}

void CZ1Engine::writeDcoStep(Upd933Chip& chip, int vnum, const EnvelopeDef& envDef, int stage, float velScale, int keyFollow, int note, bool isReleased)
{
    uint32_t target = 0;
    uint32_t rate = 0;
    uint32_t sus = 0;

    if (stage >= envDef.end)
    {
        target = 0;
        const auto& step = envDef.steps[std::min(stage, 7)];
        rate = step.rate;
        sus = isReleased ? 1 : 0;
    }
    else
    {
        const auto& step = envDef.steps[stage];
        // Preserves bit 6 (level units: 0 = 6.25 cents/unit, 1 = 2 semitones/unit)
        const uint8_t rawLvl = step.level & 0x7f;
        const bool bit6 = (rawLvl >> 6) & 1;
        const int units = rawLvl & 0x3f;
        const int scaledUnits = std::clamp(static_cast<int>(std::round(units * velScale)), 0, 63);
        target = static_cast<uint32_t>(scaledUnits | (bit6 ? 0x40 : 0));
        rate = step.rate;
        sus = (stage == envDef.sustain && !isReleased) ? 1 : 0;
    }

    // DCO pitch envelope does NOT have key follow in CZ synths
    rate = std::clamp(rate, 0u, 127u);

    uint32_t targetPitch = (target & 0x3f) << (ENV_DCO_SHIFT + 5);
    if ((target >> 6) & 1)
        targetPitch <<= 5;

    const uint32_t currentVal = chip.dco[vnum].current;
    const uint32_t dir = (targetPitch < currentVal) ? 1 : 0;
    const uint16_t regWord = static_cast<uint16_t>((dir << 15) | (rate << 8) | (sus << 7) | (target & 0x7f));
    chip.writeReg(0x10 + vnum, regWord);
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

uint8_t CZ1Engine::getDcaAttackTarget(int velocity)
{
    const int v = std::clamp(velocity, 0, 127);
    if (v <= 28) return 117;
    if (v <= 41) return 118;
    if (v <= 48) return 119;
    if (v <= 55) return 120;
    if (v <= 65) return 121;
    if (v <= 73) return 122;
    if (v <= 81) return 123;
    if (v <= 87) return 124;
    if (v <= 95) return 125;
    if (v <= 101) return 126;
    return 127;
}

uint8_t CZ1Engine::getDcaSustainTarget(int velocity)
{
    return getDcaAttackTarget(velocity) - 1;
}

uint8_t CZ1Engine::getDcaRelease1Target(int velocity)
{
    return getDcaAttackTarget(velocity) - 20;
}

uint8_t CZ1Engine::getDcwAttackTarget(int velocity)
{
    const int v = std::clamp(velocity, 0, 127);
    if (v <= 29) return 88;
    if (v <= 32) return 89;
    if (v <= 36) return 90;
    if (v <= 40) return 91;
    if (v <= 42) return 92;
    if (v <= 44) return 93;
    if (v <= 45) return 94;
    if (v <= 47) return 95;
    if (v <= 49) return 96;
    if (v <= 50) return 97;
    if (v <= 51) return 98;
    if (v <= 53) return 99;
    if (v <= 55) return 100;
    if (v <= 59) return 101;
    if (v <= 61) return 102;
    if (v <= 63) return 103;
    if (v <= 67) return 104;
    if (v <= 69) return 105;
    if (v <= 71) return 106;
    if (v <= 73) return 107;
    if (v <= 75) return 108;
    if (v <= 77) return 109;
    if (v <= 79) return 111;
    if (v <= 81) return 112;
    if (v <= 83) return 113;
    if (v <= 85) return 114;
    if (v <= 87) return 115;
    if (v <= 89) return 116;
    if (v <= 91) return 117;
    if (v <= 93) return 118;
    if (v <= 95) return 120;
    if (v <= 97) return 121;
    if (v <= 99) return 122;
    if (v <= 101) return 123;
    return 127;
}

uint8_t CZ1Engine::getDcwDecay1Target(int velocity)
{
    return getDcwAttackTarget(velocity) - 33;
}

uint8_t CZ1Engine::getDcoAttackTarget(int velocity)
{
    const int v = std::clamp(velocity, 0, 127);
    if (v <= 28) return 3;
    if (v <= 29) return 4;
    if (v <= 32) return 5;
    if (v <= 34) return 6;
    if (v <= 36) return 7;
    if (v <= 40) return 8;
    if (v <= 41) return 9;
    if (v <= 42) return 10;
    if (v <= 44) return 11;
    if (v <= 45) return 12;
    if (v <= 46) return 13;
    if (v <= 48) return 14;
    if (v <= 49) return 15;
    if (v <= 50) return 16;
    if (v <= 52) return 17;
    if (v <= 53) return 18;
    if (v <= 55) return 19;
    if (v <= 59) return 20;
    if (v <= 61) return 21;
    if (v <= 63) return 22;
    if (v <= 67) return 23;
    if (v <= 69) return 24;
    if (v <= 71) return 25;
    if (v <= 75) return 26;
    if (v <= 77) return 27;
    if (v <= 81) return 29;
    if (v <= 83) return 30;
    if (v <= 85) return 31;
    if (v <= 89) return 32;
    return 33;
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

    // 3b. Priority: Steal a voice that is sustained ONLY by damper pedal (!v.keyHeld)
    int bestPedalSustainedIdx = -1;
    uint64_t oldestPedalSustainedTime = 0xffffffffffffffffULL;
    for (int i = 0; i < NUM_VOICE_PAIRS; ++i)
    {
        if (allocatedVoices[i].active && !allocatedVoices[i].isReleased && !allocatedVoices[i].keyHeld)
        {
            if (allocatedVoices[i].noteOnCounter < oldestPedalSustainedTime)
            {
                oldestPedalSustainedTime = allocatedVoices[i].noteOnCounter;
                bestPedalSustainedIdx = i;
            }
        }
    }
    if (bestPedalSustainedIdx != -1)
        return bestPedalSustainedIdx;

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

    const int vel = std::clamp(velocity, 1, 127);
    voice.midiNote = midiNote;
    voice.velocity = vel;
    voice.active = true;
    voice.pairIndex = idx;
    voice.chipIndex = voicePairs[idx].chipIndex;
    voice.v1 = voicePairs[idx].v1;
    voice.v2 = voicePairs[idx].v2;
    voice.dcaStage = 0;
    voice.dcwStage = 0;
    voice.dcoStage = 0;
    voice.line1 = LineVoiceState();
    voice.line2 = LineVoiceState();
    voice.isReleased = false;
    voice.keyHeld = true;
    voice.noteOnCounter = ++globalNoteCounter;

    const int kOffset = getKeyRateOffset(midiNote);
    voice.keyRateOffset = kOffset;

    const int v1 = voice.v1;
    const int v2 = voice.v2;
    Upd933Chip& chip = (voice.chipIndex == 0) ? chip0 : chip1;

    if (currentPresetIndex == 0)
    {
        // =====================================================================
        // PRESET 0: EXACT HARDCODED MAME CZ-1 DEBUG LOG BASELINE (A-1 BRASS 1)
        // 100% bit-for-bit preserved from hardware debug log
        // =====================================================================
        const int shift = (midiNote - 60) * 512;
        const uint16_t pitchV1 = static_cast<uint16_t>(std::clamp(0x5200 + shift, 0, 0xffff));
        const uint16_t pitchV2 = static_cast<uint16_t>(std::clamp(0x51d8 + shift, 0, 0xffff));

        chip.voices[v1].pmLevel = 0;
        chip.voices[v2].pmLevel = 0;

        chip.writeReg(0x60 + v1, pitchV1);
        chip.writeReg(0x60 + v2, pitchV2);

        chip.updatePitchStep(v1, pitchTable, pitchFine);
        chip.updatePitchStep(v2, pitchTable, pitchFine);

        // Reset phase counters (0x0080 for Line 1, 0x0000 for Line 2 from log)
        chip.writeReg(0x98 + v1, 0x0080);
        chip.writeReg(0x98 + v2, 0x0000);

        // Step 1: DCA Attack (dir=0, rate=99+kOffset, sus=0, target from velocity calibration: 117..127)
        const uint8_t dcaAttackTarget = getDcaAttackTarget(vel);
        const uint8_t dcaAttackRate = static_cast<uint8_t>(std::clamp(99 + kOffset, 0, 127));
        const uint16_t dcaAttackWord = static_cast<uint16_t>((dcaAttackRate << 8) | dcaAttackTarget);
        chip.writeReg(0x00 + v1, dcaAttackWord);
        chip.writeReg(0x00 + v2, dcaAttackWord);

        // Step 1: DCW Attack (dir=0, rate=98+kOffset, sus=0, target from velocity calibration: 88..127)
        const uint8_t dcwAttackTarget = getDcwAttackTarget(vel);
        const uint8_t dcwAttackRate = static_cast<uint8_t>(std::clamp(98 + kOffset, 0, 127));
        const uint16_t dcwAttackWord = static_cast<uint16_t>((dcwAttackRate << 8) | dcwAttackTarget);
        chip.writeReg(0x20 + v1, dcwAttackWord);
        chip.writeReg(0x20 + v2, dcwAttackWord);

        // Step 1: DCO Attack (dir=0, rate=93+kOffset, sus=0, target from velocity calibration: 3..33)
        const uint8_t dcoAttackTarget = getDcoAttackTarget(vel);
        const uint8_t dcoAttackRate = static_cast<uint8_t>(std::clamp(93 + kOffset, 0, 127));
        const uint16_t dcoAttackWord = static_cast<uint16_t>((dcoAttackRate << 8) | dcoAttackTarget);
        chip.writeReg(0x10 + v1, dcoAttackWord);
        chip.writeReg(0x10 + v2, dcoAttackWord);
        return;
    }

    // =====================================================================
    // PRESETS 1..63: MULTI-STEP SYSEX PRESETS
    // =====================================================================
    const auto& preset = CZ1_FACTORY_PRESETS[currentPresetIndex];
    const int lineMode = preset.lineMode;
    const auto& lineDef1 = preset.lines[0];
    const auto& lineDef2 = (lineMode == 2) ? preset.lines[0] : preset.lines[1];

    const bool hasLine1 = (lineMode != 1);
    const bool hasLine2 = (lineMode != 0);

    const int baseOctShift = preset.octave * 12 * 512;
    const int keyShift = (midiNote - 60) * 512;
    // Authentic Middle C (MIDI Key 60) standard Octave 0 baseline is 0x6a00 (C4 = 261.6 Hz).
    // Note: BRASS 1 has octave = -1, which evaluates to 0x6a00 - 0x1800 = 0x5200 (C3 = 130.8 Hz).
    const uint16_t basePitch = static_cast<uint16_t>(std::clamp(0x6a00 + keyShift + baseOctShift, 0, 0xffff));

    const int detuneShift = static_cast<int>(std::round(preset.detuneSemitones * 512.0f));
    const uint16_t line2Pitch = static_cast<uint16_t>(std::clamp(static_cast<int>(basePitch) + detuneShift, 0, 0xffff));

    chip.voices[v1].pmLevel = 0;
    chip.voices[v2].pmLevel = 0;

    chip.writeReg(0x60 + v1, basePitch);
    chip.writeReg(0x60 + v2, hasLine2 ? line2Pitch : basePitch);

    chip.updatePitchStep(v1, pitchTable, pitchFine);
    chip.updatePitchStep(v2, pitchTable, pitchFine);

    // Reset phase counters
    chip.writeReg(0x98 + v1, 0x0080);
    chip.writeReg(0x98 + v2, 0x0000);

    const float velScaleDca1 = 1.0f - ((127 - vel) * lineDef1.dcaVelocity) / (127.0f * 15.0f);
    const float velScaleDcw1 = 1.0f - ((127 - vel) * lineDef1.dcwVelocity) / (127.0f * 15.0f);
    const float velScaleDco1 = 1.0f - ((127 - vel) * lineDef1.dcoVelocity) / (127.0f * 15.0f);

    const float velScaleDca2 = 1.0f - ((127 - vel) * lineDef2.dcaVelocity) / (127.0f * 15.0f);
    const float velScaleDcw2 = 1.0f - ((127 - vel) * lineDef2.dcwVelocity) / (127.0f * 15.0f);
    const float velScaleDco2 = 1.0f - ((127 - vel) * lineDef2.dcoVelocity) / (127.0f * 15.0f);

    if (hasLine1)
    {
        writeDcaStep(chip, v1, lineDef1.dca, 0, velScaleDca1, lineDef1.dcaKeyFollow, midiNote, false);
        writeDcwStep(chip, v1, lineDef1.dcw, 0, velScaleDcw1, lineDef1.dcwKeyFollow, midiNote, false);
        writeDcoStep(chip, v1, lineDef1.dco, 0, velScaleDco1, 0, midiNote, false);
    }
    else
    {
        chip.writeReg(0x00 + v1, 0xff80);
        voice.line1.finished = true;
    }

    if (hasLine2)
    {
        writeDcaStep(chip, v2, lineDef2.dca, 0, velScaleDca2, lineDef2.dcaKeyFollow, midiNote, false);
        writeDcwStep(chip, v2, lineDef2.dcw, 0, velScaleDcw2, lineDef2.dcwKeyFollow, midiNote, false);
        writeDcoStep(chip, v2, lineDef2.dco, 0, velScaleDco2, 0, midiNote, false);
    }
    else
    {
        chip.writeReg(0x00 + v2, 0xff80);
        voice.line2.finished = true;
    }
}

void CZ1Engine::triggerVoiceRelease(NoteVoice& v)
{
    if (!v.active || v.isReleased) return;
    v.isReleased = true;

    const auto& preset = CZ1_FACTORY_PRESETS[currentPresetIndex];
    const int lineMode = preset.lineMode;
    const auto& lineDef1 = preset.lines[0];
    const auto& lineDef2 = (lineMode == 2) ? preset.lines[0] : preset.lines[1];
    const bool hasLine1 = (lineMode != 1);
    const bool hasLine2 = (lineMode != 0);

    const int v1 = v.v1;
    const int v2 = v.v2;
    Upd933Chip& chip = (v.chipIndex == 0) ? chip0 : chip1;
    const int kOffset = v.keyRateOffset;
    const int midiNote = v.midiNote;

    if (currentPresetIndex == 0)
    {
        // =============================================================
        // PRESET 0: EXACT HARDCODED MAME CZ-1 DEBUG LOG RELEASE BASELINE
        // =============================================================
        v.dcaStage = 2;
        const uint8_t dcaRel1Target = getDcaRelease1Target(v.velocity);
        const uint8_t dcaRel1Rate = static_cast<uint8_t>(std::clamp(77 + kOffset, 0, 127));
        const uint16_t dcaRel1Word = static_cast<uint16_t>((1 << 15) | (dcaRel1Rate << 8) | dcaRel1Target);
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
        return;
    }

    // =============================================================
    // PRESETS 1..63: MULTI-STEP SYSEX RELEASE
    // =============================================================
    const int vel = v.velocity;
    const float velScaleDca1 = 1.0f - ((127 - vel) * lineDef1.dcaVelocity) / (127.0f * 15.0f);
    const float velScaleDcw1 = 1.0f - ((127 - vel) * lineDef1.dcwVelocity) / (127.0f * 15.0f);
    const float velScaleDco1 = 1.0f - ((127 - vel) * lineDef1.dcoVelocity) / (127.0f * 15.0f);

    const float velScaleDca2 = 1.0f - ((127 - vel) * lineDef2.dcaVelocity) / (127.0f * 15.0f);
    const float velScaleDcw2 = 1.0f - ((127 - vel) * lineDef2.dcwVelocity) / (127.0f * 15.0f);
    const float velScaleDco2 = 1.0f - ((127 - vel) * lineDef2.dcoVelocity) / (127.0f * 15.0f);

    if (hasLine1 && !v.line1.finished)
    {
        if (lineDef1.dca.sustain != -1)
        {
            if (v.line1.dcaStage <= lineDef1.dca.sustain)
                v.line1.dcaStage = lineDef1.dca.sustain + 1;
        }
        else
        {
            v.line1.dcaStage = lineDef1.dca.end;
        }
        if (v.line1.dcaStage > lineDef1.dca.end) v.line1.dcaStage = lineDef1.dca.end;
        writeDcaStep(chip, v1, lineDef1.dca, v.line1.dcaStage, velScaleDca1, lineDef1.dcaKeyFollow, midiNote, true);

        if (lineDef1.dcw.sustain != -1)
        {
            if (v.line1.dcwStage <= lineDef1.dcw.sustain)
                v.line1.dcwStage = lineDef1.dcw.sustain + 1;
        }
        else
        {
            v.line1.dcwStage = lineDef1.dcw.end;
        }
        if (v.line1.dcwStage > lineDef1.dcw.end) v.line1.dcwStage = lineDef1.dcw.end;
        writeDcwStep(chip, v1, lineDef1.dcw, v.line1.dcwStage, velScaleDcw2, lineDef1.dcwKeyFollow, midiNote, true);

        if (lineDef1.dco.sustain != -1)
        {
            if (v.line1.dcoStage <= lineDef1.dco.sustain)
                v.line1.dcoStage = lineDef1.dco.sustain + 1;
        }
        else
        {
            v.line1.dcoStage = lineDef1.dco.end;
        }
        if (v.line1.dcoStage > lineDef1.dco.end) v.line1.dcoStage = lineDef1.dco.end;
        writeDcoStep(chip, v1, lineDef1.dco, v.line1.dcoStage, velScaleDco1, 0, midiNote, true);
    }

    if (hasLine2 && !v.line2.finished)
    {
        if (lineDef2.dca.sustain != -1)
        {
            if (v.line2.dcaStage <= lineDef2.dca.sustain)
                v.line2.dcaStage = lineDef2.dca.sustain + 1;
        }
        else
        {
            v.line2.dcaStage = lineDef2.dca.end;
        }
        if (v.line2.dcaStage > lineDef2.dca.end) v.line2.dcaStage = lineDef2.dca.end;
        writeDcaStep(chip, v2, lineDef2.dca, v.line2.dcaStage, velScaleDca2, lineDef2.dcaKeyFollow, midiNote, true);

        if (lineDef2.dcw.sustain != -1)
        {
            if (v.line2.dcwStage <= lineDef2.dcw.sustain)
                v.line2.dcwStage = lineDef2.dcw.sustain + 1;
        }
        else
        {
            v.line2.dcwStage = lineDef2.dcw.end;
        }
        if (v.line2.dcwStage > lineDef2.dcw.end) v.line2.dcwStage = lineDef2.dcw.end;
        writeDcwStep(chip, v2, lineDef2.dcw, v.line2.dcwStage, velScaleDcw2, lineDef2.dcwKeyFollow, midiNote, true);

        if (lineDef2.dco.sustain != -1)
        {
            if (v.line2.dcoStage <= lineDef2.dco.sustain)
                v.line2.dcoStage = lineDef2.dco.sustain + 1;
        }
        else
        {
            v.line2.dcoStage = lineDef2.dco.end;
        }
        if (v.line2.dcoStage > lineDef2.dco.end) v.line2.dcoStage = lineDef2.dco.end;
        writeDcoStep(chip, v2, lineDef2.dco, v.line2.dcoStage, velScaleDco2, 0, midiNote, true);
    }
}

void CZ1Engine::noteOff(int midiNote, int /*velocity*/)
{
    for (auto& v : allocatedVoices)
    {
        if (v.active && v.midiNote == midiNote && !v.isReleased)
        {
            v.keyHeld = false;
            if (sustainPedal)
            {
                continue;
            }
            triggerVoiceRelease(v);
        }
    }
}

void CZ1Engine::setSustainPedal(bool down)
{
    sustainPedal = down;
    if (!down)
    {
        for (auto& v : allocatedVoices)
        {
            if (v.active && !v.isReleased && !v.keyHeld)
            {
                triggerVoiceRelease(v);
            }
        }
    }
}

void CZ1Engine::handleControlChange(int controller, int value)
{
    if (controller == 64)
    {
        setSustainPedal(value >= 64);
    }
}

void CZ1Engine::allNotesOff()
{
    sustainPedal = false;
    for (auto& v : allocatedVoices)
    {
        if (v.active && !v.isReleased)
        {
            v.keyHeld = false;
            triggerVoiceRelease(v);
        }
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
    const auto& preset = CZ1_FACTORY_PRESETS[currentPresetIndex];
    const int lineMode = preset.lineMode;
    const auto& lineDef1 = preset.lines[0];
    const auto& lineDef2 = (lineMode == 2) ? preset.lines[0] : preset.lines[1];
    const bool hasLine1 = (lineMode != 1);
    const bool hasLine2 = (lineMode != 0);

    for (auto& v : allocatedVoices)
    {
        if (!v.active) continue;

        const int v1 = v.v1;
        const int v2 = v.v2;
        Upd933Chip& chip = (v.chipIndex == 0) ? chip0 : chip1;
        const int kOffset = v.keyRateOffset;

        if (currentPresetIndex == 0)
        {
            // =================================================================
            // PRESET 0: EXACT HARDCODED MAME CZ-1 DEBUG LOG ENVELOPE SEQUENCER
            // =================================================================
            if (v.dcaStage == 0) // Attack -> Decay/Sustain
            {
                if (chip.dca[v1].current >= chip.dca[v1].target)
                {
                    v.dcaStage = 1;
                    // Step 2: Decay towards sustain (dir=1, rate=105+kOffset, sus=1, target from velocity: 116..126)
                    const uint8_t dcaDecayTarget = getDcaSustainTarget(v.velocity);
                    const uint8_t dcaDecayRate = static_cast<uint8_t>(std::clamp(105 + kOffset, 0, 127));
                    const uint16_t dcaDecayWord = static_cast<uint16_t>((1 << 15) | (dcaDecayRate << 8) | (1 << 7) | dcaDecayTarget);
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
                    // Decay 1: dir=1, rate=56, sus=0, target from velocity (55..94)
                    const uint8_t dcwDecayTarget = getDcwDecay1Target(v.velocity);
                    const uint16_t dcwDecayWord = static_cast<uint16_t>((1 << 15) | (56 << 8) | dcwDecayTarget);
                    chip.writeReg(0x20 + v1, dcwDecayWord);
                    chip.writeReg(0x20 + v2, dcwDecayWord);
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
            continue;
        }

        // =====================================================================
        // PRESETS 1..63: MULTI-STEP SYSEX ENVELOPE SEQUENCER
        // =====================================================================
        const int vel = v.velocity;
        const int note = v.midiNote;
        const bool isReleased = v.isReleased;

        const float velScaleDca1 = 1.0f - ((127 - vel) * lineDef1.dcaVelocity) / (127.0f * 15.0f);
        const float velScaleDcw1 = 1.0f - ((127 - vel) * lineDef1.dcwVelocity) / (127.0f * 15.0f);
        const float velScaleDco1 = 1.0f - ((127 - vel) * lineDef1.dcoVelocity) / (127.0f * 15.0f);

        const float velScaleDca2 = 1.0f - ((127 - vel) * lineDef2.dcaVelocity) / (127.0f * 15.0f);
        const float velScaleDcw2 = 1.0f - ((127 - vel) * lineDef2.dcwVelocity) / (127.0f * 15.0f);
        const float velScaleDco2 = 1.0f - ((127 - vel) * lineDef2.dcoVelocity) / (127.0f * 15.0f);

        // Line 1 Progression
        if (hasLine1 && !v.line1.finished)
        {
            if (chip.dca[v1].current == chip.dca[v1].target)
            {
                if (v.line1.dcaStage == lineDef1.dca.sustain && !isReleased)
                {
                    // Hold sustain
                }
                else if (lineDef1.dca.sustain == -1 && v.line1.dcaStage == lineDef1.dca.end - 1 && lineDef1.dca.end > 1 && !isReleased)
                {
                    // Preset without sustain point: stays at decay stage until note-off or natural decay to silence
                    if (chip.dca[v1].current == 0)
                    {
                        v.line1.finished = true;
                        chip.writeReg(0x00 + v1, 0xff80);
                    }
                }
                else if (v.line1.dcaStage < lineDef1.dca.end)
                {
                    v.line1.dcaStage++;
                    writeDcaStep(chip, v1, lineDef1.dca, v.line1.dcaStage, velScaleDca1, lineDef1.dcaKeyFollow, note, isReleased);
                }
                else
                {
                    if (chip.dca[v1].current == 0)
                    {
                        v.line1.finished = true;
                        chip.writeReg(0x00 + v1, 0xff80);
                    }
                }
            }

            if (chip.dcw[v1].current == chip.dcw[v1].target)
            {
                if (v.line1.dcwStage == lineDef1.dcw.sustain && !isReleased)
                {
                    // Hold sustain
                }
                else if (v.line1.dcwStage < lineDef1.dcw.end)
                {
                    v.line1.dcwStage++;
                    writeDcwStep(chip, v1, lineDef1.dcw, v.line1.dcwStage, velScaleDcw1, lineDef1.dcwKeyFollow, note, isReleased);
                }
            }

            if (chip.dco[v1].current == chip.dco[v1].target)
            {
                if (v.line1.dcoStage == lineDef1.dco.sustain && !isReleased)
                {
                    // Hold sustain
                }
                else if (v.line1.dcoStage < lineDef1.dco.end)
                {
                    v.line1.dcoStage++;
                    writeDcoStep(chip, v1, lineDef1.dco, v.line1.dcoStage, velScaleDco1, 0, note, isReleased);
                }
            }
        }

        // Line 2 Progression
        if (hasLine2 && !v.line2.finished)
        {
            if (chip.dca[v2].current == chip.dca[v2].target)
            {
                if (v.line2.dcaStage == lineDef2.dca.sustain && !isReleased)
                {
                    // Hold sustain
                }
                else if (lineDef2.dca.sustain == -1 && v.line2.dcaStage == lineDef2.dca.end - 1 && lineDef2.dca.end > 1 && !isReleased)
                {
                    // Preset without sustain point: stays at decay stage until note-off or natural decay to silence
                    if (chip.dca[v2].current == 0)
                    {
                        v.line2.finished = true;
                        chip.writeReg(0x00 + v2, 0xff80);
                    }
                }
                else if (v.line2.dcaStage < lineDef2.dca.end)
                {
                    v.line2.dcaStage++;
                    writeDcaStep(chip, v2, lineDef2.dca, v.line2.dcaStage, velScaleDca2, lineDef2.dcaKeyFollow, note, isReleased);
                }
                else
                {
                    if (chip.dca[v2].current == 0)
                    {
                        v.line2.finished = true;
                        chip.writeReg(0x00 + v2, 0xff80);
                    }
                }
            }

            if (chip.dcw[v2].current == chip.dcw[v2].target)
            {
                if (v.line2.dcwStage == lineDef2.dcw.sustain && !isReleased)
                {
                    // Hold sustain
                }
                else if (v.line2.dcwStage < lineDef2.dcw.end)
                {
                    v.line2.dcwStage++;
                    writeDcwStep(chip, v2, lineDef2.dcw, v.line2.dcwStage, velScaleDcw2, lineDef2.dcwKeyFollow, note, isReleased);
                }
            }

            if (chip.dco[v2].current == chip.dco[v2].target)
            {
                if (v.line2.dcoStage == lineDef2.dco.sustain && !isReleased)
                {
                    // Hold sustain
                }
                else if (v.line2.dcoStage < lineDef2.dco.end)
                {
                    v.line2.dcoStage++;
                    writeDcoStep(chip, v2, lineDef2.dco, v.line2.dcoStage, velScaleDco2, 0, note, isReleased);
                }
            }
        }

        const bool line1Done = !hasLine1 || v.line1.finished;
        const bool line2Done = !hasLine2 || v.line2.finished;
        if (line1Done && line2Done)
        {
            v.active = false;
            v.isReleased = false;
            chip.writeReg(0x00 + v1, 0xff80);
            chip.writeReg(0x00 + v2, 0xff80);
            chip.writeReg(0x20 + v1, 0x7f80);
            chip.writeReg(0x20 + v2, 0x7f80);
            chip.writeReg(0x10 + v1, 0x7f80);
            chip.writeReg(0x10 + v2, 0x7f80);
        }
    }
}

void CZ1Engine::renderNativeSample(float& outLeft, float& outRight)
{
    processEnvelopeSequencer();

    const int16_t raw0 = chip0.renderChipSample(cosineTable, volumeTable, pitchTable, pitchFine);
    const int16_t raw1 = chip1.renderChipSample(cosineTable, volumeTable, pitchTable, pitchFine);

    const float f0 = static_cast<float>(raw0) / 10000.0f;
    const float f1 = static_cast<float>(raw1) / 10000.0f;

    const float mono = std::tanh(f0 + f1);

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

void CZ1Engine::loadPreset(int presetIndex)
{
    currentPresetIndex = std::clamp(presetIndex, 0, CZ1_NUM_FACTORY_PRESETS - 1);
    applyPresetWaveforms();
}

const char* CZ1Engine::getPresetName(int presetIndex) const
{
    const int idx = std::clamp(presetIndex, 0, CZ1_NUM_FACTORY_PRESETS - 1);
    return CZ1_FACTORY_PRESETS[idx].name;
}

const char* CZ1Engine::getPresetSlot(int presetIndex) const
{
    const int idx = std::clamp(presetIndex, 0, CZ1_NUM_FACTORY_PRESETS - 1);
    return CZ1_FACTORY_PRESETS[idx].slot;
}

