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

float CZ1Engine::Upd933Chip::renderChipSample(const uint16_t* cosine, const uint16_t* volumeTable, const uint32_t* pitchTable, const uint16_t* pitchFine)
{
    int32_t total = 0;
    static const int voiceMap[] = {5, 0, 7, 2, 1, 4, 3, 6};
    for (int v : voiceMap)
    {
        total += renderVoiceSample(v, cosine, volumeTable, pitchTable, pitchFine);
    }
    sampleCount++;
    return static_cast<float>(total) / 32768.0f;
}

// -----------------------------------------------------------------------------
// CZ1Engine Main Class
// -----------------------------------------------------------------------------

CZ1Engine::CZ1Engine()
{
    initTables();
    resetFactoryPresets();
    reset();
}

void CZ1Engine::resetFactoryPresets()
{
    activePresets.assign(CZ1_FACTORY_PRESETS, CZ1_FACTORY_PRESETS + CZ1_NUM_FACTORY_PRESETS);
    currentBankName = "CZ-1 Factory ROM (64 Patches)";
    currentBankFormat = "CZ-1 (144 bytes)";
    currentPresetIndex = 0;
    applyPresetWaveforms();
}

bool CZ1Engine::loadSyxData(const uint8_t* data, size_t size, const std::string& bankName)
{
    auto result = CZSysEx::parseSyx(data, size, bankName);
    if (!result.success || result.presets.empty())
        return false;

    activePresets = std::move(result.presets);
    currentBankName = result.bankName;
    currentBankFormat = result.format;
    currentPresetIndex = 0;
    applyPresetWaveforms();
    return true;
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
    if (activePresets.empty())
        return;

    const auto& preset = getCurrentPresetDef();
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
            const float kfRatio = keyFollow / 9.0f;
            const float noteRatio = std::clamp((note - 36) / 72.0f, 0.0f, 1.0f);
            const float rollOff = std::max(0.05f, 1.0f - kfRatio * noteRatio * 0.85f);
            target = static_cast<uint32_t>(std::clamp(static_cast<int>(std::round(target * rollOff)), 0, 127));
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
    // Calibrated directly against MAME hardware register logs:
    // Key 21/48: 0, Key 60: 1, Key 72: 4, Key 84: 11, Key 96: 23, Key 108: 40
    if (midiNote <= 48)
        return 0;

    const float x = (midiNote - 48) / 12.0f;
    float off = x * x;
    if (x > 2.0f)
        off += std::pow(x - 2.0f, 2.0f) * 1.666f;

    return std::min(64, static_cast<int>(std::round(off)));
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
    const auto& preset = getCurrentPresetDef();
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

    const int line1Note = std::clamp(midiNote + preset.octave * 12, 0, 127);
    const int line2Note = std::clamp(static_cast<int>(std::round(midiNote + preset.octave * 12 + preset.detuneSemitones)), 0, 127);
    voice.line1Note = line1Note;
    voice.line2Note = line2Note;

    if (hasLine1)
    {
        writeDcaStep(chip, v1, lineDef1.dca, 0, velScaleDca1, lineDef1.dcaKeyFollow, line1Note, false);
        writeDcwStep(chip, v1, lineDef1.dcw, 0, velScaleDcw1, lineDef1.dcwKeyFollow, line1Note, false);
        writeDcoStep(chip, v1, lineDef1.dco, 0, velScaleDco1, 0, line1Note, false);
    }
    else
    {
        chip.writeReg(0x00 + v1, 0xff80);
        voice.line1.finished = true;
    }

    if (hasLine2)
    {
        writeDcaStep(chip, v2, lineDef2.dca, 0, velScaleDca2, lineDef2.dcaKeyFollow, line2Note, false);
        writeDcwStep(chip, v2, lineDef2.dcw, 0, velScaleDcw2, lineDef2.dcwKeyFollow, line2Note, false);
        writeDcoStep(chip, v2, lineDef2.dco, 0, velScaleDco2, 0, line2Note, false);
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

    const auto& preset = getCurrentPresetDef();
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

    const int vel = v.velocity;
    const float velScaleDca1 = 1.0f - ((127 - vel) * lineDef1.dcaVelocity) / (127.0f * 15.0f);
    const float velScaleDcw1 = 1.0f - ((127 - vel) * lineDef1.dcwVelocity) / (127.0f * 15.0f);
    const float velScaleDco1 = 1.0f - ((127 - vel) * lineDef1.dcoVelocity) / (127.0f * 15.0f);

    const float velScaleDca2 = 1.0f - ((127 - vel) * lineDef2.dcaVelocity) / (127.0f * 15.0f);
    const float velScaleDcw2 = 1.0f - ((127 - vel) * lineDef2.dcwVelocity) / (127.0f * 15.0f);
    const float velScaleDco2 = 1.0f - ((127 - vel) * lineDef2.dcoVelocity) / (127.0f * 15.0f);

    const int line1Note = v.line1Note;
    const int line2Note = v.line2Note;

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
        writeDcaStep(chip, v1, lineDef1.dca, v.line1.dcaStage, velScaleDca1, lineDef1.dcaKeyFollow, line1Note, true);

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
        writeDcwStep(chip, v1, lineDef1.dcw, v.line1.dcwStage, velScaleDcw1, lineDef1.dcwKeyFollow, line1Note, true);

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
        writeDcoStep(chip, v1, lineDef1.dco, v.line1.dcoStage, velScaleDco1, 0, line1Note, true);
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
        writeDcaStep(chip, v2, lineDef2.dca, v.line2.dcaStage, velScaleDca2, lineDef2.dcaKeyFollow, line2Note, true);

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
        writeDcwStep(chip, v2, lineDef2.dcw, v.line2.dcwStage, velScaleDcw2, lineDef2.dcwKeyFollow, line2Note, true);

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
        writeDcoStep(chip, v2, lineDef2.dco, v.line2.dcoStage, velScaleDco2, 0, line2Note, true);
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
    const auto& preset = getCurrentPresetDef();
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

        const int vel = v.velocity;
        const int note = v.midiNote;
        const int line1Note = v.line1Note;
        const int line2Note = v.line2Note;
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
                else if (v.line1.dcaStage < lineDef1.dca.end)
                {
                    v.line1.dcaStage++;
                    writeDcaStep(chip, v1, lineDef1.dca, v.line1.dcaStage, velScaleDca1, lineDef1.dcaKeyFollow, line1Note, isReleased);
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
                    writeDcwStep(chip, v1, lineDef1.dcw, v.line1.dcwStage, velScaleDcw1, lineDef1.dcwKeyFollow, line1Note, isReleased);
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
                    writeDcoStep(chip, v1, lineDef1.dco, v.line1.dcoStage, velScaleDco1, 0, line1Note, isReleased);
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
                else if (v.line2.dcaStage < lineDef2.dca.end)
                {
                    v.line2.dcaStage++;
                    writeDcaStep(chip, v2, lineDef2.dca, v.line2.dcaStage, velScaleDca2, lineDef2.dcaKeyFollow, line2Note, isReleased);
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
                    writeDcwStep(chip, v2, lineDef2.dcw, v.line2.dcwStage, velScaleDcw2, lineDef2.dcwKeyFollow, line2Note, isReleased);
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
                    writeDcoStep(chip, v2, lineDef2.dco, v.line2.dcoStage, velScaleDco2, 0, line2Note, isReleased);
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

static inline float softLimit(float x)
{
    constexpr float threshold = 0.85f;
    if (std::abs(x) <= threshold)
        return x;
    const float excess = std::abs(x) - threshold;
    constexpr float margin = 1.0f - threshold;
    const float compressed = threshold + margin * std::tanh(excess / margin);
    return (x < 0.0f) ? -compressed : compressed;
}

void CZ1Engine::renderNativeSample(float& outLeft, float& outRight)
{
    processEnvelopeSequencer();

    const float f0 = chip0.renderChipSample(cosineTable, volumeTable, pitchTable, pitchFine);
    const float f1 = chip1.renderChipSample(cosineTable, volumeTable, pitchTable, pitchFine);

    const float mono = (f0 + f1) * 0.65f;

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

        outLeft  = softLimit(mono * 0.70f + dL * 0.35f);
        outRight = softLimit(mono * 0.70f + dR * 0.35f);
    }
    else
    {
        outLeft  = softLimit(f0 * 0.75f + f1 * 0.25f);
        outRight = softLimit(f0 * 0.25f + f1 * 0.75f);
    }
}

void CZ1Engine::loadPreset(int presetIndex)
{
    if (activePresets.empty())
        return;
    currentPresetIndex = std::clamp(presetIndex, 0, static_cast<int>(activePresets.size()) - 1);
    applyPresetWaveforms();
}

const char* CZ1Engine::getPresetName(int presetIndex) const
{
    if (activePresets.empty())
        return "Unknown";
    const int idx = std::clamp(presetIndex, 0, static_cast<int>(activePresets.size()) - 1);
    return activePresets[idx].name;
}

const char* CZ1Engine::getPresetSlot(int presetIndex) const
{
    if (activePresets.empty())
        return "U-01";
    const int idx = std::clamp(presetIndex, 0, static_cast<int>(activePresets.size()) - 1);
    return activePresets[idx].slot;
}

const CZ1PresetDef& CZ1Engine::getPresetDef(int presetIndex) const
{
    static CZ1PresetDef dummy{};
    if (activePresets.empty())
        return dummy;
    const int idx = std::clamp(presetIndex, 0, static_cast<int>(activePresets.size()) - 1);
    return activePresets[idx];
}

const CZ1PresetDef& CZ1Engine::getCurrentPresetDef() const
{
    return getPresetDef(currentPresetIndex);
}

const char* CZ1Engine::getWaveformName(uint8_t waveIndex)
{
    static const char* const names[] = {
        "Sawtooth", "Square", "Pulse", "Double Sine",
        "Saw-Pulse", "Reso Saw", "Reso Triangle", "Reso Trapezoid"
    };
    return names[waveIndex & 7];
}

const char* CZ1Engine::getWindowName(uint8_t windowIndex)
{
    static const char* const names[] = {
        "Off", "Sawtooth", "Triangle", "Trapezoid", "Pulse", "Double Saw", "Double Saw", "Double Saw"
    };
    return names[windowIndex & 7];
}

const char* CZ1Engine::getLineModeName(int8_t lineMode)
{
    switch (lineMode)
    {
        case 0: return "LINE 1 (Single Line, 8v)";
        case 1: return "LINE 2 (Single Line, 8v)";
        case 2: return "LINE 1+1' (Detuned Dual, 4v)";
        case 3: return "LINE 1+2' (Distinct Dual, 4v)";
        default: return "LINE 1";
    }
}


