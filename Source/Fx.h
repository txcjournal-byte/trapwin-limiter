#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// BIT ERA FX engine: one FX slot = a type + 3 controls (0..1) + mix. Retro-first selection: bit crushing, console
// echo, tape wow, radio, plus the studio basics (EQ, comp, chorus, phaser, flanger, delay, reverb, limiter...).
namespace bitera
{
enum FxType
{
    fxOff = 0, fxBite, fxDrive, fxChorus, fxPhaser, fxFlanger, fxDelay, fxConsoleEcho, fxReverb, fxWowFlutter,
    fxEq, fxCompressor, fxRingMod, fxTremolo, fxWidth, fxRadio, fxLimiter, fxFilter, numFxTypes
};

const char* fxName (int type);
const char* fxParamName (int type, int param);         // param 0..2
juce::String fxParamText (int type, int param, float v);  // v 0..1
float fxDefault (int type, int param);

struct FxParams { int type = fxOff; float p[3] { 0.5f, 0.5f, 0.5f }; float mix = 1.0f; };

class Fx
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    // p1Mod / mixMod: modulation offsets (-1..1) from the matrix
    void process (float* L, float* R, int num, const FxParams& fp, double bpm, float p1Mod = 0.0f, float mixMod = 0.0f);

private:
    double sr = 44100.0;
    int lastType = -1;
    // delay memory (2.5 s stereo) shared by delay / echo / chorus / flanger / width
    std::vector<float> dl, dr;
    int wpos = 0;
    double ph = 0.0, ph2 = 0.0;
    float st[16] {};             // misc filter / envelope state
    float hold[2] {}, holdAcc = 0.0f;
    float ap[2][6] {};           // phaser all-pass states
    juce::dsp::Reverb reverb;
    juce::Random rng;
    juce::dsp::IIR::Filter<float> eq[3][2];
    float eqLast[3] { -1, -1, -1 };

    float readDelay (const std::vector<float>& buf, double samplesBack) const;
};
} // namespace bitera
