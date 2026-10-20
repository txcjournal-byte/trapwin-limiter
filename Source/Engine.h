#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Chips.h"
#include "Fx.h"

// BIT ERA sound engine.
//   4 layers, each its own little instrument: chip / sampler source, amp + filter envelopes, filter, pitch envelope,
//   vibrato, key / velocity range, voice mode (poly / mono / legato + glide), an arpeggiator / step sequencer
//   and 3 FX slots.
//   Modulation: 3 LFOs, 2 mod envelopes, velocity, key, wheel, aftertouch, bend, random, 8 macros, 2 XY pads
//   -> 16 matrix slots onto layer, FX and master destinations.
//   Master: 4 FX slots, tone, space, volume.
namespace bitera
{
constexpr int numLayers = 4, numLfos = 3, numModEnvs = 2, numMacros = 8, numModSlots = 16, maxSteps = 32;
constexpr int layerFxSlots = 3, masterFxSlots = 4;

enum FilterType { fOff = 0, fLp12, fLp24, fHp12, fBp12, fNotch, numFilterTypes };
const char* filterName (int type);
enum VoiceMode { vmPoly = 0, vmMono, vmLegato };

enum ModSource
{
    msOff = 0, msLfo1, msLfo2, msLfo3, msEnv1, msEnv2, msVelocity, msKey, msWheel, msAftertouch, msBend, msRandom,
    msMacro1, msMacro8 = msMacro1 + 7, msXY1X, msXY1Y, msXY2X, msXY2Y, numModSources
};
enum ModDest
{
    mdPitch = 0, mdLevel, mdPan, mdCutoff, mdReso, mdChar, mdFltEnv, mdVibrato,
    mdFxA1, mdFxAMix, mdFxB1, mdFxBMix, mdFxC1, mdFxCMix,
    mdVolume, mdTone, mdSpace, mdMFx1, mdMFx2, mdMFx3, mdMFx4,
    numModDests
};
const char* modSourceName (int s);
const char* modDestName (int d);
bool modDestIsVoice (int d);    // evaluated per voice (else per layer / master from the global sources)

enum LfoShape { lsSine = 0, lsTriangle, lsSawUp, lsSawDown, lsSquare, lsSampleHold, lsSmoothRandom, numLfoShapes };
const char* lfoShapeName (int s);
const juce::StringArray& syncNames();
double syncBeats (int sync);   // 0 = free

enum ArpMode { amOff = 0, amArp, amSeq, amArpSeq };   // ARP+SEQ: the arp picks the note, the step adds its transpose
enum ArpDir { adUp = 0, adDown, adUpDown, adDownUp, adPlayed, adRandom, adChord, adConverge, numArpDirs };
const char* arpDirName (int d);
enum ArpEnd { aeLoop = 0, aeOneShot, aeHold };

// tracker-style macro: one value per 1/60 s frame (x speed), loops from 'loop' or holds the last value
struct Macro
{
    static constexpr int maxLen = 16;
    int len = 0, loop = -1;
    int v[maxLen] {};
    int at (int frame) const
    {
        if (len <= 0) return 0;
        if (frame < len) return v[frame];
        if (loop >= 0 && loop < len) return v[loop + (frame - len) % (len - loop)];
        return v[len - 1];
    }
};

struct LayerParams
{
    bool on = false;
    int source = 0, wave = 0;
    float level = 0.8f, pan = 0.0f, coarse = 0.0f, fine = 0.0f, character = 0.5f, velSens = 0.5f;
    int keyLo = 0, keyHi = 127;
    float a = 0.002f, d = 0.6f, s = 1.0f, r = 0.3f;
    int filter = fOff;
    float cutoff = 20000.0f, reso = 0.1f, fltEnv = 0.0f, keyTrack = 0.0f;
    float fa = 0.002f, fd = 0.4f, fs = 0.0f, fr = 0.3f;
    float pitchEnv = 0.0f, pitchDecay = 0.1f;
    float vibDepth = 0.0f, vibRate = 5.5f;
    int mode = vmPoly;
    float glide = 0.0f;

    // arpeggiator / sequencer
    int arpMode = amOff, arpDir = adUp, arpSync = 7, arpOctaves = 1, arpSteps = 8, arpEnd = aeLoop;
    float arpGate = 0.6f, arpSwing = 0.0f;
    float stepVel[maxSteps], stepGate[maxSteps], stepPan[maxSteps];
    int stepNote[maxSteps], stepWave[maxSteps];   // stepWave -1 = layer wave
    bool stepOn[maxSteps];

    FxParams fx[layerFxSlots];

    // tracker macros: volume 0..15, arpeggio (semitones), wave index (-1 = keep), pitch (cents per frame); speed = frames per step
    Macro mVol, mArp, mWave, mPitch;
    int macroSpeed = 1;

    LayerParams()
    {
        for (int i = 0; i < maxSteps; ++i) { stepVel[i] = 1.0f; stepGate[i] = 1.0f; stepPan[i] = 0.0f; stepNote[i] = 0; stepWave[i] = -1; stepOn[i] = true; }
    }
};

struct LfoParams { int shape = lsSine; float rate = 2.0f; int sync = 0; bool retrig = true; float fade = 0.0f; bool unipolar = false; };
struct ModEnvParams { float a = 0.01f, d = 0.5f, s = 0.0f, r = 0.3f; };
struct ModSlot { int src = msOff, dst = mdPitch, layer = 0; float amount = 0.0f; };   // layer 0 = all layers

struct EngineParams
{
    LayerParams layer[numLayers];
    LfoParams lfo[numLfos];
    ModEnvParams menv[numModEnvs];
    ModSlot slot[numModSlots];
    float macro[numMacros] {};
    float xy[4] { 0.5f, 0.5f, 0.5f, 0.5f };
    FxParams masterFx[masterFxSlots];
    float bendRange = 2.0f;
    float volumeDb = -6.0f, trimDb = 0.0f, tone = 1.0f, space = 0.15f;
    double bpm = 120.0, ppq = 0.0;
    bool playing = false;
};

class Engine
{
public:
    Engine();
    void setBank (const chips::Bank* b) { bank = b; }
    void prepare (double sampleRate, int maxBlock);
    void reset();
    // stop every note: hard = 3 ms fade (preset change), else a normal release
    void allNotesOff (bool hard);
    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const EngineParams& p);
    bool isSilent() const;

    // current arp / seq step of a layer (for the UI), -1 when idle
    int currentStep (int layer) const { return layers[juce::jlimit (0, numLayers - 1, layer)].uiStep.load(); }
    float macroValue (int m) const { return m >= 0 && m < numMacros ? lastMacro[m] : 0.0f; }

    static constexpr int voicesPerLayer = 24;

private:
    struct Env
    {
        enum Stage { idle, attack, decay, sustain, release } stage = idle;
        float level = 0.0f;
        float cD = -1.0f, cR = -1.0f, kD = 0.0f, kR = 0.0f;   // cached decay / release coefficients
        void gate (bool on) { stage = on ? attack : (stage == idle ? idle : release); }
        float next (float a, float d, float s, float r, double sr);
    };

    struct Filter
    {
        float ic1 = 0, ic2 = 0, z[4] {};
        float lastFc = -1.0f, lastRes = -1.0f, g = 0.0f, k = 0.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;   // cached coefficients
        void reset() { ic1 = ic2 = 0; for (auto& v : z) v = 0; }
        float process (float x, int type, float fc, float res, double sr);
    };

    struct Voice
    {
        bool active = false, held = false, sustained = false, arpNote = false, kill = false;
        float killGain = 1.0f, envSmooth = 0.0f, macroCents = 0.0f;
        int macroFrame = -1, macroArp = 0, macroVol = 15, baseWave = 0;   // envSmooth: the chips' stepped volume, de-clicked
        int note = -1;
        float velocity = 0.0f, pitch = 60.0f, target = 60.0f, vel = 1.0f, panOffset = 0.0f, random = 0.0f;
        double t = 0.0, gateLeft = -1.0;   // arp notes: samples until their note-off
        juce::uint64 age = 0;
        chips::Osc osc;
        Env amp, flt, menv[numModEnvs];
        Filter filter;
        double lfoPh[numLfos] {};
        float lfoSH[numLfos] {}, lfoPrev[numLfos] {};
        juce::Random rng;
    };

    struct Layer
    {
        Voice voices[voicesPerLayer];
        std::vector<int> held;          // mono note stack
        std::vector<std::pair<int, float>> arpHeld;   // notes held for the arp (note, velocity), in played order
        float lastPitch = -1.0f;
        juce::uint64 counter = 0;
        // arp clock
        double stepPos = 0.0;   // samples into the current step
        int step = -1, arpIndex = 0, seqCycles = 0;
        bool arpRunning = false, arpFinished = false;
        std::atomic<int> uiStep { -1 };
        juce::AudioBuffer<float> buffer;
        Fx fx[layerFxSlots];
        juce::Random rng;
    };

    // per layer
    void noteOn (int li, int note, float vel, const EngineParams& p, bool fromArp = false, int stepIdx = -1);
    void noteOff (int li, int note, const EngineParams& p);
    void releaseAll (int li);
    void startVoice (int li, Voice& v, int note, float vel, float fromPitch, const EngineParams& p, int stepIdx);
    void renderLayer (int li, int start, int num, const EngineParams& p);
    void renderVoices (int li, int start, int num, const EngineParams& p);
    void arpStep (int li, const EngineParams& p);
    double stepSamples (const LayerParams& lp, const EngineParams& p, int step) const;

    // modulation
    float lfoValue (int shape, double ph, float sh, float prev) const;
    void globalSources (float* src) const;
    void modSums (const EngineParams& p, const float* src, int layer, bool voiceDests, float* out) const;

    const chips::Bank* bank = nullptr;
    double sr = 44100.0;
    int maxBlockSize = 512;
    Layer layers[numLayers];
    float bend = 0.0f, wheel = 0.0f, aftertouch = 0.0f;
    bool sustainPedal = false;
    double gLfoPh[numLfos] {};
    float gLfoSH[numLfos] {}, gLfoPrev[numLfos] {};
    float lastMacro[numMacros] {};
    float gSrc[numModSources] {};
    juce::Random rng;

    Fx masterFx[masterFxSlots];
    juce::dsp::StateVariableTPTFilter<float> tone;
    juce::dsp::Reverb reverb;
    juce::SmoothedValue<float> gain;
};
} // namespace bitera
