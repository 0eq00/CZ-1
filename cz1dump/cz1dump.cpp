/**
 * @file cz1dump.cpp
 * @brief Casio CZ-1 / CZ Series SysEx Preset Dump CUI Tool
 *
 * Build for Windows (MSYS2 / MinGW-w64) standalone EXE (no DLLs required):
 *   g++ -O2 -std=c++17 -Wall -static -static-libgcc -static-libstdc++ cz1dump.cpp -o cz1dump.exe
 *
 * Usage:
 *   cz1dump <syx_file> [offset]
 *   cz1dump <syx_file> -p <patch_number (1-based)>
 *   cz1dump <syx_file> --list
 *
 * Examples:
 *   .\cz1dump.exe ..\SYX\CZ1ORGB1.SYX
 *   .\cz1dump.exe cz1-factory.syx 0
 *   .\cz1dump.exe cz1-factory.syx 296
 *   .\cz1dump.exe cz1-factory.syx 0x0128
 *   .\cz1dump.exe cz1-factory.syx -p 2
 *   .\cz1dump.exe cz1-factory.syx --list
 */

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>

// --- Lookup Constants ---

static const char* WAVEFORMS[9] = {
    "None",
    "1: Sawtooth",
    "2: Square",
    "3: Pulse",
    "4: Double Sine",
    "5: Saw-Pulse",
    "6: Resonance 1 (Saw)",
    "7: Resonance 2 (Tri)",
    "8: Resonance 3 (Trap)"
};

static const char* WINDOWS[8] = {
    "None", "Saw", "Triangle", "Trapezoid", "Pulse", "2x Saw", "2x Saw", "2x Saw"
};

static const char* LINE_SELECTS[4] = {
    "Line 1 only (16 Poly)",
    "Line 2 only (16 Poly)",
    "Line 1 + 1' (Double Line 1, 8 Poly)",
    "Line 1 + 2' (Dual Detuned, 8 Poly)"
};

static const char* VIBRATO_WAVES[4] = {
    "Triangle",
    "Saw Up",
    "Saw Down",
    "Square"
};

// 100-step Vibrato lookup tables matching Casio CZ hardware firmware
static const uint8_t VIB_DELAY_TABLE[100][3] = {
    {0x00, 0x00, 0x00}, {0x01, 0x00, 0x01}, {0x02, 0x00, 0x02}, {0x03, 0x00, 0x03},
    {0x04, 0x00, 0x04}, {0x05, 0x00, 0x05}, {0x06, 0x00, 0x06}, {0x07, 0x00, 0x07},
    {0x08, 0x00, 0x08}, {0x09, 0x00, 0x09}, {0x0A, 0x00, 0x0A}, {0x0B, 0x00, 0x0B},
    {0x0C, 0x00, 0x0C}, {0x0D, 0x00, 0x0D}, {0x0E, 0x00, 0x0E}, {0x0F, 0x00, 0x0F},
    {0x10, 0x00, 0x10}, {0x11, 0x00, 0x11}, {0x12, 0x00, 0x12}, {0x13, 0x00, 0x13},
    {0x14, 0x00, 0x14}, {0x15, 0x00, 0x15}, {0x16, 0x00, 0x16}, {0x17, 0x00, 0x17},
    {0x18, 0x00, 0x18}, {0x19, 0x00, 0x19}, {0x1A, 0x00, 0x1A}, {0x1B, 0x00, 0x1B},
    {0x1C, 0x00, 0x1C}, {0x1D, 0x00, 0x1D}, {0x1E, 0x00, 0x1E}, {0x1F, 0x00, 0x1F},
    {0x20, 0x00, 0x21}, {0x21, 0x00, 0x23}, {0x22, 0x00, 0x25}, {0x23, 0x00, 0x27},
    {0x24, 0x00, 0x29}, {0x25, 0x00, 0x2B}, {0x26, 0x00, 0x2D}, {0x27, 0x00, 0x2F},
    {0x28, 0x00, 0x31}, {0x29, 0x00, 0x33}, {0x2A, 0x00, 0x35}, {0x2B, 0x00, 0x37},
    {0x2C, 0x00, 0x39}, {0x2D, 0x00, 0x3B}, {0x2E, 0x00, 0x3D}, {0x2F, 0x00, 0x3F},
    {0x30, 0x00, 0x43}, {0x31, 0x00, 0x47}, {0x32, 0x00, 0x4B}, {0x33, 0x00, 0x4F},
    {0x34, 0x00, 0x53}, {0x35, 0x00, 0x57}, {0x36, 0x00, 0x5B}, {0x37, 0x00, 0x5F},
    {0x38, 0x00, 0x63}, {0x39, 0x00, 0x67}, {0x3A, 0x00, 0x6B}, {0x3B, 0x00, 0x6F},
    {0x3C, 0x00, 0x73}, {0x3D, 0x00, 0x77}, {0x3E, 0x00, 0x7B}, {0x3F, 0x00, 0x7F},
    {0x40, 0x00, 0x87}, {0x41, 0x00, 0x8F}, {0x42, 0x00, 0x97}, {0x43, 0x00, 0x9F},
    {0x44, 0x00, 0xA7}, {0x45, 0x00, 0xAF}, {0x46, 0x00, 0xB7}, {0x47, 0x00, 0xBF},
    {0x48, 0x00, 0xC7}, {0x49, 0x00, 0xCF}, {0x4A, 0x00, 0xD7}, {0x4B, 0x00, 0xDF},
    {0x4C, 0x00, 0xE7}, {0x4D, 0x00, 0xEF}, {0x4E, 0x00, 0xF7}, {0x4F, 0x00, 0xFF},
    {0x50, 0x01, 0x0F}, {0x51, 0x01, 0x1F}, {0x52, 0x01, 0x2F}, {0x53, 0x01, 0x3F},
    {0x54, 0x01, 0x4F}, {0x55, 0x01, 0x5F}, {0x56, 0x01, 0x6F}, {0x67, 0x01, 0x7F},
    {0x58, 0x01, 0x8F}, {0x59, 0x01, 0x9F}, {0x5A, 0x01, 0xAF}, {0x5B, 0x01, 0xBF},
    {0x5C, 0x01, 0xCF}, {0x5D, 0x01, 0xDF}, {0x5E, 0x01, 0xEF}, {0x5F, 0x01, 0xFF},
    {0x60, 0x02, 0x1F}, {0x61, 0x02, 0x3F}, {0x62, 0x02, 0x5F}, {0x63, 0x02, 0x7F}
};

static const uint8_t VIB_RATE_TABLE[100][3] = {
    {0x00, 0x00, 0x20}, {0x01, 0x00, 0x40}, {0x02, 0x00, 0x60}, {0x03, 0x00, 0x80},
    {0x04, 0x00, 0xA0}, {0x05, 0x00, 0xC0}, {0x06, 0x00, 0xE0}, {0x07, 0x00, 0x00},
    {0x08, 0x01, 0x20}, {0x09, 0x01, 0x40}, {0x0A, 0x01, 0x60}, {0x0B, 0x01, 0x80},
    {0x0C, 0x01, 0xA0}, {0x0D, 0x01, 0xC0}, {0x0E, 0x01, 0xE0}, {0x0F, 0x02, 0x00},
    {0x10, 0x02, 0x20}, {0x11, 0x02, 0x40}, {0x12, 0x02, 0x60}, {0x13, 0x02, 0x80},
    {0x14, 0x02, 0xA0}, {0x15, 0x02, 0xC0}, {0x16, 0x03, 0xE0}, {0x17, 0x03, 0x00},
    {0x18, 0x03, 0x20}, {0x19, 0x03, 0x40}, {0x1A, 0x03, 0x60}, {0x1B, 0x03, 0x80},
    {0x1C, 0x03, 0xA0}, {0x1D, 0x03, 0xC0}, {0x1E, 0x03, 0xE0}, {0x1F, 0x04, 0x00},
    {0x20, 0x04, 0x60}, {0x21, 0x04, 0xA0}, {0x22, 0x04, 0xE0}, {0x23, 0x05, 0x20},
    {0x24, 0x05, 0x60}, {0x25, 0x05, 0xA0}, {0x26, 0x05, 0xE0}, {0x27, 0x06, 0x20},
    {0x28, 0x06, 0x60}, {0x29, 0x06, 0xA0}, {0x2A, 0x06, 0xE0}, {0x2B, 0x07, 0x20},
    {0x2C, 0x07, 0x60}, {0x2D, 0x07, 0xA0}, {0x2E, 0x07, 0xE0}, {0x2F, 0x08, 0x20},
    {0x30, 0x08, 0xE0}, {0x31, 0x09, 0x60}, {0x32, 0x09, 0xE0}, {0x33, 0x0A, 0x60},
    {0x34, 0x0A, 0xE0}, {0x35, 0x0B, 0x60}, {0x36, 0x0B, 0xE0}, {0x37, 0x0C, 0x60},
    {0x38, 0x0C, 0xE0}, {0x39, 0x0D, 0x60}, {0x3A, 0x0D, 0xE0}, {0x3B, 0x0E, 0x60},
    {0x3C, 0x0E, 0xE0}, {0x3D, 0x0F, 0x60}, {0x3E, 0x0F, 0xE0}, {0x3F, 0x10, 0x60},
    {0x40, 0x11, 0xE0}, {0x41, 0x12, 0xE0}, {0x42, 0x13, 0xE0}, {0x43, 0x14, 0xE0},
    {0x44, 0x15, 0xE0}, {0x45, 0x16, 0xE0}, {0x46, 0x17, 0xE0}, {0x47, 0x18, 0xE0},
    {0x48, 0x19, 0xE0}, {0x49, 0x1A, 0xE0}, {0x4A, 0x1B, 0xE0}, {0x4B, 0x1C, 0xE0},
    {0x4C, 0x1D, 0xE0}, {0x4D, 0x1E, 0xE0}, {0x4E, 0x1F, 0xE0}, {0x4F, 0x20, 0xE0},
    {0x50, 0x23, 0xE0}, {0x51, 0x25, 0xE0}, {0x52, 0x27, 0xE0}, {0x53, 0x29, 0xE0},
    {0x54, 0x2B, 0xE0}, {0x55, 0x2D, 0xE0}, {0x56, 0x2F, 0xE0}, {0x57, 0x31, 0xE0},
    {0x58, 0x33, 0xE0}, {0x59, 0x35, 0xE0}, {0x5A, 0x37, 0xE0}, {0x5B, 0x39, 0xE0},
    {0x5C, 0x3B, 0xE0}, {0x5D, 0x3D, 0xE0}, {0x5E, 0x3F, 0xE0}, {0x5F, 0x41, 0xE0},
    {0x60, 0x47, 0xE0}, {0x61, 0x4B, 0xE0}, {0x62, 0x4F, 0xE0}, {0x63, 0x53, 0xE0}
};

static const uint8_t VIB_DEPTH_TABLE[100][3] = {
    {0x00, 0x00, 0x01}, {0x01, 0x00, 0x02}, {0x02, 0x00, 0x03}, {0x03, 0x00, 0x04},
    {0x04, 0x00, 0x05}, {0x05, 0x00, 0x06}, {0x06, 0x00, 0x07}, {0x07, 0x00, 0x08},
    {0x08, 0x00, 0x09}, {0x09, 0x00, 0x0A}, {0x0A, 0x00, 0x0B}, {0x0B, 0x00, 0x0C},
    {0x0C, 0x00, 0x0D}, {0x0D, 0x00, 0x0E}, {0x0E, 0x00, 0x0F}, {0x0F, 0x00, 0x10},
    {0x10, 0x00, 0x11}, {0x11, 0x00, 0x12}, {0x12, 0x00, 0x13}, {0x13, 0x00, 0x14},
    {0x14, 0x00, 0x15}, {0x15, 0x00, 0x16}, {0x16, 0x00, 0x17}, {0x17, 0x00, 0x18},
    {0x18, 0x00, 0x19}, {0x19, 0x00, 0x1A}, {0x1A, 0x00, 0x1B}, {0x1B, 0x00, 0x1C},
    {0x1C, 0x00, 0x1D}, {0x1D, 0x00, 0x1E}, {0x1E, 0x00, 0x1F}, {0x1F, 0x00, 0x20},
    {0x20, 0x00, 0x23}, {0x21, 0x00, 0x25}, {0x22, 0x00, 0x27}, {0x23, 0x00, 0x29},
    {0x24, 0x00, 0x2B}, {0x25, 0x00, 0x2D}, {0x26, 0x00, 0x2F}, {0x27, 0x00, 0x31},
    {0x28, 0x00, 0x33}, {0x29, 0x00, 0x35}, {0x2A, 0x00, 0x37}, {0x2B, 0x00, 0x39},
    {0x2C, 0x00, 0x3B}, {0x2D, 0x00, 0x3D}, {0x2E, 0x00, 0x3F}, {0x2F, 0x00, 0x41},
    {0x30, 0x00, 0x47}, {0x31, 0x00, 0x4B}, {0x32, 0x00, 0x4F}, {0x33, 0x00, 0x53},
    {0x34, 0x00, 0x57}, {0x35, 0x00, 0x5B}, {0x36, 0x00, 0x5F}, {0x37, 0x00, 0x63},
    {0x38, 0x00, 0x67}, {0x39, 0x00, 0x6B}, {0x3A, 0x00, 0x6F}, {0x3B, 0x00, 0x73},
    {0x3C, 0x00, 0x77}, {0x3D, 0x00, 0x7B}, {0x3E, 0x00, 0x7F}, {0x3F, 0x00, 0x83},
    {0x40, 0x00, 0x8F}, {0x41, 0x00, 0x97}, {0x42, 0x00, 0x9F}, {0x43, 0x00, 0xA7},
    {0x44, 0x00, 0xAF}, {0x45, 0x00, 0xB7}, {0x46, 0x00, 0xBF}, {0x47, 0x00, 0xC7},
    {0x48, 0x00, 0xCF}, {0x49, 0x00, 0xD7}, {0x4A, 0x00, 0xDF}, {0x4B, 0x00, 0xE7},
    {0x4C, 0x00, 0xEF}, {0x4D, 0x00, 0xF7}, {0x4E, 0x00, 0xFF}, {0x4F, 0x01, 0x07},
    {0x50, 0x01, 0x1F}, {0x51, 0x01, 0x2F}, {0x52, 0x01, 0x3F}, {0x53, 0x01, 0x4F},
    {0x54, 0x01, 0x5F}, {0x55, 0x01, 0x6F}, {0x56, 0x01, 0x7F}, {0x57, 0x01, 0x8F},
    {0x58, 0x01, 0x9F}, {0x59, 0x01, 0xAF}, {0x5A, 0x01, 0xBF}, {0x5B, 0x01, 0xCF},
    {0x5C, 0x01, 0xDF}, {0x5D, 0x01, 0xEF}, {0x5E, 0x01, 0xFF}, {0x5F, 0x02, 0x0F},
    {0x60, 0x02, 0x3F}, {0x61, 0x02, 0x5F}, {0x62, 0x02, 0x7F}, {0x63, 0x03, 0x00}
};

// --- Conversion Helpers ---

static inline uint8_t unnybblize(uint8_t lsb, uint8_t msb) {
    return ((lsb & 0x0F) | ((msb & 0x0F) << 4));
}

static inline int sysexToDCARate(uint8_t b) {
    if (b == 0) return 0;
    if (b >= 119) return 99;
    return std::min(99, (int)(b * 99 / 119) + 1);
}

static inline int sysexToDCALevel(uint8_t b) {
    if (b == 0) return 0;
    return std::max(0, std::min(99, (int)b - 28));
}

static inline int sysexToDCWRate(uint8_t b) {
    if (b <= 8) return 0;
    if (b >= 127) return 99;
    return std::min(99, (int)((b - 8) * 99 / 119) + 1);
}

static inline int sysexToDCWLevel(uint8_t b) {
    if (b == 0) return 0;
    return std::min(99, (int)(b * 99 / 127) + 1);
}

static inline int sysexToDCORate(uint8_t b) {
    if (b == 0) return 0;
    if (b >= 127) return 99;
    return std::min(99, (int)(b * 99 / 127) + 1);
}

static inline int sysexToDCOLevel(uint8_t b) {
    if (b > 63) return std::min(99, (int)b - 4);
    return std::min(99, (int)b);
}

static int decodeVibrato(const uint8_t table[100][3], uint8_t high, uint8_t med, uint8_t low) {
    for (int i = 0; i < 100; ++i) {
        if (table[i][0] == low && table[i][1] == med && table[i][2] == high) {
            return i;
        }
    }
    return std::max(0, std::min(99, (int)(high * 32 + med * 8 + (low >> 2))));
}

static std::string formatHex(uint64_t val, int width = 8) {
    std::ostringstream ss;
    ss << "0x" << std::hex << std::setw(width) << std::setfill('0') << val;
    return ss.str();
}

static std::string trimString(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// --- Data Structures ---

struct Step {
    int rate = 0;
    int level = 0;
    bool isSustain = false;
    bool isEnd = false;
};

struct Envelope {
    Step steps[8];
    int sustainStep = 0; // 0: None, 1..8
    int endStep = 1;     // 1..8
    int velocity = 0;    // 0..15
    int keyFollow = 0;   // 0..9
};

struct Line {
    int lineNum = 1;
    bool mute = false;
    bool ringMod = false;
    bool noiseMod = false;
    int wave1 = 1;
    int wave2 = 1;
    bool wave2On = false;
    int window = 0;
    int mainLevel = 15;
    Envelope dco;
    Envelope dcw;
    Envelope dca;
};

struct Patch {
    std::string name;
    int lineSelect = 0;
    int octave = 0;
    char detuneSign = '+';
    int detuneOctave = 0;
    int detuneNote = 0;
    int detuneFine = 0;
    int totalCents = 0;
    int vibratoWave = 0;
    int vibratoDelay = 0;
    int vibratoRate = 0;
    int vibratoDepth = 0;
    Line line1;
    Line line2;
    bool isCZ1 = true;
    uint8_t destAddr = 0x60;
    uint8_t channel = 1;
    size_t syxOffset = 0;
    size_t syxLength = 0;
};

static void parseLine(const std::vector<uint8_t>& b, size_t offset, Line& line) {
    uint8_t mfwHigh = b[offset];
    uint8_t mfwLow = b[offset + 1];
    uint16_t mfwVal = ((uint16_t)mfwHigh << 8) | mfwLow;

    // Modulation bits in MFW second byte (mfwLow):
    // Bit 5 (0x20): Ring Modulation enable
    // Bit 4 & 3 (0x18): Noise Modulation enable (pitch modulation from noise)
    line.ringMod = (line.lineNum == 1) ? (((mfwLow >> 5) & 1) == 1) : false;
    line.noiseMod = (line.lineNum == 1) ? ((mfwLow & 0x18) == 0x18) : false;
    line.mute = (line.lineNum == 1) ? (((mfwLow >> 2) & 1) == 1) : false;

    line.wave1 = ((mfwVal >> 13) & 7) + 1;
    line.wave2 = ((mfwVal >> 10) & 7) + 1;
    line.wave2On = ((mfwVal >> 9) & 1) == 1;
    line.window = (mfwVal >> 6) & 7;

    // DCA Key Follow & Main Level
    uint8_t mamdLow = b[offset + 2];
    line.dca.keyFollow = mamdLow & 0x0F;
    line.mainLevel = 15 - (mamdLow >> 4);

    // DCW Key Follow
    uint8_t mwmdLow = b[offset + 4];
    line.dcw.keyFollow = mwmdLow & 0x0F;

    // DCA Envelope
    uint8_t pmal = b[offset + 6];
    line.dca.velocity = pmal >> 4;
    line.dca.endStep = std::max(1, std::min(8, (pmal & 0x07) + 1));
    for (int s = 0; s < 8; ++s) {
        uint8_t r = b[offset + 7 + s * 2];
        uint8_t l = b[offset + 7 + s * 2 + 1];
        line.dca.steps[s].rate = sysexToDCARate(r & 127);
        line.dca.steps[s].level = sysexToDCALevel(l & 127);
        line.dca.steps[s].isSustain = (l >> 7) == 1;
        if (line.dca.steps[s].isSustain) line.dca.sustainStep = s + 1;
        line.dca.steps[s].isEnd = (s + 1 == line.dca.endStep);
    }

    // DCW Envelope
    uint8_t pmwl = b[offset + 23];
    line.dcw.velocity = pmwl >> 4;
    line.dcw.endStep = std::max(1, std::min(8, (pmwl & 0x07) + 1));
    for (int s = 0; s < 8; ++s) {
        uint8_t r = b[offset + 24 + s * 2];
        uint8_t l = b[offset + 24 + s * 2 + 1];
        line.dcw.steps[s].rate = sysexToDCWRate(r & 127);
        line.dcw.steps[s].level = sysexToDCWLevel(l & 127);
        line.dcw.steps[s].isSustain = (l >> 7) == 1;
        if (line.dcw.steps[s].isSustain) line.dcw.sustainStep = s + 1;
        line.dcw.steps[s].isEnd = (s + 1 == line.dcw.endStep);
    }

    // DCO Envelope
    uint8_t pmpl = b[offset + 40];
    line.dco.velocity = pmpl >> 4;
    line.dco.endStep = std::max(1, std::min(8, (pmpl & 0x07) + 1));
    for (int s = 0; s < 8; ++s) {
        uint8_t r = b[offset + 41 + s * 2];
        uint8_t l = b[offset + 41 + s * 2 + 1];
        line.dco.steps[s].rate = sysexToDCORate(r & 127);
        line.dco.steps[s].level = sysexToDCOLevel(l & 127);
        line.dco.steps[s].isSustain = (l >> 7) == 1;
        if (line.dco.steps[s].isSustain) line.dco.sustainStep = s + 1;
        line.dco.steps[s].isEnd = (s + 1 == line.dco.endStep);
    }
}

static bool parsePatchAt(const std::vector<uint8_t>& fileData, size_t offset, Patch& patch) {
    if (offset >= fileData.size()) return false;

    // Check F0 header or find next F0
    size_t f0Pos = offset;
    if (fileData[f0Pos] != 0xF0) {
        // Search forward up to 1024 bytes
        bool found = false;
        for (size_t i = offset; i < std::min(fileData.size(), offset + 1024); ++i) {
            if (fileData[i] == 0xF0) {
                f0Pos = i;
                found = true;
                break;
            }
        }
        if (!found) return false;
    }

    // Find F7
    size_t f7Pos = f0Pos;
    while (f7Pos < fileData.size() && fileData[f7Pos] != 0xF7) {
        ++f7Pos;
    }
    if (f7Pos >= fileData.size() || fileData[f7Pos] != 0xF7) return false;

    size_t msgLen = f7Pos - f0Pos + 1;
    if (msgLen < 264) return false;

    patch.syxOffset = f0Pos;
    patch.syxLength = msgLen;
    patch.isCZ1 = (msgLen >= 296);

    patch.channel = (fileData[f0Pos + 4] & 0x0F) + 1;
    patch.destAddr = fileData[f0Pos + 6];

    size_t logicalCount = patch.isCZ1 ? 144 : 128;
    std::vector<uint8_t> log(logicalCount, 0);

    size_t dataStart = f0Pos + 7;
    for (size_t i = 0; i < logicalCount; ++i) {
        size_t p = dataStart + i * 2;
        if (p + 1 < fileData.size()) {
            log[i] = unnybblize(fileData[p], fileData[p + 1]);
        }
    }

    // Byte 0: Line Select & Octave
    uint8_t pflag = log[0];
    patch.lineSelect = pflag & 3;
    uint8_t octRaw = (pflag >> 2) & 3;
    if (octRaw == 1) patch.octave = 1;
    else if (octRaw == 2) patch.octave = -1;
    else patch.octave = 0;

    // Detune
    uint8_t pds = log[1];
    patch.detuneSign = (pds == 0) ? '+' : '-';
    uint8_t detFine = log[2] >> 2;
    if (detFine >= 49) detFine -= 3;
    else if (detFine >= 33) detFine -= 2;
    else if (detFine >= 17) detFine -= 1;
    patch.detuneFine = std::max(0, std::min(60, (int)detFine));

    uint8_t detCoarse = log[3];
    patch.detuneOctave = detCoarse / 12;
    patch.detuneNote = detCoarse % 12;
    patch.totalCents = (pds == 0 ? 1 : -1) * (detCoarse * 100 + (patch.detuneFine * 100 / 60));

    // Vibrato
    uint8_t pvk = log[4];
    if (((pvk >> 3) & 1) == 1) patch.vibratoWave = 0;
    else if (((pvk >> 2) & 1) == 1) patch.vibratoWave = 1;
    else if (((pvk >> 5) & 1) == 1) patch.vibratoWave = 2;
    else if (((pvk >> 1) & 1) == 1) patch.vibratoWave = 3;
    else patch.vibratoWave = 0;

    patch.vibratoDelay = decodeVibrato(VIB_DELAY_TABLE, log[6], log[7], log[5]);
    patch.vibratoRate  = decodeVibrato(VIB_RATE_TABLE,  log[9], log[10], log[8]);
    patch.vibratoDepth = decodeVibrato(VIB_DEPTH_TABLE, log[12], log[13], log[11]);

    // Line 1
    patch.line1.lineNum = 1;
    parseLine(log, 14, patch.line1);

    // Line 2
    patch.line2.lineNum = 2;
    parseLine(log, 71, patch.line2);

    // Name
    std::string rawName = "";
    if (patch.isCZ1 && log.size() >= 144) {
        for (size_t i = 128; i < 144; ++i) {
            uint8_t c = log[i];
            if (c >= 32 && c <= 126) rawName += (char)c;
            else rawName += ' ';
        }
    } else {
        rawName = "UNTITLED (CZ-101/1000)";
    }

    patch.name = trimString(rawName);
    if (patch.name.empty()) patch.name = "NO_NAME";

    return true;
}

static void printEnvelope(const char* title, const Envelope& env) {
    std::cout << "  " << title << "  [End: Step " << env.endStep << ", Sustain: ";
    if (env.sustainStep > 0) std::cout << "Step " << env.sustainStep;
    else std::cout << "OFF";
    std::cout << ", Velocity: " << env.velocity << "/15";
    if (env.keyFollow > 0) std::cout << ", KeyFollow: " << env.keyFollow << "/9";
    std::cout << "]\n";

    std::cout << "   Step :";
    for (int s = 0; s < 8; ++s) {
        std::cout << "   S" << (s + 1) << " ";
    }
    std::cout << "\n   Rate :";
    for (int s = 0; s < 8; ++s) {
        if (s < env.endStep) {
            std::cout << "   " << std::setw(2) << std::setfill('0') << env.steps[s].rate << " ";
        } else {
            std::cout << "    -  ";
        }
    }
    std::cout << "\n   Level:";
    for (int s = 0; s < 8; ++s) {
        if (s < env.endStep) {
            std::cout << "   " << std::setw(2) << std::setfill('0') << env.steps[s].level << " ";
        } else {
            std::cout << "    -  ";
        }
    }
    std::cout << "\n   Flags:";
    for (int s = 0; s < 8; ++s) {
        if (s < env.endStep) {
            if (env.steps[s].isSustain && env.steps[s].isEnd) std::cout << " [S+E] ";
            else if (env.steps[s].isSustain) std::cout << " [SUS] ";
            else if (env.steps[s].isEnd)     std::cout << " [END] ";
            else                             std::cout << "   -   ";
        } else {
            std::cout << "   -   ";
        }
    }
    std::cout << "\n";
}

static void printLine(const Line& l) {
    std::cout << "  Waveform 1  : " << WAVEFORMS[l.wave1] << "\n";
    std::cout << "  Waveform 2  : ";
    if (l.wave2On) std::cout << WAVEFORMS[l.wave2] << " (ON)\n";
    else std::cout << "OFF (Single)\n";
    std::cout << "  Window      : " << WINDOWS[l.window] << "\n";
    std::cout << "  Line Volume : Level " << l.mainLevel << " / 15";
    if (l.mute) std::cout << " [MUTED]";
    std::cout << "\n";
    std::cout << "  Key Follow  : DCA: " << l.dca.keyFollow << "/9 | DCW: " << l.dcw.keyFollow << "/9\n";
    std::cout << "\n";

    printEnvelope("DCO Envelope (Pitch)", l.dco);
    std::cout << "\n";
    printEnvelope("DCW Envelope (Wave PD)", l.dcw);
    std::cout << "\n";
    printEnvelope("DCA Envelope (Amplitude)", l.dca);
}

static void printPatch(const std::string& filename, const Patch& p) {
    std::cout << "+=============================================================================+\n";
    std::cout << "| CASIO CZ-1 SOUND PRESET DUMP                                                |\n";
    std::cout << "| File: " << std::left << std::setw(32) << std::setfill(' ') << filename
              << " Offset: " << formatHex(p.syxOffset, 8)
              << " (" << p.syxOffset << ")\n";
    std::cout << "+=============================================================================+\n";
    std::cout << "| PATCH NAME : [" << std::left << std::setw(16) << std::setfill(' ') << p.name << "]       "
              << "FORMAT : " << (p.isCZ1 ? "CZ-1 (144 bytes)" : "CZ-101 (128 bytes)") << "\n";
    std::cout << "| DEST MEMORY: " << formatHex(p.destAddr, 2)
              << " (" << (p.destAddr == 0x60 ? "Edit Buffer" : "Patch Memory") << ")"
              << "          MIDI BASIC CH: " << (int)p.channel << "\n";
    std::cout << "+-----------------------------------------------------------------------------+\n";
    std::cout << "| [GLOBAL SETTINGS]\n";
    std::cout << "|  Line Select : " << LINE_SELECTS[p.lineSelect] << "\n";
    std::cout << "|  Octave      : " << (p.octave >= 0 ? "+" : "") << p.octave << " Octave\n";
    std::cout << "|  Detune (L2) : Sign: " << p.detuneSign << " | Oct: " << p.detuneOctave
              << " | Note: " << p.detuneNote << " | Fine: " << p.detuneFine << "/60"
              << " (Total: " << (p.totalCents >= 0 ? "+" : "") << p.totalCents << " cents)\n";
    std::cout << "|  Vibrato     : " << VIBRATO_WAVES[p.vibratoWave]
              << " | Delay: " << p.vibratoDelay
              << " | Rate: "  << p.vibratoRate
              << " | Depth: " << p.vibratoDepth << "\n";
    std::cout << "|  Modulation  : Ring Mod: " << (p.line1.ringMod ? "ON" : "OFF")
              << " | Noise Mod: " << (p.line1.noiseMod ? "ON" : "OFF") << "\n";
    std::cout << "+-----------------------------------------------------------------------------+\n";
    std::cout << "| [LINE 1]\n";
    printLine(p.line1);
    std::cout << "+-----------------------------------------------------------------------------+\n";
    std::cout << "| [LINE 2]\n";
    printLine(p.line2);
    std::cout << "+=============================================================================+\n";
}

static void listAllPatches(const std::string& filename, const std::vector<uint8_t>& data) {
    std::cout << "+=============================================================================+\n";
    std::cout << "| Casio CZ SysEx Presets in: " << filename << "\n";
    std::cout << "+-------+------------+------------+------+------------------+-----------------+\n";
    std::cout << "| INDEX | OFFSET DEC | OFFSET HEX | SIZE | DEST             | PATCH NAME      |\n";
    std::cout << "+-------+------------+------------+------+------------------+-----------------+\n";

    size_t offset = 0;
    int count = 0;
    while (offset < data.size()) {
        if (data[offset] == 0xF0) {
            Patch p;
            if (parsePatchAt(data, offset, p)) {
                std::cout << "| "
                          << std::right << std::setw(5) << std::setfill(' ') << (count + 1) << " | "
                          << std::setw(10) << p.syxOffset << " | "
                          << formatHex(p.syxOffset, 8) << " | "
                          << std::setw(4) << p.syxLength << " | "
                          << formatHex(p.destAddr, 2)
                          << (p.destAddr == 0x60 ? " (EditBuf) " : " (PatchMem)") << "| "
                          << std::left << std::setw(16) << std::setfill(' ') << p.name << " |\n";
                count++;
                offset = p.syxOffset + p.syxLength;
                continue;
            }
        }
        offset++;
    }

    std::cout << "+-------+------------+------------+------+------------------+-----------------+\n";
    std::cout << " Total presets found: " << count << "\n";
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Enable UTF-8 codepage for Windows Command Prompt and PowerShell
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (argc < 2) {
        std::cerr << "Casio CZ-1 / CZ Series SysEx Preset Dump CUI Tool\n\n";
        std::cerr << "Usage: cz1dump <syx_file> [offset]\n";
        std::cerr << "       cz1dump <syx_file> -p <patch_number (1-based)>\n";
        std::cerr << "       cz1dump <syx_file> --list\n\n";
        std::cerr << "Examples:\n";
        std::cerr << "  cz1dump cz1-factory.syx 0\n";
        std::cerr << "  cz1dump cz1-factory.syx 296\n";
        std::cerr << "  cz1dump cz1-factory.syx 0x0128\n";
        std::cerr << "  cz1dump cz1-factory.syx -p 1\n";
        std::cerr << "  cz1dump cz1-factory.syx --list\n";
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'\n";
        return 1;
    }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    file.close();

    if (data.empty()) {
        std::cerr << "Error: File is empty.\n";
        return 1;
    }

    // Check for --list / -l
    if (argc >= 3 && (std::string(argv[2]) == "--list" || std::string(argv[2]) == "-l")) {
        listAllPatches(filename, data);
        return 0;
    }

    size_t targetOffset = 0;

    // Check for -p <patch_num>
    if (argc >= 4 && (std::string(argv[2]) == "-p" || std::string(argv[2]) == "--patch")) {
        int patchIdx = std::atoi(argv[3]) - 1; // 1-based to 0-based
        if (patchIdx < 0) patchIdx = 0;

        // Find N-th patch
        size_t offset = 0;
        int foundCount = 0;
        bool found = false;
        while (offset < data.size()) {
            if (data[offset] == 0xF0) {
                Patch p;
                if (parsePatchAt(data, offset, p)) {
                    if (foundCount == patchIdx) {
                        targetOffset = p.syxOffset;
                        found = true;
                        break;
                    }
                    foundCount++;
                    offset = p.syxOffset + p.syxLength;
                    continue;
                }
            }
            offset++;
        }
        if (!found) {
            std::cerr << "Error: Patch index " << (patchIdx + 1) << " not found (found " << foundCount << " patches).\n";
            return 1;
        }
    } else if (argc >= 3) {
        // Direct offset argument (decimal or hex)
        std::string offStr = argv[2];
        if (offStr.rfind("0x", 0) == 0 || offStr.rfind("0X", 0) == 0) {
            targetOffset = std::strtoull(offStr.c_str(), nullptr, 16);
        } else {
            targetOffset = std::strtoull(offStr.c_str(), nullptr, 10);
        }
    }

    Patch patch;
    if (!parsePatchAt(data, targetOffset, patch)) {
        std::cerr << "Error: No valid Casio CZ patch found at offset " << targetOffset
                  << " (" << formatHex(targetOffset, 8) << ").\n";
        std::cerr << "Hint: Run 'cz1dump " << filename << " --list' to see all valid patch offsets.\n";
        return 1;
    }

    printPatch(filename, patch);
    return 0;
}
