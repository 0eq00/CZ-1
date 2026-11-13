/*
  ==============================================================================
    CZ1SysExParser.h
    Casio CZ SysEx File Parser & Decoder for JUCE C++ Plugin
    Supports both:
      1. Casio CZ-1 format (144 bytes per patch, 288 nibbles, ASCII name, velocity)
      2. Casio CZ-101 / 1000 / 3000 / 5000 format (128 bytes per patch, 256 nibbles)
    Handles both standard MIDI SysEx framing (F0 44 ... F7) and raw binary dumps.
  ==============================================================================
*/

#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <cmath>
#include "CZ1FactoryPresets.h"

namespace CZSysEx
{
    struct VibratoDef
    {
        int wave = 1;  // 1=Tri, 2=SawUp, 3=SawDown, 4=Square
        int delay = 0; // 0..99
        int rate = 50; // 0..99
        int depth = 0; // 0..99
    };

    inline constexpr uint16_t kVibDelayWords[100] = {
        0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,
        0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,0x0010,0x0011,0x0012,0x0013,
        0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,
        0x001E,0x001F,0x0021,0x0023,0x0025,0x0027,0x0029,0x002B,0x002D,0x002F,
        0x0031,0x0033,0x0035,0x0037,0x0039,0x003B,0x003D,0x003F,0x0043,0x0047,
        0x004B,0x004F,0x0053,0x0057,0x005B,0x005F,0x0063,0x0067,0x006B,0x006F,
        0x0073,0x0077,0x007B,0x007F,0x0087,0x008F,0x0097,0x009F,0x00A7,0x00AF,
        0x00B7,0x00BF,0x00C7,0x00CF,0x00D7,0x00DF,0x00E7,0x00EF,0x00F7,0x00FF,
        0x010F,0x011F,0x012F,0x013F,0x014F,0x015F,0x016F,0x017F,0x018F,0x019F,
        0x01AF,0x01BF,0x01CF,0x01DF,0x01EF,0x01FF,0x021F,0x023F,0x025F,0x027F
    };

    inline constexpr uint16_t kVibRateWords[100] = {
        0x0020,0x0040,0x0060,0x0080,0x00A0,0x00C0,0x00E0,0x0000,0x0120,0x0140,
        0x0160,0x0180,0x01A0,0x01C0,0x01E0,0x0200,0x0220,0x0240,0x0260,0x0280,
        0x02A0,0x02C0,0x03E0,0x0300,0x0320,0x0340,0x0360,0x0380,0x03A0,0x03C0,
        0x03E0,0x0400,0x0460,0x04A0,0x04E0,0x0520,0x0560,0x05A0,0x05E0,0x0620,
        0x0660,0x06A0,0x06E0,0x0720,0x0760,0x07A0,0x07E0,0x0820,0x08E0,0x0960,
        0x09E0,0x0A60,0x0AE0,0x0B60,0x0BE0,0x0C60,0x0CE0,0x0D60,0x0DE0,0x0E60,
        0x0EE0,0x0F60,0x0FE0,0x1060,0x11E0,0x12E0,0x13E0,0x14E0,0x15E0,0x16E0,
        0x17E0,0x18E0,0x19E0,0x1AE0,0x1BE0,0x1CE0,0x1DE0,0x1EE0,0x1FE0,0x20E0,
        0x23E0,0x25E0,0x27E0,0x29E0,0x2BE0,0x2DE0,0x2FE0,0x31E0,0x33E0,0x35E0,
        0x37E0,0x39E0,0x3BE0,0x3DE0,0x3FE0,0x41E0,0x47E0,0x4BE0,0x4FE0,0x53E0
    };

    inline constexpr uint16_t kVibDepthWords[100] = {
        0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000A,
        0x000B,0x000C,0x000D,0x000E,0x000F,0x0010,0x0011,0x0012,0x0013,0x0014,
        0x0015,0x0016,0x0017,0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,
        0x001F,0x0020,0x0023,0x0025,0x0027,0x0029,0x002B,0x002D,0x002F,0x0031,
        0x0033,0x0035,0x0037,0x0039,0x003B,0x003D,0x003F,0x0041,0x0047,0x004B,
        0x004F,0x0053,0x0057,0x005B,0x005F,0x0063,0x0067,0x006B,0x006F,0x0073,
        0x0077,0x007B,0x007F,0x0083,0x008F,0x0097,0x009F,0x00A7,0x00AF,0x00B7,
        0x00BF,0x00C7,0x00CF,0x00D7,0x00DF,0x00E7,0x00EF,0x00F7,0x00FF,0x0107,
        0x011F,0x012F,0x013F,0x014F,0x015F,0x016F,0x017F,0x018F,0x019F,0x01AF,
        0x01BF,0x01CF,0x01DF,0x01EF,0x01FF,0x020F,0x023F,0x025F,0x027F,0x0300
    };

    static const char* const CZ101_FACTORY_NAMES[4][16] = {
        // Bank 1
        {
            "BRASS 1", "BRASS 2", "BRASS 3", "STRINGS 1",
            "STRINGS 2", "STRINGS 3", "STRINGS 4", "ORCHESTRA",
            "ACO.GUITAR", "JAZZ GUITAR", "ELEC.GUITAR", "SLAP BASS",
            "SYNTH.BASS", "ELEC.BASS 1", "ELEC.BASS 2", "HARP"
        },
        // Bank 2
        {
            "BRASS 4", "SAXOPHONE", "CELLO", "FLUTE",
            "WHISTLE", "HARMONICA", "RECORDER", "KOTO",
            "PIANO 1", "PIANO 2", "PIANO 3", "ELEC.PIANO",
            "HONKY-TONK", "FUNKY CLAVI.1", "FUNKY CLAVI.2", "HARPSICHORD"
        },
        // Bank 3
        {
            "JAZZ ORGAN 1", "JAZZ ORGAN 2", "PIPE ORGAN 1", "PIPE ORGAN 2",
            "ACCORDION", "VOICE 1", "VOICE 2", "VOICE 3",
            "MUSIC BOX", "VIBRAPHONE", "XYLOPHONE", "MARIMBA",
            "MALLET LOG", "AFRO-PERCUSSION", "BELLS", "METALLIC SOUND"
        },
        // Bank 4
        {
            "SYNTH.STRINGS", "FAT ENSEMBLE", "SITAR", "SYNTH.LEAD 1",
            "SYNTH.LEAD 2", "SYNTH.LEAD 3", "SYNTH.LEAD 4", "SWEEP SOUND 1",
            "SYNTH.DRUMS 1", "SYNTH.DRUMS 2", "CONGA", "STEEL DRUM",
            "SWEEP SOUND 2", "JET ROAR", "MOTORCYCLE", "TYPHOON SOUND"
        }
    };

    inline void decodeNibbles(const uint8_t* nibbles, size_t nibbleCount, std::vector<uint8_t>& outBytes)
    {
        outBytes.clear();
        outBytes.reserve(nibbleCount / 2);
        for (size_t i = 0; i < nibbleCount / 2; ++i)
        {
            const uint8_t low  = nibbles[i * 2] & 0x0f;
            const uint8_t high = nibbles[i * 2 + 1] & 0x0f;
            outBytes.push_back(static_cast<uint8_t>(low | (high << 4)));
        }
    }

    inline EnvelopeDef decodeEnvelope(const uint8_t* rawBytes, int lineOffset, int envOffset)
    {
        EnvelopeDef env{};
        const uint8_t header = rawBytes[lineOffset + envOffset];
        env.end = header & 0x07;
        env.sustain = -1;

        for (int s = 0; s < 8; ++s)
        {
            const uint8_t rByte = rawBytes[lineOffset + envOffset + 1 + s * 2];
            const uint8_t lByte = rawBytes[lineOffset + envOffset + 1 + s * 2 + 1];
            env.steps[s].rate = rByte & 0x7f;
            env.steps[s].level = lByte & 0x7f;
            env.steps[s].sustain = (lByte & 0x80) != 0;
            if (env.steps[s].sustain && env.sustain == -1)
            {
                env.sustain = static_cast<int8_t>(s);
            }
        }
        return env;
    }

    inline PresetLine decodeLine(const uint8_t* rawBytes, int offset, bool is144)
    {
        PresetLine line{};
        line.rawWord = static_cast<uint16_t>((rawBytes[offset] << 8) | rawBytes[offset + 1]);
        line.lineLevel = is144 ? static_cast<uint8_t>(std::clamp(15 - (rawBytes[offset + 2] >> 4), 1, 15)) : 15;
        line.dcaKeyFollow = rawBytes[offset + 2] & 0x0f;
        line.dcwKeyFollow = rawBytes[offset + 4] & 0x0f;

        line.dcaVelocity = is144 ? static_cast<uint8_t>(rawBytes[offset + 6] >> 4) : 0;
        line.dcwVelocity = is144 ? static_cast<uint8_t>(rawBytes[offset + 23] >> 4) : 0;
        line.dcoVelocity = is144 ? static_cast<uint8_t>(rawBytes[offset + 40] >> 4) : 0;

        line.dca = decodeEnvelope(rawBytes, offset, 6);
        line.dcw = decodeEnvelope(rawBytes, offset, 23);
        line.dco = decodeEnvelope(rawBytes, offset, 40);

        return line;
    }

    inline CZ1PresetDef decodeCZPatch(const uint8_t* rawBytes, size_t length, int index,
                                      const char* slotOverride = nullptr,
                                      const char* fallbackName = nullptr)
    {
        CZ1PresetDef p{};
        p.index = index;
        const bool is144 = (length >= 144);

        // Slot
        if (slotOverride != nullptr && slotOverride[0] != '\0')
        {
            std::strncpy(p.slot, slotOverride, sizeof(p.slot) - 1);
        }
        else
        {
            char bankLetter = static_cast<char>('A' + (index / 8));
            int toneNum = (index % 8) + 1;
            std::snprintf(p.slot, sizeof(p.slot), "%c-%d", bankLetter, toneNum);
        }

        // Name
        bool foundName = false;
        if (is144)
        {
            char tempName[17] = {0};
            for (int i = 0; i < 16; ++i)
            {
                uint8_t c = rawBytes[128 + i];
                if (c >= 32 && c <= 126)
                    tempName[i] = static_cast<char>(c);
                else
                    tempName[i] = '\0';
            }
            // Trim trailing spaces
            int endIdx = 15;
            while (endIdx >= 0 && (tempName[endIdx] == ' ' || tempName[endIdx] == '\0'))
            {
                tempName[endIdx] = '\0';
                endIdx--;
            }
            if (tempName[0] != '\0')
            {
                std::strncpy(p.name, tempName, sizeof(p.name) - 1);
                foundName = true;
            }
        }

        if (!foundName)
        {
            if (fallbackName != nullptr && fallbackName[0] != '\0')
                std::strncpy(p.name, fallbackName, sizeof(p.name) - 1);
            else
                std::snprintf(p.name, sizeof(p.name), "CZ Tone %d", index + 1);
        }

        // Byte 0: Line mode & octave
        p.lineMode = static_cast<int8_t>(rawBytes[0] & 3);
        const int octCode = (rawBytes[0] >> 2) & 3;
        if (octCode == 2)
            p.octave = -1;
        else if (octCode == 1)
            p.octave = 1;
        else
            p.octave = 0;

        // Bytes 1..3: Detune
        const float detuneSign = (rawBytes[1] & 1) ? -1.0f : 1.0f;
        const int detuneFine = rawBytes[2] >> 2;
        const int detuneNote = rawBytes[3];
        p.detuneSemitones = detuneSign * (detuneNote + detuneFine / 64.0f);

        // Lines 1 and 2
        p.lines[0] = decodeLine(rawBytes, 14, is144);
        p.lines[1] = decodeLine(rawBytes, 71, is144);

        return p;
    }

    inline VibratoDef decodeVibrato(const uint8_t* rawBytes)
    {
        VibratoDef v{};
        if (rawBytes[4] & 0x20) v.wave = 3;      // Saw Down
        else if (rawBytes[4] & 0x04) v.wave = 2; // Saw Up
        else if (rawBytes[4] & 0x02) v.wave = 4; // Square
        else v.wave = 1;                         // Triangle (0x08)

        v.delay = std::clamp(static_cast<int>(rawBytes[5]), 0, 99);
        v.rate  = std::clamp(static_cast<int>(rawBytes[8]), 0, 99);
        v.depth = std::clamp(static_cast<int>(rawBytes[11]), 0, 99);
        return v;
    }

    inline std::array<uint8_t, 144> encodeCZPatchTo144Bytes(const CZ1PresetDef& preset, const VibratoDef& vib = {})
    {
        std::array<uint8_t, 144> bytes{};
        bytes.fill(0);

        const int octCode = (preset.octave == 1) ? 1 : ((preset.octave == -1) ? 2 : 0);
        bytes[0] = static_cast<uint8_t>((preset.lineMode & 3) | (octCode << 2));

        // Bytes 1..3: Detune
        bytes[1] = (preset.detuneSemitones < 0.0f) ? 1 : 0;
        const float absSt = std::abs(preset.detuneSemitones);
        const int totalNote = std::clamp(static_cast<int>(std::floor(absSt + 1e-4f)), 0, 47);
        int hwFine = std::clamp(static_cast<int>(std::round((absSt - totalNote) * 60.0f)), 0, 60);
        if (hwFine >= 48) hwFine += 3;
        else if (hwFine >= 32) hwFine += 2;
        else if (hwFine >= 16) hwFine += 1;
        bytes[2] = static_cast<uint8_t>((hwFine & 0x3f) << 2);
        bytes[3] = static_cast<uint8_t>(totalNote);

        // Bytes 4..13: Vibrato
        const uint8_t vibFlag = (vib.wave == 2) ? 0x04 : (vib.wave == 3 ? 0x20 : (vib.wave == 4 ? 0x02 : 0x08));
        bytes[4] = vibFlag;
        const int dIdx = std::clamp(vib.delay, 0, 99);
        const int rIdx = std::clamp(vib.rate, 0, 99);
        const int depIdx = std::clamp(vib.depth, 0, 99);
        bytes[5]  = static_cast<uint8_t>(dIdx);
        bytes[6]  = static_cast<uint8_t>(kVibDelayWords[dIdx] & 0xff);
        bytes[7]  = static_cast<uint8_t>((kVibDelayWords[dIdx] >> 8) & 0xff);
        bytes[8]  = static_cast<uint8_t>(rIdx);
        bytes[9]  = static_cast<uint8_t>(kVibRateWords[rIdx] & 0xff);
        bytes[10] = static_cast<uint8_t>((kVibRateWords[rIdx] >> 8) & 0xff);
        bytes[11] = static_cast<uint8_t>(depIdx);
        bytes[12] = static_cast<uint8_t>(kVibDepthWords[depIdx] & 0xff);
        bytes[13] = static_cast<uint8_t>((kVibDepthWords[depIdx] >> 8) & 0xff);

        auto writeLine = [&bytes](int offset, const PresetLine& line)
        {
            bytes[offset]     = static_cast<uint8_t>((line.rawWord >> 8) & 0xff);
            bytes[offset + 1] = static_cast<uint8_t>(line.rawWord & 0xff);
            const int lvl = (line.lineLevel == 0) ? 15 : std::clamp(static_cast<int>(line.lineLevel), 1, 15);
            const uint8_t lvlNib = static_cast<uint8_t>(std::clamp(15 - lvl, 0, 15));
            bytes[offset + 2] = static_cast<uint8_t>((lvlNib << 4) | (line.dcaKeyFollow & 0x0f));
            bytes[offset + 3] = 0;
            bytes[offset + 4] = static_cast<uint8_t>(line.dcwKeyFollow & 0x0f);
            bytes[offset + 5] = 0;

            auto writeEnv = [&bytes, offset](int envOff, const EnvelopeDef& env, uint8_t vel)
            {
                bytes[offset + envOff] = static_cast<uint8_t>(((vel & 0x0f) << 4) | (env.end & 0x07));
                for (int s = 0; s < 8; ++s)
                {
                    const auto& st = env.steps[s];
                    const bool isSus = (env.sustain == s) || (env.sustain == -1 && st.sustain);
                    bytes[offset + envOff + 1 + s * 2]     = static_cast<uint8_t>(st.rate & 0x7f);
                    bytes[offset + envOff + 1 + s * 2 + 1] = static_cast<uint8_t>((st.level & 0x7f) | (isSus ? 0x80 : 0));
                }
            };

            writeEnv(6,  line.dca, line.dcaVelocity);
            writeEnv(23, line.dcw, line.dcwVelocity);
            writeEnv(40, line.dco, line.dcoVelocity);
        };

        writeLine(14, preset.lines[0]);
        writeLine(71, preset.lines[1]);

        // Bytes 128..143: 16-byte ASCII name
        std::string pname(preset.name);
        while (pname.size() < 16) pname.push_back(' ');
        for (int i = 0; i < 16; ++i)
        {
            const unsigned char ch = static_cast<unsigned char>(pname[static_cast<size_t>(i)]);
            bytes[128 + i] = (ch >= 32 && ch <= 126) ? ch : ' ';
        }

        return bytes;
    }

    inline std::vector<uint8_t> encodeBankToSyx(const std::vector<CZ1PresetDef>& presets, const std::vector<VibratoDef>& vibratos)
    {
        std::vector<uint8_t> out;
        out.reserve(presets.size() * 296);

        for (size_t i = 0; i < presets.size(); ++i)
        {
            const VibratoDef vib = (i < vibratos.size()) ? vibratos[i] : VibratoDef{};
            const auto raw144 = encodeCZPatchTo144Bytes(presets[i], vib);
            const uint8_t slotByte = static_cast<uint8_t>(0x20 + (i & 0x3f));

            out.push_back(0xf0);
            out.push_back(0x44);
            out.push_back(0x00);
            out.push_back(0x00);
            out.push_back(0x70);
            out.push_back(0x20);
            out.push_back(slotByte);

            for (size_t b = 0; b < 144; ++b)
            {
                out.push_back(static_cast<uint8_t>(raw144[b] & 0x0f));
                out.push_back(static_cast<uint8_t>((raw144[b] >> 4) & 0x0f));
            }
            out.push_back(0xf7);
        }

        return out;
    }

    struct SysExParseResult
    {
        bool success = false;
        std::string format = "Unknown";
        std::string bankName;
        std::vector<CZ1PresetDef> presets;
        std::vector<VibratoDef> vibratos;
        std::string errorMessage;
    };

    inline SysExParseResult parseSyx(const uint8_t* data, size_t size, const std::string& fileName = "bank.syx")
    {
        SysExParseResult res;
        res.bankName = fileName;

        if (size < 128)
        {
            res.success = false;
            res.errorMessage = "File is too small. Casio CZ patches require at least 128 bytes.";
            return res;
        }

        // Bank hint from filename (e.g. CZ1ORGB1.SYX -> Bank 1)
        int bankHint = -1;
        for (size_t i = 0; i + 1 < fileName.length(); ++i)
        {
            if ((fileName[i] == 'b' || fileName[i] == 'B') && (fileName[i + 1] >= '1' && fileName[i + 1] <= '4'))
            {
                bankHint = fileName[i + 1] - '1';
                break;
            }
        }

        // 1. Check for MIDI SysEx messages (F0 44 ... F7)
        size_t pos = 0;
        bool foundSysex = false;
        bool any144 = false;
        bool any128 = false;

        while (pos < size)
        {
            if (data[pos] == 0xf0)
            {
                size_t end = pos + 1;
                while (end < size && data[end] != 0xf7)
                    ++end;

                if (end < size && data[end] == 0xf7)
                {
                    foundSysex = true;
                    const size_t msgLen = end - pos + 1;
                    const uint8_t* msg = data + pos;
                    pos = end + 1;

                    if (msgLen >= 8 && msg[1] == 0x44) // Casio ID
                    {
                        const int patchIdx = static_cast<int>(res.presets.size());
                        const uint8_t* nibbles = nullptr;
                        size_t nibbleCount = 0;
                        bool is144 = false;

                        if (msgLen == 264 || msgLen == 265)
                        {
                            nibbles = msg + 7;
                            nibbleCount = 256;
                            is144 = false;
                        }
                        else if (msgLen == 296 || msgLen == 297)
                        {
                            nibbles = msg + 7;
                            nibbleCount = 288;
                            is144 = true;
                        }
                        else if (msgLen >= 8 + 288)
                        {
                            nibbles = (msg + msgLen - 1) - 288;
                            nibbleCount = 288;
                            is144 = true;
                        }
                        else if (msgLen >= 8 + 256)
                        {
                            nibbles = (msg + msgLen - 1) - 256;
                            nibbleCount = 256;
                            is144 = false;
                        }

                        if (nibbles != nullptr && (nibbleCount == 256 || nibbleCount == 288))
                        {
                            if (is144) any144 = true;
                            else any128 = true;

                            std::vector<uint8_t> rawBytes;
                            decodeNibbles(nibbles, nibbleCount, rawBytes);

                            const char* fallback = nullptr;
                            if (!is144 && bankHint >= 0 && bankHint < 4 && patchIdx < 16)
                            {
                                fallback = CZ101_FACTORY_NAMES[bankHint][patchIdx];
                            }

                            char slot[16];
                            char bankLetter = static_cast<char>('A' + (patchIdx / 8));
                            int toneNum = (patchIdx % 8) + 1;
                            std::snprintf(slot, sizeof(slot), "%c-%d", bankLetter, toneNum);

                            res.presets.push_back(decodeCZPatch(rawBytes.data(), rawBytes.size(), patchIdx, slot, fallback));
                            res.vibratos.push_back(decodeVibrato(rawBytes.data()));
                        }
                    }
                    continue;
                }
            }
            ++pos;
        }

        // 2. If no SysEx frames found, check for raw binary tone dumps
        if (!foundSysex && res.presets.empty())
        {
            if (size % 144 == 0)
            {
                any144 = true;
                const size_t count = size / 144;
                for (size_t i = 0; i < count; ++i)
                {
                    const uint8_t* rawBytes = data + i * 144;
                    char slot[16];
                    char bankLetter = static_cast<char>('A' + (i / 8));
                    int toneNum = static_cast<int>((i % 8) + 1);
                    std::snprintf(slot, sizeof(slot), "%c-%d", bankLetter, toneNum);
                    res.presets.push_back(decodeCZPatch(rawBytes, 144, static_cast<int>(i), slot));
                    res.vibratos.push_back(decodeVibrato(rawBytes));
                }
            }
            else if (size % 128 == 0)
            {
                any128 = true;
                const size_t count = size / 128;
                for (size_t i = 0; i < count; ++i)
                {
                    const uint8_t* rawBytes = data + i * 128;
                    char slot[16];
                    char bankLetter = static_cast<char>('A' + (i / 8));
                    int toneNum = static_cast<int>((i % 8) + 1);
                    std::snprintf(slot, sizeof(slot), "%c-%d", bankLetter, toneNum);

                    const char* fallback = nullptr;
                    if (bankHint >= 0 && bankHint < 4 && i < 16)
                    {
                        fallback = CZ101_FACTORY_NAMES[bankHint][i];
                    }
                    res.presets.push_back(decodeCZPatch(rawBytes, 128, static_cast<int>(i), slot, fallback));
                    res.vibratos.push_back(decodeVibrato(rawBytes));
                }
            }
        }

        if (res.presets.empty())
        {
            res.success = false;
            res.errorMessage = "No valid Casio CZ-1 or CZ-101 SysEx tone data was found in this file.";
            return res;
        }

        if (any144 && any128)
            res.format = "Mixed CZ Banks";
        else if (any144)
            res.format = "CZ-1 (144 bytes)";
        else
            res.format = "CZ-101 (128 bytes)";

        res.success = true;
        return res;
    }
}
