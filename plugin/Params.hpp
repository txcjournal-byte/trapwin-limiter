// Shared parameter definitions for Trap Limit (DSP + UI)
#pragma once
#include <cstdint>

enum ParamId : uint32_t {
    kGain = 0,
    kCeiling,
    kLookahead,
    kAttack,
    kRelease,
    kAutoRel,
    kClip,
    kDrive,
    kSat,
    k808,
    kLink,
    kSubSplit,
    kHpfSc,
    kMS,
    kStereo,
    kPunch,
    kMonoSub,
    kOversample,
    kDither,
    kTruePeak,
    kStream,
    kClipShape,
    kMode,
    kLimit,
    kUnity,
    kDelta,
    kBypass,
    kResetMeters,
    kNumInputParams,
    // ---- outputs (meters) ----
    kOutInL = kNumInputParams,
    kOutInR,
    kOutOutL,
    kOutOutR,
    kOutGR,
    kOutLufsM,
    kOutLufsS,
    kOutLufsI,
    kOutLRA,
    kOutTP,
    kOutCPU,
    kOutLatency,
    kOutClipGR,
    kParamCount
};

enum ParamKind : uint8_t { PK_Float, PK_Bool, PK_Enum, PK_Trigger };

struct ParamInfo {
    const char* name;     // full name
    const char* symbol;   // short symbol
    const char* unit;
    float min, max, def;
    ParamKind kind;
    bool log;             // logarithmic mapping for UI drag
};

static const ParamInfo kParamInfo[kNumInputParams] = {
    { "Gain",        "gain",      "dB",  0.f,   24.f,  0.f,   PK_Float, false },
    { "Ceiling",     "ceiling",   "dB", -12.f,  0.f,  -0.3f,  PK_Float, false },
    { "Lookahead",   "lookahead", "ms",  0.1f,  5.f,   1.5f,  PK_Float, false },
    { "Attack",      "attack",    "ms",  0.05f, 5.f,   0.5f,  PK_Float, true  },
    { "Release",     "release",   "ms",  5.f,   1000.f, 80.f, PK_Float, true  },
    { "Auto Release","autorel",   "",    0.f,   1.f,   1.f,   PK_Bool,  false },
    { "Clip",        "clip",      "",    0.f,   1.f,   1.f,   PK_Bool,  false },
    { "Drive",       "drive",     "dB",  0.f,   12.f,  0.f,   PK_Float, false },
    { "Saturation",  "sat",       "%",   0.f,   100.f, 0.f,   PK_Float, false },
    { "808 Boost",   "boost808",  "%",   0.f,   100.f, 0.f,   PK_Float, false },
    { "Stereo Link", "link",      "%",   0.f,   100.f, 100.f, PK_Float, false },
    { "Sub Split",   "subsplit",  "",    0.f,   1.f,   0.f,   PK_Bool,  false },
    { "HPF Sidechain","hpfsc",    "",    0.f,   1.f,   0.f,   PK_Bool,  false },
    { "Mid/Side",    "ms",        "",    0.f,   1.f,   0.f,   PK_Bool,  false },
    { "Stereo Width","width",     "%",   0.f,   200.f, 100.f, PK_Float, false },
    { "Punch",       "punch",     "%",   0.f,   100.f, 0.f,   PK_Float, false },
    { "Mono Sub",    "monosub",   "",    0.f,   1.f,   0.f,   PK_Bool,  false },
    { "Oversampling","oversample","",    0.f,   2.f,   1.f,   PK_Enum,  false },
    { "Dither",      "dither",    "",    0.f,   1.f,   0.f,   PK_Bool,  false },
    { "True Peak",   "truepeak",  "",    0.f,   1.f,   1.f,   PK_Bool,  false },
    { "Stream Preview","stream",  "",    0.f,   3.f,   0.f,   PK_Enum,  false },
    { "Clip Shape",  "clipshape", "",    0.f,   2.f,   0.f,   PK_Enum,  false },
    { "Mode",        "mode",      "",    0.f,   3.f,   0.f,   PK_Enum,  false },
    { "Limiter",     "limit",     "",    0.f,   1.f,   1.f,   PK_Bool,  false },
    { "Unity Gain",  "unity",     "",    0.f,   1.f,   0.f,   PK_Bool,  false },
    { "Delta",       "delta",     "",    0.f,   1.f,   0.f,   PK_Bool,  false },
    { "Bypass",      "bypass",    "",    0.f,   1.f,   0.f,   PK_Bool,  false },
    { "Reset Meters","resetmeters","",   0.f,   1.f,   0.f,   PK_Trigger, false },
};

static const char* const kOversampleNames[3] = { "1x", "2x", "4x" };
static const char* const kStreamNames[4]     = { "Off", "-14 LUFS", "-16 LUFS", "-9 LUFS" };
static const float       kStreamTargets[4]   = { 0.f, -14.f, -16.f, -9.f };
static const char* const kClipShapeNames[3]  = { "Soft", "Hard", "Tape" };
static const char* const kModeNames[4]       = { "Transparent", "Punchy", "Loud", "Dirty" };

// ---------------- factory presets ----------------
struct PresetValue { uint32_t id; float v; };
struct Preset { const char* name; PresetValue vals[16]; int n; };

static const int kNumPresets = 6;
static const Preset kPresets[kNumPresets] = {
    { "Default", {}, 0 },
    { "808 Heavy", { {kGain,6.f},{kDrive,3.f},{k808,45.f},{kSubSplit,1.f},{kMonoSub,1.f},{kHpfSc,1.f},
                     {kClip,1.f},{kClipShape,0.f},{kRelease,120.f},{kMode,1.f} }, 10 },
    { "Trap Master Loud", { {kGain,9.f},{kDrive,2.f},{kClip,1.f},{kClipShape,0.f},{kPunch,30.f},{kSat,15.f},
                     {kMonoSub,1.f},{kRelease,60.f},{kMode,2.f},{kCeiling,-0.3f},{kOversample,2.f} }, 11 },
    { "Underground Dirty", { {kGain,7.f},{kDrive,5.f},{kSat,55.f},{k808,25.f},{kClip,1.f},{kClipShape,2.f},
                     {kRelease,90.f},{kMode,3.f} }, 8 },
    { "Vocal Bus", { {kGain,4.f},{kClip,0.f},{kAttack,1.5f},{kLookahead,3.f},{kRelease,150.f},{kSat,10.f},
                     {kMode,0.f},{kCeiling,-1.f} }, 8 },
    { "Transparent Master", { {kGain,4.f},{kClip,0.f},{kLookahead,4.f},{kAttack,2.f},{kRelease,200.f},
                     {kMode,0.f},{kCeiling,-1.f},{kOversample,2.f} }, 8 },
};

// ---------------- display data shared via direct access ----------------
static const int kHistoryLen = 360;
struct DisplayHistory {
    float peak[kHistoryLen];   // linear output peak per column
    float gr[kHistoryLen];     // gain reduction dB (<= 0) per column
    volatile int writePos;
};

// implemented in the DSP side; ptr is the value of UI::getPluginInstancePointer()
DisplayHistory* tlGetHistory(void* pluginPtr);
