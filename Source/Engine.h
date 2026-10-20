#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Chips.h"

// BIT ERA sound engine: up to 4 layers per voice, each with its own chip source, envelopes, filter, pitch envelope,
// vibrato, key / velocity response. Poly, mono or legato with glide. Master tone + space + volume.
namespace bitera
{
constexpr int numLayers = 4;

enum FilterType { fOff = 0, fLp12, fLp24, fHp12, fBp12, fNotch, numFilterTypes };
const char* filterName (int type);
enum VoiceMode { vmPoly = 0, vmMono, vmLegato };

struct LayerParams
{
    bool on = false;
    int source = 0, wave = 0;
    float level = 0.8f, pan = 0.0f, coarse = 0.0f, fine = 0.0f, character = 0.5f, velSens = 0.5f;
    int keyLo = 0, keyHi = 127;
    float a = 0.002f, d = 0.6f, s = 1.0f, r = 0.3f;              // seconds / level
    int filter = fOff;
    float cutoff = 20000.0f, reso = 0.1f, fltEnv = 0.0f, keyTrack = 0.0f;
    float fa = 0.002f, fd = 0.4f, fs = 0.0f, fr = 0.3f;
    float pitchEnv = 0.0f, pitchDecay = 0.1f;                      // semitones, seconds
    float vibDepth = 0.0f, vibRate = 5.5f;                         // cents, Hz
};

struct EngineParams
{
    LayerParams layer[numLayers];
    int mode = vmPoly;
    float glide = 0.0f;          // seconds
    float bendRange = 2.0f;      // semitones
    float volumeDb = -6.0f, tone = 1.0f, space = 0.15f;
};

class Engine
{
public:
    Engine();
    void setBank (const chips::Bank* b) { bank = b; }
    void prepare (double sampleRate, int maxBlock);
    void reset();
    // renders into a cleared buffer (1 or 2 channels)
    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const EngineParams& p);
    bool isSilent() const;

    static constexpr int maxVoices = 24;

private:
    struct Env
    {
        enum Stage { idle, attack, decay, sustain, release } stage = idle;
        float level = 0.0f;
        void gate (bool on) { stage = on ? attack : (stage == idle ? idle : release); }
        float next (float a, float d, float s, float r, double sr);
    };

    struct Filter
    {
        float ic1 = 0, ic2 = 0, z[4] {};
        void reset() { ic1 = ic2 = 0; for (auto& v : z) v = 0; }
        float process (float x, int type, float fc, float res, double sr);
    };

    struct LayerVoice
    {
        bool active = false;
        chips::Osc osc;
        Env amp, flt;
        Filter filter;
        float vel = 1.0f;
    };

    struct Voice
    {
        bool active = false, held = false, sustained = false;
        int note = -1;
        float velocity = 0.0f, pitch = 60.0f, target = 60.0f;
        juce::uint64 age = 0;
        double t = 0.0;
        LayerVoice layers[numLayers];
    };

    void noteOn (int note, float vel, const EngineParams& p);
    void noteOff (int note, const EngineParams& p);
    void startVoice (Voice& v, int note, float vel, float fromPitch, const EngineParams& p);
    void render (int start, int num, juce::AudioBuffer<float>& buf, const EngineParams& p);

    const chips::Bank* bank = nullptr;
    double sr = 44100.0;
    Voice voices[maxVoices];
    juce::uint64 counter = 0;
    std::vector<int> heldNotes;   // mono / legato note stack
    float lastPitch = -1.0f, bend = 0.0f;
    bool sustainPedal = false;

    juce::dsp::StateVariableTPTFilter<float> tone;
    juce::dsp::Reverb reverb;
    juce::SmoothedValue<float> gain;
};
} // namespace bitera
