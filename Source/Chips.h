#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>

// BIT ERA sound sources: emulations of the classic sound chip families (named generically, no brands) plus the
// built-in era samples played through the sampler chips. Every source is synthesized live, so it costs no disk space.
namespace bitera::chips
{
enum Source
{
    eraSample = 0,   // the built-in era sounds, clean playback
    console8,        // 8-bit console: 2 pulses, stepped triangle, LFSR noise, 1.79 MHz period quantization
    handheld4,       // 4-bit handheld: pulses, 32-step 4-bit wave RAM, 7/15-bit noise
    computerSid,     // home computer SID: saw / tri / PWM pulse / noise, combined waves, ring mod, hard sync
    fm16,            // 16-bit console FM: 4 operators, 8 algorithms, 9-bit DAC
    pcFm,            // PC FM card: 2 operators with the 4 classic FM-card waveforms
    psg,             // PSG square chip: 10-bit dividers, periodic / white noise, envelope buzz
    polyCounter,     // early console / home computer poly counters: square, poly4/5/9, div31 (wonky tuning)
    sampler8,        // 8-bit computer sampler / tracker: 8-bit, period quantized, no interpolation, LED filter
    sampler16,       // 16-bit console sampler: 32 kHz, 4-bit block compression, gaussian interpolation
    sampler32,       // 32-bit console sampler: 22 kHz ADPCM, gaussian interpolation
    handheldPcm,     // 32-bit handheld PCM: 8-bit at 13-18 kHz, no filter
    console64,       // 64-bit console: 22 kHz, soft low-pass
    rompler90s,      // 90s PC sound card / ROMpler: 22 kHz 16-bit, linear interpolation
    numSources
};

const char* sourceName (int source);
const char* sourceShort (int source);   // short tag for the browser (e.g. "8-BIT")
bool isSampler (int source);
int numWaves (int source);              // for sampler sources: the number of samples in the bank
juce::String waveName (int source, int wave);
int volumeSteps (int source);           // chips with stepped volume (0 = continuous)
bool volumeLog (int source);            // PSG: 2 dB steps

// one decoded sample of the bank
struct Sample
{
    std::vector<float> data;
    std::vector<float> blockPeak;   // peak of every 16-sample block (for the 4-bit block compression emulation)
    double sampleRate = 44100.0;
    int root = 60;
    juce::String name;
};
using Bank = std::vector<Sample>;
void registerBank (const Bank* bank);   // the processor's bank (for wave names / counts of the sampler sources)

// one oscillator of one layer of one voice
class Osc
{
public:
    void start (int source, int wave, double hostRate, const Bank* bank);
    // freq in Hz of the note (for samplers: freq / root freq is the playback ratio). charAmt 0..1 = per chip colour.
    float next (double freq, float charAmt);
    bool finished() const { return done; }   // sample sources: reached the end

private:
    float pulseBlep (double inc, double width);
    float fmNext (double freq, float charAmt);
    float oplNext (double freq, float charAmt);
    float samplerNext (double freq, float charAmt);
    float noiseNes (double freq, bool shortMode, bool authentic);
    float noiseLfsr15 (double rateHz, bool sevenBit);

    int src = 0, wv = 0;
    double sr = 44100.0, ph = 0.0, ph2 = 0.0, ph3 = 0.0, t = 0.0;
    double clockAcc = 0.0;
    juce::uint32 lfsr = 1;
    float held = 0.0f, lp = 0.0f, lp2 = 0.0f, prevOut = 0.0f;
    int step = 0;
    bool done = false;

    double cacheF = -1.0, cacheA = 0.0, cacheB = 0.0;   // per-frequency cached values
    // FM state
    float opDec[4] {}, opDecK[4] {};
    double opPh[4] {};
    float opOut[4] {}, fb[2] {};

    // sampler state
    const Bank* bk = nullptr;
    double pos = 0.0, tickAcc = 0.0;
    float hold = 0.0f, prevHold = 0.0f;
};
} // namespace bitera::chips
