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

#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <cmath>
#include "CZ1FactoryPresets.h"

namespace CZSysEx
{
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
        line.dcaKeyFollow = rawBytes[offset + 2] & 0x0f;
        line.dcwKeyFollow = rawBytes[offset + 4] & 0x0f;

        line.dcaVelocity = is144 ? static_cast<uint8_t>((rawBytes[offset + 3] >> 4) & 0x0f) : 0;
        line.dcwVelocity = is144 ? static_cast<uint8_t>(rawBytes[offset + 3] & 0x0f) : 0;
        line.dcoVelocity = 0;

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

    struct SysExParseResult
    {
        bool success = false;
        std::string format = "Unknown";
        std::string bankName;
        std::vector<CZ1PresetDef> presets;
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
