/*
  ==============================================================================
    CZ1FactoryPresets.h
    Casio CZ-1 64 Factory ROM Presets (A-1 through H-8)
    Directly extracted and verified from MAME CZ-1 firmware ROM dump (MAME-CZ-1.syx).
  ==============================================================================
*/

#pragma once

#include <cstdint>
#include <cstddef>

#pragma pack(push, 1)

struct EnvelopeStepDef
{
    uint8_t rate;
    uint8_t level;
    bool sustain;
};

struct EnvelopeDef
{
    uint8_t end;
    int8_t sustain; // -1 for none, 0..7 for sustain step
    EnvelopeStepDef steps[8];
};

struct PresetLine
{
    uint16_t rawWord;
    uint8_t wave1;
    uint8_t wave2;
    uint8_t window;
    uint8_t modulation;
    uint8_t lineLevel;
    uint8_t dcaKeyFollow;
    uint8_t dcwKeyFollow;
    uint8_t dcaVelocity;
    uint8_t dcwVelocity;
    uint8_t dcoVelocity;
    EnvelopeDef dco;
    EnvelopeDef dcw;
    EnvelopeDef dca;
};

struct CZ1PresetDef
{
    int index;
    char slot[8];
    char name[20];
    int8_t lineMode;
    int8_t octave;
    float detuneSemitones;
    PresetLine lines[2];
};

#pragma pack(pop)

static constexpr size_t CZ1_NUM_FACTORY_PRESETS = 64;

static const CZ1PresetDef CZ1_FACTORY_PRESETS[CZ1_NUM_FACTORY_PRESETS] = {
    // [A-1] BRASS 1
    {
        0,
        "A-1",
        "BRASS 1",
        2,
        -1,
        -0.078125f,
        {
            {
            40992, 5, 5, 0, 4, 0,
            9, 0, 1, 4, 11,
            { 1, -1, { { 93, 33, false }, { 87, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 98, 127, false }, { 56, 94, false }, { 52, 96, true }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 1, { { 98, 127, false }, { 104, 126, true }, { 76, 107, false }, { 66, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            40960, 5, 5, 0, 0, 0,
            9, 0, 1, 4, 11,
            { 1, -1, { { 93, 33, false }, { 87, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 98, 127, false }, { 56, 94, false }, { 52, 96, true }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 1, { { 98, 127, false }, { 104, 126, true }, { 76, 107, false }, { 66, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [A-2] BRASS 2
    {
        1,
        "A-2",
        "BRASS 2",
        3,
        0,
        12.078125f,
        {
            {
            42496, 5, 1, 0, 0, 0,
            0, 0, 3, 9, 3,
            { 1, -1, { { 127, 33, false }, { 87, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 4, 2, { { 114, 118, false }, { 50, 25, false }, { 36, 65, true }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 4, 2, { { 114, 78, false }, { 92, 127, false }, { 80, 121, true }, { 94, 87, false }, { 66, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            22016, 2, 5, 0, 0, 0,
            0, 0, 6, 6, 1,
            { 1, -1, { { 127, 33, false }, { 87, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 4, 2, { { 94, 127, false }, { 70, 52, false }, { 52, 74, true }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 4, 2, { { 116, 96, false }, { 92, 127, false }, { 80, 119, true }, { 94, 87, false }, { 66, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [A-3] BRASS 3
    {
        2,
        "A-3",
        "BRASS 3",
        2,
        0,
        0.09375f,
        {
            {
            0, 0, 0, 0, 0, 0,
            0, 0, 4, 0, 0,
            { 1, -1, { { 127, 33, false }, { 87, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 99, 127, false }, { 64, 123, false }, { 52, 66, true }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 4, 2, { { 119, 78, false }, { 92, 127, false }, { 80, 119, true }, { 94, 87, false }, { 39, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            0, 0, 0, 0, 0, 0,
            0, 0, 4, 0, 0,
            { 1, -1, { { 127, 33, false }, { 87, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 99, 127, false }, { 64, 123, false }, { 52, 66, true }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 4, 2, { { 119, 78, false }, { 92, 127, false }, { 80, 119, true }, { 94, 87, false }, { 39, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [A-4] STRINGS 1
    {
        3,
        "A-4",
        "STRINGS 1",
        2,
        0,
        0.09375f,
        {
            {
            0, 0, 0, 0, 0, 0,
            0, 4, 6, 2, 6,
            { 1, -1, { { 78, 12, false }, { 61, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 110, 127, false }, { 24, 109, true }, { 58, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 90, 105, false }, { 66, 127, true }, { 54, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            0, 0, 0, 0, 0, 0,
            0, 4, 6, 2, 6,
            { 1, -1, { { 78, 12, false }, { 61, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 110, 127, false }, { 24, 109, true }, { 58, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 90, 105, false }, { 66, 127, true }, { 54, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [A-5] STRINGS 2
    {
        4,
        "A-5",
        "STRINGS 2",
        3,
        0,
        -0.09375f,
        {
            {
            0, 0, 0, 0, 0, 0,
            0, 0, 4, 1, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 92, 115, false }, { 50, 83, true }, { 44, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 79, 127, false }, { 50, 125, true }, { 48, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            0, 0, 0, 0, 0, 0,
            0, 0, 5, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 1, { { 92, 121, false }, { 32, 102, true }, { 44, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 79, 121, false }, { 14, 105, true }, { 48, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [A-6] STRINGS 3
    {
        5,
        "A-6",
        "STRINGS 3",
        2,
        0,
        0.1875f,
        {
            {
            0, 0, 0, 0, 0, 0,
            4, 7, 4, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 127, 121, true }, { 56, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 81, 127, false }, { 36, 121, true }, { 37, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            0, 0, 0, 0, 0, 0,
            4, 7, 4, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 127, 121, true }, { 56, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 81, 127, false }, { 36, 121, true }, { 37, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [A-7] STRINGS 4
    {
        6,
        "A-7",
        "STRINGS 4",
        2,
        -1,
        0.046875f,
        {
            {
            32768, 4, 4, 0, 0, 0,
            0, 4, 5, 0, 0,
            { 2, 1, { { 127, 69, false }, { 71, 70, true }, { 0, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 124, 127, false }, { 80, 102, true }, { 58, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 87, 100, false }, { 42, 127, false }, { 34, 117, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            32768, 4, 4, 0, 0, 0,
            0, 4, 5, 0, 0,
            { 2, 1, { { 127, 69, false }, { 71, 70, true }, { 0, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 124, 127, false }, { 80, 102, true }, { 58, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 87, 100, false }, { 42, 127, false }, { 34, 117, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [A-8] ORCHESTRA
    {
        7,
        "A-8",
        "ORCHESTRA",
        3,
        0,
        12.109375f,
        {
            {
            43520, 5, 2, 0, 0, 0,
            4, 0, 4, 0, 3,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 4, 1, { { 86, 127, false }, { 65, 89, true }, { 36, 30, false }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 1, { { 97, 126, false }, { 92, 127, true }, { 51, 49, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            38400, 4, 5, 0, 0, 0,
            0, 0, 4, 2, 1,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 4, 1, { { 88, 127, false }, { 63, 76, true }, { 52, 74, false }, { 39, 0, false }, { 90, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 0, { { 88, 127, true }, { 61, 85, false }, { 85, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [B-1] ACO.GUITAR
    {
        8,
        "B-1",
        "ACO.GUITAR",
        3,
        -1,
        19.046875f,
        {
            {
            32, 0, 0, 0, 4, 0,
            4, 8, 4, 5, 0,
            { 1, -1, { { 118, 57, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 4, 1, { { 127, 127, false }, { 127, 98, true }, { 127, 64, false }, { 60, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 116, 113, false }, { 36, 0, false }, { 86, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            32768, 4, 4, 0, 0, 0,
            3, 1, 4, 2, 0,
            { 2, 1, { { 100, 58, false }, { 115, 0, true }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 127, 106, false }, { 66, 0, true }, { 74, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 39, 0, true }, { 70, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [B-2] JAZZ GUITAR
    {
        9,
        "B-2",
        "JAZZ GUITAR",
        3,
        -1,
        19.015625f,
        {
            {
            32, 0, 0, 0, 4, 0,
            3, 1, 9, 15, 0,
            { 1, -1, { { 102, 56, false }, { 105, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 122, 127, false }, { 127, 32, false }, { 127, 64, true }, { 41, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 108, 127, false }, { 30, 0, false }, { 86, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            0, 0, 0, 0, 0, 0,
            4, 3, 2, 6, 0,
            { 1, -1, { { 103, 52, false }, { 102, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 127, 127, false }, { 127, 112, false }, { 63, 89, true }, { 34, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 36, 0, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [B-3] ELEC.GUITAR
    {
        10,
        "B-3",
        "ELEC.GUITAR",
        3,
        1,
        -12.046875f,
        {
            {
            45568, 5, 4, 0, 0, 0,
            0, 0, 3, 0, 0,
            { 1, -1, { { 105, 73, false }, { 121, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 118, 116, false }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 119, 127, false }, { 32, 55, false }, { 87, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            32768, 4, 4, 0, 0, 0,
            0, 0, 3, 12, 0,
            { 1, -1, { { 91, 60, false }, { 91, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 113, 127, false }, { 53, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 46, 120, true }, { 119, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [B-4] SLAP BASS
    {
        11,
        "B-4",
        "SLAP BASS",
        3,
        -1,
        41.046875f,
        {
            {
            38432, 4, 5, 0, 4, 0,
            0, 0, 0, 0, 0,
            { 1, -1, { { 127, 72, false }, { 121, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 4, 1, { { 127, 51, false }, { 99, 0, true }, { 68, 46, false }, { 46, 127, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 40, 0, true }, { 70, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            35328, 4, 2, 0, 0, 0,
            0, 0, 15, 4, 0,
            { 1, 0, { { 127, 69, true }, { 25, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 127, 100, false }, { 104, 0, true }, { 77, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 60, 0, true }, { 61, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [B-5] SYNTH.BASS
    {
        12,
        "B-5",
        "SYNTH.BASS",
        2,
        -1,
        0.0f,
        {
            {
            10784, 1, 2, 0, 4, 0,
            6, 7, 2, 4, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 124, 127, false }, { 59, 112, false }, { 68, 106, true }, { 70, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 34, 0, true }, { 73, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            10752, 1, 2, 0, 0, 0,
            6, 7, 2, 4, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 124, 127, false }, { 59, 112, false }, { 68, 106, true }, { 70, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 34, 0, true }, { 73, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [B-6] ELEC.BASS 1
    {
        13,
        "B-6",
        "ELEC.BASS 1",
        2,
        -1,
        0.09375f,
        {
            {
            4608, 0, 4, 0, 0, 0,
            9, 9, 0, 8, 0,
            { 1, -1, { { 89, 69, false }, { 115, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, -1, { { 125, 102, false }, { 68, 64, false }, { 48, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 116, 127, false }, { 36, 0, false }, { 75, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            4608, 0, 4, 0, 0, 0,
            9, 9, 0, 8, 0,
            { 1, -1, { { 89, 69, false }, { 115, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, -1, { { 125, 102, false }, { 68, 64, false }, { 48, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 116, 127, false }, { 36, 0, false }, { 75, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [B-7] ELEC.BASS 2
    {
        14,
        "B-7",
        "ELEC.BASS 2",
        3,
        -1,
        -12.109375f,
        {
            {
            35328, 4, 2, 0, 0, 0,
            1, 0, 3, 10, 0,
            { 1, -1, { { 127, 52, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, -1, { { 113, 111, false }, { 53, 41, false }, { 59, 94, false }, { 21, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 119, 127, false }, { 30, 108, false }, { 18, 60, true }, { 98, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            16384, 2, 2, 0, 0, 0,
            9, 9, 1, 9, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, -1, { { 89, 107, false }, { 36, 96, false }, { 24, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 111, 127, false }, { 13, 42, false }, { 119, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [B-8] HARP
    {
        15,
        "B-8",
        "HARP",
        2,
        0,
        0.046875f,
        {
            {
            8224, 1, 1, 0, 4, 0,
            5, 3, 7, 3, 2,
            { 1, -1, { { 127, 33, false }, { 106, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 127, 127, false }, { 68, 0, true }, { 16, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 127, false }, { 42, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            8192, 1, 1, 0, 0, 0,
            5, 3, 7, 3, 2,
            { 1, -1, { { 127, 33, false }, { 106, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 127, 127, false }, { 68, 0, true }, { 16, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 127, false }, { 42, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [C-1] BRASS 4
    {
        16,
        "C-1",
        "BRASS 4",
        3,
        0,
        0.03125f,
        {
            {
            8192, 1, 1, 0, 0, 0,
            2, 0, 3, 5, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 69, 76, true }, { 78, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, 0, { { 91, 109, true }, { 70, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            16384, 2, 2, 0, 0, 0,
            2, 2, 5, 3, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 88, 101, false }, { 60, 118, false }, { 44, 92, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 4, 3, { { 91, 127, false }, { 91, 72, false }, { 91, 127, false }, { 28, 117, true }, { 57, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [C-2] SAXOPHONE
    {
        17,
        "C-2",
        "SAXOPHONE",
        3,
        -1,
        32.15625f,
        {
            {
            4640, 0, 4, 0, 4, 0,
            7, 9, 4, 4, 0,
            { 1, 0, { { 127, 70, true }, { 0, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 127, 119, true }, { 90, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, 0, { { 90, 127, true }, { 82, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            38400, 4, 5, 0, 0, 0,
            9, 9, 0, 0, 0,
            { 1, 0, { { 109, 41, true }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 100, 6, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 1, { { 119, 127, false }, { 60, 71, true }, { 90, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [C-3] CELLO
    {
        18,
        "C-3",
        "CELLO",
        0,
        0,
        0.0f,
        {
            {
            4608, 0, 4, 0, 0, 0,
            2, 0, 3, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 4, 1, { { 127, 127, false }, { 84, 84, true }, { 45, 61, false }, { 110, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 74, 96, false }, { 40, 127, false }, { 30, 98, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            4608, 0, 4, 0, 0, 0,
            2, 0, 3, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 4, 1, { { 127, 127, false }, { 84, 84, true }, { 45, 61, false }, { 110, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 74, 96, false }, { 40, 127, false }, { 30, 98, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [C-4] FLUTE
    {
        19,
        "C-4",
        "FLUTE",
        3,
        0,
        0.0f,
        {
            {
            40984, 5, 5, 0, 3, 0,
            0, 3, 4, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 80, 73, false }, { 63, 46, false }, { 42, 57, true }, { 38, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 119, 64, false }, { 84, 119, false }, { 70, 127, true }, { 72, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            0, 0, 0, 0, 0, 0,
            2, 3, 7, 3, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 100, 48, false }, { 33, 0, false }, { 68, 6, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 62, 45, false }, { 24, 41, false }, { 38, 0, true }, { 62, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [C-5] WHISTLE
    {
        20,
        "C-5",
        "WHISTLE",
        0,
        0,
        0.0f,
        {
            {
            8192, 1, 1, 0, 0, 0,
            0, 0, 2, 0, 0,
            { 2, 1, { { 127, 69, false }, { 51, 70, true }, { 0, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 0, -1, { { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 82, 116, false }, { 55, 127, false }, { 37, 103, true }, { 72, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            8192, 1, 1, 0, 0, 0,
            0, 0, 2, 0, 0,
            { 2, 1, { { 127, 69, false }, { 51, 70, true }, { 0, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 0, -1, { { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 82, 116, false }, { 55, 127, false }, { 37, 103, true }, { 72, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [C-6] HARMONICA
    {
        21,
        "C-6",
        "HARMONICA",
        3,
        -1,
        0.03125f,
        {
            {
            41472, 5, 0, 0, 0, 0,
            0, 0, 5, 6, 0,
            { 1, 0, { { 124, 76, true }, { 57, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 116, 35, false }, { 36, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 84, 127, false }, { 28, 0, false }, { 84, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            41472, 5, 0, 0, 0, 0,
            0, 0, 5, 2, 0,
            { 2, 0, { { 124, 76, true }, { 57, 70, false }, { 0, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, -1, { { 116, 127, false }, { 36, 0, false }, { 112, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 84, 115, false }, { 28, 0, false }, { 84, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [C-7] RECORDER
    {
        22,
        "C-7",
        "RECORDER",
        3,
        0,
        24.09375f,
        {
            {
            8224, 1, 1, 0, 4, 0,
            0, 0, 4, 0, 0,
            { 2, 1, { { 127, 60, false }, { 66, 70, true }, { 0, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 0, { { 116, 76, true }, { 42, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 81, 127, false }, { 25, 33, false }, { 72, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            8192, 1, 1, 0, 0, 0,
            0, 4, 4, 0, 0,
            { 2, -1, { { 119, 78, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 0, { { 119, 29, true }, { 50, 0, false }, { 72, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, 0, { { 88, 99, true }, { 72, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [C-8] KOTO
    {
        23,
        "C-8",
        "KOTO",
        3,
        1,
        19.03125f,
        {
            {
            21024, 2, 4, 0, 4, 0,
            4, 8, 8, 6, 7,
            { 2, -1, { { 120, 81, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 127, 60, false }, { 74, 0, true }, { 113, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 119, 127, false }, { 42, 32, false }, { 34, 0, true }, { 46, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            45568, 5, 4, 0, 0, 0,
            5, 8, 3, 0, 0,
            { 1, -1, { { 112, 73, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 127, 127, false }, { 68, 0, true }, { 56, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 42, 50, true }, { 70, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [D-1] PIANO 1
    {
        24,
        "D-1",
        "PIANO 1",
        2,
        -1,
        19.03125f,
        {
            {
            16416, 2, 2, 0, 4, 0,
            7, 9, 6, 8, 0,
            { 1, 0, { { 110, 70, true }, { 2, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 1, { { 127, 127, false }, { 60, 101, true }, { 72, 3, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 110, 127, false }, { 30, 0, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            16384, 2, 2, 0, 0, 0,
            7, 9, 6, 8, 0,
            { 1, 0, { { 110, 70, true }, { 2, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 1, { { 127, 127, false }, { 60, 101, true }, { 72, 3, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 110, 127, false }, { 30, 0, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [D-2] PIANO 2
    {
        25,
        "D-2",
        "PIANO 2",
        3,
        0,
        21.0f,
        {
            {
            22016, 2, 5, 0, 0, 0,
            4, 4, 8, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 116, 105, false }, { 32, 83, true }, { 52, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 119, 127, false }, { 36, 108, false }, { 30, 0, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            22016, 2, 5, 0, 0, 0,
            6, 3, 7, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 127, 127, false }, { 104, 32, false }, { 84, 0, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 119, 51, false }, { 28, 38, false }, { 43, 0, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [D-3] PIANO 3
    {
        26,
        "D-3",
        "PIANO 3",
        3,
        0,
        21.0f,
        {
            {
            40960, 5, 5, 0, 0, 0,
            4, 7, 8, 4, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 127, 105, false }, { 40, 115, true }, { 52, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 119, 127, false }, { 36, 108, false }, { 30, 0, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            22016, 2, 5, 0, 0, 0,
            6, 3, 7, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 127, 127, false }, { 104, 32, false }, { 84, 0, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 119, 51, false }, { 28, 38, false }, { 43, 0, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [D-4] ELEC.PIANO
    {
        27,
        "D-4",
        "ELEC.PIANO",
        3,
        0,
        46.046875f,
        {
            {
            16384, 2, 2, 0, 0, 0,
            5, 4, 6, 15, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, -1, { { 127, 115, false }, { 65, 64, false }, { 30, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 112, 127, false }, { 25, 0, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            16384, 2, 2, 0, 0, 0,
            5, 9, 15, 15, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 127, 127, false }, { 106, 51, true }, { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 119, 108, false }, { 38, 56, false }, { 28, 0, true }, { 52, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [D-5] HONKY-TONK
    {
        28,
        "D-5",
        "HONKY-TONK",
        2,
        0,
        0.21875f,
        {
            {
            22016, 2, 5, 0, 0, 0,
            4, 6, 5, 5, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 127, 114, false }, { 15, 69, true }, { 52, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 119, 127, false }, { 36, 108, false }, { 30, 0, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            22016, 2, 5, 0, 0, 0,
            4, 6, 5, 5, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 127, 114, false }, { 15, 69, true }, { 52, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 119, 127, false }, { 36, 108, false }, { 30, 0, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [D-6] FUNKY CLAVI.1
    {
        29,
        "D-6",
        "FUNKY CLAVI.1",
        3,
        -1,
        43.0f,
        {
            {
            38432, 4, 5, 0, 4, 0,
            0, 5, 7, 15, 0,
            { 2, 0, { { 127, 70, true }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 104, 127, false }, { 64, 0, true }, { 94, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 36, 0, true }, { 72, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            32768, 4, 4, 0, 0, 0,
            0, 0, 5, 15, 15,
            { 1, -1, { { 127, 55, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 111, 127, false }, { 68, 0, true }, { 81, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 43, 77, true }, { 70, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [D-7] FUNKY CLAVI.2
    {
        30,
        "D-7",
        "FUNKY CLAVI.2",
        3,
        0,
        12.0f,
        {
            {
            21024, 2, 4, 0, 4, 0,
            1, 2, 4, 3, 0,
            { 1, -1, { { 127, 70, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 127, 127, false }, { 62, 57, false }, { 56, 38, true }, { 57, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 111, 118, false }, { 30, 108, false }, { 36, 0, true }, { 98, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            45568, 5, 4, 0, 0, 0,
            1, 2, 3, 4, 0,
            { 1, -1, { { 127, 76, false }, { 100, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 127, 73, false }, { 62, 57, false }, { 56, 38, true }, { 57, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 111, 118, false }, { 30, 108, false }, { 56, 0, true }, { 99, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [D-8] HARPSICHORD
    {
        31,
        "D-8",
        "HARPSICHORD",
        3,
        0,
        0.0f,
        {
            {
            38400, 4, 5, 0, 0, 0,
            1, 2, 0, 0, 0,
            { 1, 0, { { 127, 70, true }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 127, 115, true }, { 11, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 112, false }, { 40, 0, true }, { 74, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            34304, 4, 1, 0, 0, 0,
            3, 2, 0, 0, 0,
            { 1, -1, { { 127, 78, false }, { 115, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 127, 115, true }, { 8, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, -1, { { 119, 112, false }, { 92, 102, false }, { 37, 0, false }, { 74, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [E-1] JAZZ ORGAN 1
    {
        32,
        "E-1",
        "JAZZ ORGAN 1",
        3,
        0,
        19.09375f,
        {
            {
            35328, 4, 2, 0, 0, 0,
            9, 0, 0, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 0, -1, { { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, 0, { { 119, 127, true }, { 78, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            8192, 1, 1, 0, 0, 0,
            2, 9, 8, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, -1, { { 93, 16, false }, { 40, 0, false }, { 8, 29, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 119, 127, false }, { 46, 0, false }, { 119, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [E-2] JAZZ ORGAN 2
    {
        33,
        "E-2",
        "JAZZ ORGAN 2",
        3,
        0,
        24.0f,
        {
            {
            20992, 2, 4, 0, 0, 0,
            9, 9, 0, 0, 0,
            { 1, -1, { { 127, 57, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 127, 67, false }, { 95, 0, false }, { 8, 29, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, 0, { { 119, 122, true }, { 119, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            20992, 2, 4, 0, 0, 0,
            9, 9, 7, 0, 0,
            { 1, -1, { { 127, 57, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 127, 67, false }, { 95, 0, false }, { 8, 29, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, 0, { { 119, 107, true }, { 119, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [E-3] PIPE ORGAN 1
    {
        34,
        "E-3",
        "PIPE ORGAN 1",
        3,
        -1,
        24.0f,
        {
            {
            1536, 0, 1, 0, 0, 0,
            0, 5, 1, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 127, 100, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 0, { { 90, 127, true }, { 54, 103, false }, { 30, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            15040, 1, 6, 3, 0, 0,
            0, 5, 1, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 127, 32, true }, { 8, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 0, { { 90, 127, true }, { 70, 103, false }, { 40, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [E-4] PIPE ORGAN 2
    {
        35,
        "E-4",
        "PIPE ORGAN 2",
        2,
        -1,
        24.0f,
        {
            {
            8192, 1, 1, 0, 0, 0,
            4, 9, 0, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 127, 64, true }, { 40, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 0, { { 91, 127, true }, { 84, 101, false }, { 33, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            8192, 1, 1, 0, 0, 0,
            4, 9, 0, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 127, 64, true }, { 40, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 0, { { 91, 127, true }, { 84, 101, false }, { 33, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [E-5] ACCORDION
    {
        36,
        "E-5",
        "ACCORDION",
        3,
        1,
        0.171875f,
        {
            {
            10752, 1, 2, 0, 0, 0,
            0, 0, 5, 0, 0,
            { 2, -1, { { 83, 33, false }, { 92, 8, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 96, 112, false }, { 75, 96, false }, { 52, 66, true }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 92, 99, false }, { 66, 105, false }, { 15, 90, true }, { 74, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            10752, 1, 2, 0, 0, 0,
            0, 0, 3, 0, 0,
            { 2, -1, { { 84, 52, false }, { 87, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 127, 112, false }, { 107, 123, false }, { 10, 66, true }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 87, 114, false }, { 24, 96, true }, { 74, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [E-6] VOICE 1
    {
        37,
        "E-6",
        "VOICE 1",
        3,
        -1,
        12.140625f,
        {
            {
            0, 0, 0, 0, 0, 0,
            0, 0, 2, 7, 0,
            { 2, 1, { { 127, 69, false }, { 51, 70, true }, { 0, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 0, -1, { { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 1, { { 70, 127, false }, { 25, 102, true }, { 37, 103, false }, { 72, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            0, 0, 0, 0, 0, 0,
            0, 4, 2, 7, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 68, 114, false }, { 33, 48, true }, { 72, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 80, 118, false }, { 25, 105, false }, { 14, 118, true }, { 42, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [E-7] VOICE 2
    {
        38,
        "E-7",
        "VOICE 2",
        2,
        -1,
        0.125f,
        {
            {
            8224, 1, 1, 0, 4, 0,
            0, 0, 2, 0, 0,
            { 2, 1, { { 127, 69, false }, { 51, 70, true }, { 0, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 0, -1, { { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 82, 116, false }, { 27, 127, false }, { 37, 105, true }, { 62, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            8192, 1, 1, 0, 0, 0,
            0, 0, 2, 0, 0,
            { 2, 1, { { 127, 69, false }, { 51, 70, true }, { 0, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 0, -1, { { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 82, 116, false }, { 27, 127, false }, { 37, 105, true }, { 62, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [E-8] VOICE 3
    {
        39,
        "E-8",
        "VOICE 3",
        3,
        -1,
        -0.03125f,
        {
            {
            49344, 6, 6, 3, 0, 0,
            7, 5, 4, 3, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 86, 55, true }, { 8, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 0, { { 102, 118, true }, { 96, 79, false }, { 37, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            49216, 6, 6, 1, 0, 0,
            6, 7, 2, 3, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 86, 61, true }, { 8, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 0, { { 102, 118, true }, { 96, 59, false }, { 34, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [F-1] MUSIC BOX
    {
        40,
        "F-1",
        "MUSIC BOX",
        3,
        -1,
        39.484375f,
        {
            {
            32800, 4, 4, 0, 4, 0,
            0, 0, 6, 0, 0,
            { 1, -1, { { 115, 73, false }, { 123, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 0, -1, { { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 127, false }, { 36, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            32768, 4, 4, 0, 0, 0,
            3, 0, 7, 10, 3,
            { 1, -1, { { 112, 71, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 74, false }, { 14, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 127, false }, { 27, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [F-2] VIBRAPHONE
    {
        41,
        "F-2",
        "VIBRAPHONE",
        3,
        0,
        24.0f,
        {
            {
            16384, 2, 2, 0, 0, 0,
            5, 3, 7, 11, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 71, false }, { 88, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 127, false }, { 36, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            16384, 2, 2, 0, 0, 0,
            2, 0, 5, 4, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 12, false }, { 82, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 105, false }, { 36, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [F-3] XYLOPHONE
    {
        42,
        "F-3",
        "XYLOPHONE",
        3,
        1,
        28.0f,
        {
            {
            42528, 5, 1, 0, 4, 0,
            2, 1, 3, 1, 2,
            { 1, -1, { { 127, 20, false }, { 111, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, -1, { { 127, 23, false }, { 124, 0, false }, { 127, 15, false }, { 114, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 114, 127, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            0, 0, 0, 0, 0, 0,
            2, 1, 3, 5, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 60, false }, { 86, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 114, 127, false }, { 63, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [F-4] MARIMBA
    {
        43,
        "F-4",
        "MARIMBA",
        2,
        0,
        19.0f,
        {
            {
            8224, 1, 1, 0, 4, 0,
            5, 2, 4, 15, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 39, false }, { 98, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 119, 127, false }, { 51, 113, false }, { 46, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            8192, 1, 1, 0, 0, 0,
            5, 2, 4, 15, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 39, false }, { 98, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 119, 127, false }, { 51, 113, false }, { 46, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [F-5] MALLET LOG
    {
        44,
        "F-5",
        "MALLET LOG",
        3,
        -1,
        -0.078125f,
        {
            {
            8192, 1, 1, 0, 0, 0,
            7, 7, 5, 0, 2,
            { 1, -1, { { 127, 60, false }, { 120, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 127, false }, { 78, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 127, false }, { 48, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            8192, 1, 1, 0, 0, 0,
            0, 7, 5, 0, 4,
            { 1, -1, { { 127, 47, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 127, false }, { 81, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 127, false }, { 52, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [F-6] AFRO-PERCUSSION
    {
        45,
        "F-6",
        "AFRO-PERCUSSION",
        3,
        1,
        22.390625f,
        {
            {
            5664, 0, 5, 0, 4, 0,
            4, 6, 7, 9, 0,
            { 1, -1, { { 127, 82, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 127, false }, { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 127, false }, { 48, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            40960, 5, 5, 0, 0, 0,
            0, 0, 5, 7, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 127, false }, { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 127, false }, { 66, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [F-7] BELLS
    {
        46,
        "F-7",
        "BELLS",
        3,
        0,
        16.109375f,
        {
            {
            5664, 0, 5, 0, 4, 0,
            4, 0, 4, 0, 0,
            { 1, -1, { { 114, 72, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 0, -1, { { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 127, false }, { 21, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            33280, 4, 0, 0, 0, 0,
            7, 0, 4, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 0, -1, { { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 127, false }, { 10, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [F-8] METALLIC SOUND
    {
        47,
        "F-8",
        "METALLIC SOUND",
        3,
        0,
        45.59375f,
        {
            {
            8224, 1, 1, 0, 4, 0,
            4, 2, 7, 15, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, -1, { { 127, 127, false }, { 117, 82, false }, { 24, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 119, 127, false }, { 34, 109, false }, { 38, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            20992, 2, 4, 0, 0, 0,
            4, 6, 7, 14, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, -1, { { 127, 127, false }, { 116, 53, false }, { 54, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 119, 127, false }, { 42, 83, false }, { 34, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [G-1] SYNTH.STRINGS
    {
        48,
        "G-1",
        "SYNTH.STRINGS",
        2,
        0,
        0.078125f,
        {
            {
            40960, 5, 5, 0, 0, 0,
            0, 0, 6, 1, 0,
            { 1, -1, { { 115, 74, false }, { 115, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 116, 127, false }, { 53, 102, false }, { 52, 84, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 105, 127, false }, { 20, 118, false }, { 60, 113, true }, { 54, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            40960, 5, 5, 0, 0, 0,
            0, 0, 6, 1, 0,
            { 1, -1, { { 115, 74, false }, { 115, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 116, 127, false }, { 53, 102, false }, { 52, 84, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 105, 127, false }, { 20, 118, false }, { 60, 113, true }, { 54, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [G-2] FAT ENSEMBLE
    {
        49,
        "G-2",
        "FAT ENSEMBLE",
        2,
        0,
        0.09375f,
        {
            {
            22016, 2, 5, 0, 0, 0,
            0, 2, 6, 3, 7,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 4, 2, { { 104, 127, false }, { 64, 123, false }, { 52, 66, true }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 4, 2, { { 119, 78, false }, { 92, 127, false }, { 80, 119, true }, { 94, 87, false }, { 39, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            22016, 2, 5, 0, 0, 0,
            0, 2, 6, 3, 7,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 4, 2, { { 104, 127, false }, { 64, 123, false }, { 52, 66, true }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 4, 2, { { 119, 78, false }, { 92, 127, false }, { 80, 119, true }, { 94, 87, false }, { 39, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [G-3] SITAR
    {
        50,
        "G-3",
        "SITAR",
        2,
        0,
        12.109375f,
        {
            {
            23232, 2, 6, 3, 0, 0,
            2, 2, 2, 0, 7,
            { 2, -1, { { 3, 1, false }, { 57, 20, false }, { 51, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, -1, { { 127, 127, false }, { 124, 38, false }, { 50, 127, false }, { 23, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, -1, { { 119, 114, false }, { 36, 95, false }, { 0, 0, false }, { 38, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            23232, 2, 6, 3, 0, 0,
            2, 2, 2, 0, 7,
            { 2, -1, { { 3, 1, false }, { 57, 20, false }, { 51, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, -1, { { 127, 127, false }, { 124, 38, false }, { 50, 127, false }, { 23, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, -1, { { 119, 114, false }, { 36, 95, false }, { 0, 0, false }, { 38, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [G-4] SYNTH.LEAD 1
    {
        51,
        "G-4",
        "SYNTH.LEAD 1",
        3,
        1,
        24.0f,
        {
            {
            0, 0, 0, 0, 0, 0,
            0, 0, 5, 0, 0,
            { 1, -1, { { 118, 57, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 117, 85, false }, { 46, 0, true }, { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 108, 124, false }, { 40, 0, true }, { 66, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            38400, 4, 5, 0, 0, 0,
            0, 0, 5, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 73, false }, { 63, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 108, 127, false }, { 54, 0, true }, { 72, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [G-5] SYNTH.LEAD 2
    {
        52,
        "G-5",
        "SYNTH.LEAD 2",
        2,
        -1,
        12.03125f,
        {
            {
            8224, 1, 1, 0, 4, 0,
            0, 0, 5, 1, 0,
            { 1, 0, { { 110, 70, true }, { 2, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 1, { { 127, 127, false }, { 63, 43, true }, { 116, 3, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 110, 127, false }, { 27, 78, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            8192, 1, 1, 0, 0, 0,
            0, 0, 5, 1, 0,
            { 1, 0, { { 110, 70, true }, { 2, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 1, { { 127, 127, false }, { 63, 43, true }, { 116, 3, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 110, 127, false }, { 27, 78, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [G-6] SYNTH.LEAD 3
    {
        53,
        "G-6",
        "SYNTH.LEAD 3",
        3,
        0,
        0.046875f,
        {
            {
            21024, 2, 4, 0, 4, 0,
            0, 0, 8, 8, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, -1, { { 127, 127, false }, { 45, 112, false }, { 56, 0, false }, { 81, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 1, { { 119, 127, false }, { 40, 123, true }, { 57, 0, false }, { 68, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            35328, 4, 2, 0, 0, 0,
            0, 0, 12, 15, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, -1, { { 118, 127, false }, { 50, 102, false }, { 58, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 4, 3, { { 119, 127, false }, { 42, 113, false }, { 69, 101, false }, { 37, 127, true }, { 66, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [G-7] SYNTH.LEAD 4
    {
        54,
        "G-7",
        "SYNTH.LEAD 4",
        3,
        0,
        45.359375f,
        {
            {
            8192, 1, 1, 0, 0, 0,
            0, 0, 9, 7, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 127, 127, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 1, { { 119, 127, false }, { 66, 118, true }, { 73, 72, false }, { 30, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            20992, 2, 4, 0, 0, 0,
            0, 0, 6, 15, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 127, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 112, 83, false }, { 69, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [G-8] SWEEP SOUND 1
    {
        55,
        "G-8",
        "SWEEP SOUND 1",
        3,
        0,
        24.0625f,
        {
            {
            16416, 2, 2, 0, 4, 0,
            4, 2, 4, 3, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 62, 106, false }, { 60, 55, false }, { 30, 62, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 2, { { 119, 127, false }, { 9, 118, false }, { 22, 113, true }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            32768, 4, 4, 0, 0, 0,
            4, 4, 9, 6, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, 1, { { 127, 103, false }, { 74, 43, true }, { 52, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 119, 127, false }, { 52, 118, false }, { 58, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [H-1] SYNTH.DRUMS 1
    {
        56,
        "H-1",
        "SYNTH.DRUMS 1",
        3,
        0,
        10.0f,
        {
            {
            41496, 5, 0, 0, 3, 0,
            0, 0, 8, 0, 0,
            { 1, -1, { { 118, 97, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, -1, { { 127, 127, false }, { 127, 110, false }, { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 49, 0, true }, { 62, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            38400, 4, 5, 0, 0, 0,
            1, 8, 8, 8, 0,
            { 2, 1, { { 127, 103, false }, { 127, 53, true }, { 16, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 127, 127, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 119, 109, false }, { 63, 63, false }, { 62, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [H-2] SYNTH.DRUMS 2
    {
        57,
        "H-2",
        "SYNTH.DRUMS 2",
        3,
        -1,
        10.0f,
        {
            {
            41496, 5, 0, 0, 3, 0,
            0, 0, 2, 0, 10,
            { 1, -1, { { 118, 81, false }, { 127, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 2, -1, { { 127, 127, false }, { 127, 110, false }, { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 49, 0, true }, { 62, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            32768, 4, 4, 0, 0, 0,
            1, 8, 15, 6, 9,
            { 2, 1, { { 127, 57, false }, { 127, 53, true }, { 16, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 127, 127, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 119, 109, false }, { 63, 63, false }, { 62, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [H-3] CONGA
    {
        58,
        "H-3",
        "CONGA",
        3,
        0,
        -8.09375f,
        {
            {
            5664, 0, 5, 0, 4, 0,
            3, 0, 3, 0, 3,
            { 2, -1, { { 42, 3, false }, { 69, 6, false }, { 76, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 127, false }, { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 127, false }, { 55, 0, true }, { 52, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            50752, 6, 1, 1, 0, 0,
            3, 0, 3, 4, 2,
            { 1, -1, { { 127, 69, false }, { 80, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 127, false }, { 111, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 119, 117, false }, { 57, 0, true }, { 42, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [H-4] STEEL DRUM
    {
        59,
        "H-4",
        "STEEL DRUM",
        3,
        0,
        0.296875f,
        {
            {
            8224, 1, 1, 0, 4, 0,
            4, 1, 3, 2, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 127, 19, false }, { 22, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 119, 127, false }, { 46, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            8192, 1, 1, 0, 0, 0,
            2, 1, 0, 8, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, -1, { { 125, 94, false }, { 53, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 1, -1, { { 106, 127, false }, { 27, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [H-5] SWEEP SOUND 2
    {
        60,
        "H-5",
        "SWEEP SOUND 2",
        2,
        0,
        0.109375f,
        {
            {
            49728, 6, 0, 1, 0, 0,
            0, 0, 0, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 44, 79, false }, { 35, 52, false }, { 36, 38, true }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 1, { { 69, 127, false }, { 33, 119, true }, { 94, 87, false }, { 48, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            49728, 6, 0, 1, 0, 0,
            0, 0, 0, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 3, 2, { { 44, 79, false }, { 35, 52, false }, { 36, 38, true }, { 39, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, 1, { { 69, 127, false }, { 33, 119, true }, { 94, 87, false }, { 48, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [H-6] JET ROAR
    {
        61,
        "H-6",
        "JET ROAR",
        3,
        -1,
        17.78125f,
        {
            {
            38424, 4, 5, 0, 3, 0,
            0, 1, 4, 0, 0,
            { 1, 0, { { 38, 103, true }, { 48, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 90, 109, true }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, -1, { { 97, 121, false }, { 18, 0, false }, { 42, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            38400, 4, 5, 0, 0, 0,
            0, 0, 4, 0, 0,
            { 1, 0, { { 25, 103, true }, { 16, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 87, 75, true }, { 35, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 3, -1, { { 97, 89, false }, { 7, 112, false }, { 18, 0, false }, { 42, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [H-7] MOTORCYCLE
    {
        62,
        "H-7",
        "MOTORCYCLE",
        3,
        -1,
        -12.0f,
        {
            {
            12800, 1, 4, 0, 0, 0,
            2, 9, 6, 0, 0,
            { 4, 3, { { 32, 74, false }, { 51, 20, false }, { 44, 83, false }, { 51, 74, true }, { 3, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 99, 62, true }, { 46, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 84, 97, false }, { 74, 122, true }, { 36, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            12800, 1, 4, 0, 0, 0,
            2, 0, 6, 0, 0,
            { 4, 3, { { 32, 74, false }, { 51, 20, false }, { 44, 83, false }, { 51, 74, true }, { 3, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 1, 0, { { 99, 107, true }, { 46, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 2, 1, { { 84, 97, false }, { 74, 127, true }, { 36, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        }
        }
    },
    // [H-8] TYPHOON SOUND
    {
        63,
        "H-8",
        "TYPHOON SOUND",
        3,
        -1,
        -12.0f,
        {
            {
            24, 0, 0, 0, 3, 0,
            0, 0, 2, 0, 0,
            { 0, -1, { { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false }, { 64, 0, false } } },
            { 0, -1, { { 127, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false }, { 68, 0, false } } },
            { 0, -1, { { 119, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false }, { 60, 0, false } } }
        },
            {
            1536, 0, 1, 0, 0, 0,
            8, 9, 2, 0, 0,
            { 7, -1, { { 127, 33, false }, { 87, 0, false }, { 23, 70, false }, { 43, 0, false }, { 51, 69, false }, { 57, 0, false }, { 44, 68, false }, { 64, 0, false } } },
            { 7, 6, { { 50, 56, false }, { 64, 23, false }, { 52, 37, false }, { 39, 57, false }, { 32, 14, false }, { 68, 66, false }, { 35, 47, true }, { 68, 0, false } } },
            { 7, 6, { { 86, 92, false }, { 44, 127, false }, { 36, 87, false }, { 39, 121, false }, { 32, 105, false }, { 40, 127, false }, { 42, 117, true }, { 26, 0, false } } }
        }
        }
    }
};
