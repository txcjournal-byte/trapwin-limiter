#include "Chips.h"

namespace bitera::chips
{
namespace
{
constexpr double twoPi = 6.283185307179586;

double polyBlep (double t, double dt)
{
    if (dt <= 0.0) return 0.0;
    if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
    return 0.0;
}

double wrap (double x) { return x - std::floor (x); }

// ---- 4-bit wave RAM tables of the handheld (32 steps, 0..15) -------------------
struct WaveRam
{
    juce::uint8 v[8][32] {};
    WaveRam()
    {
        for (int i = 0; i < 32; ++i)
        {
            const double x = twoPi * i / 32.0;
            auto q = [] (double s) { return (juce::uint8) juce::jlimit (0, 15, (int) std::lround (7.5 + 7.5 * s)); };
            v[0][i] = (juce::uint8) (i / 2);                                                       // saw
            v[1][i] = q (std::sin (x));                                                            // sine
            v[2][i] = q ((std::sin (x) + 0.5 * std::sin (2 * x) + 0.33 * std::sin (3 * x)) / 1.45); // organ
            v[3][i] = q ((std::sin (x) + 0.5 * std::sin (4 * x) + 0.35 * std::sin (7 * x)) / 1.5);  // bell
            v[4][i] = q (std::tanh (2.5 * std::sin (x) + 1.2 * std::sin (3 * x)) * 0.98);         // growl
            v[5][i] = q ((std::sin (x) + 0.8 * std::sin (3 * x) + 0.55 * std::sin (5 * x)) / 1.6);  // voice
            v[6][i] = q (std::tanh (3.0 * std::sin (x)));                                          // bass
            v[7][i] = q ((0.6 * std::sin (x) + std::sin (2 * x)) / 1.4);                           // hollow
        }
    }
};
const WaveRam& waveRam() { static WaveRam w; return w; }

// ---- 16-bit console FM patches (4 operators) --------------------------------------
struct FmPatch { int alg; float ratio[4], level[4], decay[4], sustain[4], feedback; };
const FmPatch fmPatches[] = {
    { 4, { 1, 1, 14, 1 },        { 1.8f, 0.7f, 0.6f, 0.5f },  { 3, 1.2f, 12, 1.5f }, { 0, 0, 0, 0 },          0.2f },  // E.Piano
    { 3, { 1, 1, 3, 1 },         { 2.5f, 1.2f, 1.0f, 1.0f },  { 8, 5, 10, 1.5f },    { 0.1f, 0.2f, 0, 0.4f },  0.5f },  // Slap Bass
    { 4, { 1, 1, 1, 1.002f },    { 2.2f, 0.8f, 1.5f, 0.6f },  { 0.8f, 0.5f, 0.6f, 0.5f }, { 0.6f, 0.7f, 0.5f, 0.7f }, 0.4f },  // Brass
    { 4, { 3.5f, 1, 7.07f, 2 },  { 2.0f, 0.8f, 1.0f, 0.4f },  { 1.5f, 0.6f, 3, 0.9f },   { 0, 0, 0, 0 },          0.0f },  // Bell
    { 0, { 1, 1, 2, 1 },         { 1.0f, 1.2f, 0.8f, 1.0f },  { 0.5f, 0.6f, 0.8f, 0.2f }, { 0.8f, 0.8f, 0.7f, 1 },  0.9f },  // Lead
    { 7, { 0.5f, 1, 2, 3 },      { 0.6f, 0.8f, 0.5f, 0.3f },  { 0, 0, 0, 0 },        { 1, 1, 1, 1 },           0.3f },  // Organ
    { 4, { 1, 1, 3, 2 },         { 3.0f, 0.8f, 1.5f, 0.4f },  { 6, 3, 8, 4 },        { 0, 0, 0, 0 },           0.1f },  // Pluck
    { 0, { 1, 1.41f, 2.83f, 1 }, { 2.5f, 2.0f, 1.5f, 1.0f },  { 2, 2, 2, 0.8f },     { 0.2f, 0.2f, 0.2f, 0.2f }, 0.6f },  // Metal
    { 2, { 1, 2, 1, 1 },         { 1.4f, 1.0f, 1.0f, 1.0f },  { 4, 6, 3, 1 },        { 0.2f, 0, 0.1f, 0.5f },  0.3f },  // Wood Bass
    { 4, { 1, 1.003f, 1, 0.998f }, { 1.2f, 0.7f, 1.2f, 0.7f }, { 0.3f, 0.2f, 0.3f, 0.2f }, { 0.8f, 0.9f, 0.8f, 0.9f }, 0.2f },  // Strings
};
const char* fmNames[] = { "E.Piano", "Slap Bass", "Brass", "Bell", "Lead", "Organ", "Pluck", "Metal", "Wood Bass", "Strings" };
constexpr int numFm = 10;

// ---- PC FM card patches (2 operators, 4 waveforms) ---------------------------------
struct OplPatch { float modRatio, modLevel, carRatio; int modWave, carWave; float feedback, modDecay, modSustain; };
const OplPatch oplPatches[] = {
    { 1, 0.8f, 1, 0, 1, 0.5f, 0, 1 },     // Organ
    { 1, 2.0f, 1, 0, 0, 0.3f, 6, 0.3f },  // Bass
    { 1, 1.6f, 1, 0, 0, 0.6f, 1, 0.7f },  // Brass
    { 2, 0.3f, 1, 0, 0, 0.0f, 0, 0.8f },  // Flute
    { 3, 2.0f, 1, 0, 1, 0.2f, 8, 0.2f },  // Clav
    { 7, 1.6f, 1, 0, 0, 0.0f, 2, 0.0f },  // Bell
    { 1, 1.5f, 1, 3, 0, 0.9f, 0, 1 },     // Synth
    { 5, 1.8f, 1, 0, 2, 0.1f, 5, 0.1f },  // Harpsi
};
const char* oplNames[] = { "Organ", "Bass", "Brass", "Flute", "Clav", "Bell", "Synth", "Harpsi" };
constexpr int numOpl = 8;

double oplWave (int w, double ph)
{
    const double s = std::sin (twoPi * ph);
    switch (w)
    {
        case 1: return s > 0 ? s : 0.0;                                    // half sine
        case 2: return std::abs (s);                                        // abs sine
        case 3: return wrap (ph * 2.0) < 0.5 ? std::abs (s) : 0.0;          // quarter (pulse sine)
        default: return s;
    }
}

const char* nesNames[] = { "Pulse 12.5%", "Pulse 25%", "Pulse 50%", "Pulse 75%", "Stepped Triangle", "Noise", "Metal Noise", "Duty Cycler" };
const char* gbNames[] = { "Pulse 12.5%", "Pulse 25%", "Pulse 50%", "Pulse 75%", "Wave Saw", "Wave Sine", "Wave Organ", "Wave Bell",
                          "Wave Growl", "Wave Voice", "Wave Bass", "Wave Hollow", "Noise 15-bit", "Noise 7-bit" };
const char* sidNames[] = { "Saw", "Triangle", "Pulse PWM", "Noise", "Saw + Tri", "Pulse + Saw", "Pulse + Tri", "Ring Mod", "Hard Sync" };
const char* psgNames[] = { "Square", "Square Octave", "Buzz Bass", "White Noise", "Periodic Noise", "Env Buzz Saw", "Env Buzz Tri", "Square + Noise" };
const char* polyNames[] = { "Pure Square", "Div31 Bass", "Poly4 Buzz", "Poly5 Rumble", "Poly9 Noise", "Poly5 Square", "Engine" };

// poly counter patterns (one bit per step)
struct PolyPatterns
{
    std::vector<juce::uint8> p[7];
    PolyPatterns()
    {
        p[0] = { 1, 0 };
        for (int i = 0; i < 31; ++i) p[1].push_back (i < 13 ? 1 : 0);
        auto lfsr = [] (int bits, int tapA, int tapB, int len)
        {
            std::vector<juce::uint8> out; juce::uint32 r = (1u << bits) - 1u;
            for (int i = 0; i < len; ++i)
            {
                out.push_back ((juce::uint8) (r & 1u));
                const juce::uint32 fb = ((r >> tapA) ^ (r >> tapB)) & 1u;
                r = (r >> 1) | (fb << (bits - 1));
            }
            return out;
        };
        p[2] = lfsr (4, 0, 1, 15);
        p[3] = lfsr (5, 0, 2, 31);
        p[4] = lfsr (9, 0, 4, 511);
        juce::uint8 sqv = 0;   // square toggled by the poly5 counter
        for (int k = 0; k < 2; ++k) for (auto b : p[3]) { if (b) sqv ^= 1; p[5].push_back (sqv); }
        for (int k = 0; k < 3; ++k) for (size_t i = 0; i < p[2].size(); ++i) p[6].push_back ((juce::uint8) (p[2][i] & (k != 1 ? 1 : 0)));
    }
};
const PolyPatterns& polyPatterns() { static PolyPatterns pp; return pp; }

float mtof (double m) { return (float) (440.0 * std::pow (2.0, (m - 69.0) / 12.0)); }
} // namespace

//==============================================================================
const char* sourceName (int s)
{
    static const char* n[] = { "ERA SAMPLES", "8-BIT CONSOLE", "4-BIT HANDHELD", "HOME COMPUTER SID", "16-BIT FM", "PC FM CARD",
                               "PSG SQUARE CHIP", "POLY COUNTER", "8-BIT SAMPLER", "16-BIT SAMPLER", "32-BIT SAMPLER",
                               "HANDHELD PCM", "64-BIT CONSOLE", "90s ROMPLER" };
    return n[juce::jlimit (0, numSources - 1, s)];
}

const char* sourceShort (int s)
{
    static const char* n[] = { "SAMPLE", "8-BIT", "4-BIT", "SID", "FM16", "PC FM", "PSG", "POLY", "S8", "S16", "S32", "PCM", "64", "ROM" };
    return n[juce::jlimit (0, numSources - 1, s)];
}

bool isSampler (int s) { return s == eraSample || s >= sampler8; }

static int bankSize = 0;   // set by the processor when the bank is loaded (for names / counts)
static const Bank* globalBank = nullptr;

int numWaves (int s)
{
    switch (s)
    {
        case console8: return 8;
        case handheld4: return 14;
        case computerSid: return 9;
        case fm16: return numFm;
        case pcFm: return numOpl;
        case psg: return 8;
        case polyCounter: return 7;
        default: return globalBank != nullptr ? (int) globalBank->size() : juce::jmax (1, bankSize);
    }
}

juce::String waveName (int s, int w)
{
    w = juce::jlimit (0, numWaves (s) - 1, w);
    switch (s)
    {
        case console8: return nesNames[w];
        case handheld4: return gbNames[w];
        case computerSid: return sidNames[w];
        case fm16: return fmNames[w];
        case pcFm: return oplNames[w];
        case psg: return psgNames[w];
        case polyCounter: return polyNames[w];
        default: return globalBank != nullptr && w < (int) globalBank->size() ? (*globalBank)[(size_t) w].name : juce::String (w + 1);
    }
}

int volumeSteps (int s) { return (s == console8 || s == handheld4 || s == psg || s == polyCounter) ? 15 : 0; }
bool volumeLog (int s) { return s == psg; }

void registerBank (const Bank* b) { globalBank = b; bankSize = b != nullptr ? (int) b->size() : 0; }

//==============================================================================
void Osc::start (int source, int wave, double hostRate, const Bank* bank)
{
    src = juce::jlimit (0, numSources - 1, source);
    wv = juce::jlimit (0, juce::jmax (0, numWaves (src) - 1), wave);
    sr = hostRate; bk = bank;
    ph = ph2 = ph3 = t = clockAcc = pos = tickAcc = 0.0;
    lfsr = 0x7fff; held = lp = lp2 = prevOut = hold = prevHold = 0.0f; step = 0; done = false;
    for (auto& p : opPh) p = 0.0;
    for (auto& o : opOut) o = 0.0f;
    for (int i = 0; i < 4; ++i) { opDec[i] = 1.0f; opDecK[i] = 0.0f; }
    cacheF = -1.0; cacheA = cacheB = 0.0;
    fb[0] = fb[1] = 0.0f;
    if (src == computerSid) lfsr = 0x7ffff8;
    if (isSampler (src) && (bk == nullptr || bk->empty())) done = true;
}

float Osc::pulseBlep (double inc, double width)
{
    double v = ph < width ? 1.0 : -1.0;
    v += polyBlep (ph, inc);
    v -= polyBlep (wrap (ph + 1.0 - width), inc);
    ph = wrap (ph + inc);
    return (float) v;
}

float Osc::noiseNes (double freq, bool shortMode, bool authentic)
{
    static const int periods[16] = { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 };
    if (freq != cacheF)
    {
        cacheF = freq;
        const double target = 1789773.0 / juce::jmax (1.0, freq * 64.0);
        int best = 0;
        for (int i = 1; i < 16; ++i)
            if (std::abs (std::log ((double) periods[i] / target)) < std::abs (std::log ((double) periods[best] / target))) best = i;
        cacheA = 1789773.0 / periods[best];   // the 16 hardware rates (authentic)
        cacheB = juce::jmin (1789773.0 / 4.0, freq * 64.0);   // continuous (playable)
    }
    clockAcc += (authentic ? cacheA : cacheB) / sr;
    while (clockAcc >= 1.0)
    {
        clockAcc -= 1.0;
        const juce::uint32 fbBit = (lfsr ^ (lfsr >> (shortMode ? 6 : 1))) & 1u;
        lfsr = (lfsr >> 1) | (fbBit << 14);
    }
    return (lfsr & 1u) ? -1.0f : 1.0f;
}

float Osc::noiseLfsr15 (double rateHz, bool sevenBit)
{
    clockAcc += rateHz / sr;
    while (clockAcc >= 1.0)
    {
        clockAcc -= 1.0;
        const juce::uint32 fbBit = (lfsr ^ (lfsr >> 1)) & 1u;
        lfsr = (lfsr >> 1) | (fbBit << 14);
        if (sevenBit) lfsr = (lfsr & ~(1u << 6)) | (fbBit << 6);
    }
    return (lfsr & 1u) ? -1.0f : 1.0f;
}

float Osc::next (double freq, float c)
{
    if (done) return 0.0f;
    freq = juce::jlimit (1.0, sr * 0.45, freq);
    t += 1.0 / sr;
    float out = 0.0f;

    switch (src)
    {
        case console8:
        {
            constexpr double clk = 1789773.0;
            if (wv <= 3 || wv == 7)
            {
                // 11-bit period of the chip; notes below its range keep their pitch (playable everywhere)
                const double praw = std::round (clk / (16.0 * freq)) - 1.0;
                const double fq = praw > 2047.0 ? freq : clk / (16.0 * (juce::jmax (8.0, praw) + 1.0));
                static const double duty[4] = { 0.125, 0.25, 0.5, 0.75 };
                double w = duty[juce::jmin (wv, 3)];
                if (wv == 7)   // duty cycler: changes duty every few frames
                {
                    const double frame = (1.0 / 60.0) * (1.0 + (1.0 - c) * 7.0);
                    w = duty[(int) (t / frame) % 3];
                }
                out = pulseBlep (fq / sr, w) * 0.5f;
            }
            else if (wv == 4)
            {
                const double praw = std::round (clk / (32.0 * freq)) - 1.0;
                const double fq = praw > 2047.0 ? freq : clk / (32.0 * (juce::jmax (0.0, praw) + 1.0));
                ph = wrap (ph + fq / sr);
                const int idx = (int) (ph * 32.0);
                const int v = idx < 16 ? 15 - idx : idx - 16;
                out = (float) (v / 7.5 - 1.0) * 0.55f;
            }
            else
                out = noiseNes (freq, wv == 6, c < 0.2f) * 0.4f;
            if (c > 0.0f && wv != 7) out = (float) (std::tanh (out * (1.0f + c * 3.0f)) / std::tanh (1.0f + c * 3.0f)) * 0.55f;   // mixer crunch
            break;
        }
        case handheld4:
        {
            if (wv <= 3)
            {
                const double x = std::round (2048.0 - 131072.0 / freq);
                const double fq = x < 0.0 ? freq : 131072.0 / (2048.0 - juce::jmin (2047.0, x));
                static const double duty[4] = { 0.125, 0.25, 0.5, 0.75 };
                out = pulseBlep (fq / sr, duty[wv]) * 0.5f;
            }
            else if (wv <= 11)
            {
                const double x = std::round (2048.0 - 65536.0 / freq);
                const double fq = x < 0.0 ? freq : 65536.0 / (2048.0 - juce::jmin (2047.0, x));
                ph = wrap (ph + fq / sr);
                int v = waveRam().v[wv - 4][(int) (ph * 32.0) & 31];
                const int shift = c < 0.4f ? 0 : (c < 0.8f ? 1 : 2);   // wave volume shift = coarser steps
                v = (v >> shift) << shift;
                out = (float) (v / 7.5 - 1.0) * 0.5f;
            }
            else
            {
                if (freq != cacheF) { cacheF = freq; cacheA = juce::jmin (262144.0, freq * 32.0); }
                out = noiseLfsr15 (cacheA, wv == 13) * 0.4f;
            }
            // the handheld's output capacitor (high-pass, gives the thin click)
            lp += (out - lp) * 0.0035f;
            out -= lp;
            break;
        }
        case computerSid:
        {
            const double inc = std::round (freq * 16777216.0 / 985248.0) * 985248.0 / 16777216.0 / sr;
            const double pw = juce::jlimit (0.05, 0.95, 0.5 + 0.42 * c * std::sin (twoPi * 0.7 * t));
            const double old = ph;
            ph = wrap (ph + inc);
            const auto acc = (juce::uint32) (ph * 16777216.0);
            const juce::uint32 saw12 = acc >> 12;
            const juce::uint32 tri12 = (((acc & 0x800000u) ? ~acc : acc) >> 11) & 0xfffu;
            const juce::uint32 pul12 = ph < pw ? 0xfffu : 0u;
            auto u12 = [] (juce::uint32 v) { return (float) (v / 2047.5 - 1.0); };
            switch (wv)
            {
                case 0: out = (float) (2.0 * ph - 1.0 - polyBlep (ph, inc)); break;
                case 1: out = (float) (4.0 * std::abs (ph - 0.5) - 1.0); break;
                case 2: { double v = ph < pw ? 1.0 : -1.0; v += polyBlep (ph, inc); v -= polyBlep (wrap (ph + 1.0 - pw), inc); out = (float) v; break; }
                case 3:
                {
                    // noise LFSR clocked 16x per cycle (bit 19 of the accumulator)
                    if (((acc >> 19) & 1u) != (((juce::uint32) (old * 16777216.0) >> 19) & 1u))
                    {
                        const juce::uint32 b = ((lfsr >> 22) ^ (lfsr >> 17)) & 1u;
                        lfsr = ((lfsr << 1) | b) & 0x7fffffu;
                    }
                    out = (float) (((lfsr >> 15) & 0xffu) / 127.5 - 1.0);
                    break;
                }
                case 4: out = u12 (saw12 & tri12) * 1.6f; break;
                case 5: out = u12 (saw12 & pul12) * 1.3f; break;
                case 6: out = u12 (tri12 & pul12) * 1.3f; break;
                case 7:
                {
                    ph2 = wrap (ph2 + inc * (1.0 + c * 3.0 + 0.5));
                    out = (float) (4.0 * std::abs (ph - 0.5) - 1.0) * (ph2 < 0.5 ? 1.0f : -1.0f);
                    break;
                }
                default:
                {
                    if (ph < old) ph2 = 0.0;   // hard sync to the master
                    ph2 = wrap (ph2 + inc * (1.0 + c * 4.0));
                    out = (float) (2.0 * ph2 - 1.0);
                    break;
                }
            }
            // 12-bit DAC + DC blocker
            out = std::round (out * 2048.0f) / 2048.0f;
            lp += (out - lp) * 0.002f;
            out = (out - lp) * 0.5f;
            break;
        }
        case fm16: out = fmNext (freq, c); break;
        case pcFm: out = oplNext (freq, c); break;
        case psg:
        {
            constexpr double clk = 3579545.0;
            auto quant = [clk] (double f) { const double n = juce::jlimit (1.0, 65535.0, std::round (clk / (32.0 * f))); return clk / (32.0 * n); };
            switch (wv)
            {
                case 0: out = pulseBlep (quant (freq) / sr, 0.5) * 0.5f; break;
                case 1:
                {
                    const double inc2 = quant (freq * 2.0) / sr;
                    double v2 = ph2 < 0.5 ? 1.0 : -1.0; v2 += polyBlep (ph2, inc2); v2 -= polyBlep (wrap (ph2 + 0.5), inc2); ph2 = wrap (ph2 + inc2);
                    out = (pulseBlep (quant (freq) / sr, 0.5) * 0.5f + (float) v2 * 0.3f);
                    break;
                }
                case 2:   // periodic noise as a 1/15 pulse: the classic buzz bass
                {
                    clockAcc += quant (freq * 15.0) / sr;
                    while (clockAcc >= 1.0) { clockAcc -= 1.0; step = (step + 1) % 15; }
                    out = (step == 0 ? 1.0f : -0.07f) * 0.6f;
                    lp += (out - lp) * 0.01f; out -= lp;
                    break;
                }
                case 3:
                {
                    clockAcc += quant (freq * 16.0) / sr;
                    while (clockAcc >= 1.0) { clockAcc -= 1.0; const juce::uint32 b = (lfsr ^ (lfsr >> 3)) & 1u; lfsr = (lfsr >> 1) | (b << 15); }
                    out = (lfsr & 1u) ? 0.4f : -0.4f;
                    break;
                }
                case 4:
                {
                    clockAcc += quant (freq * 15.0 * 0.5) / sr;   // periodic noise one octave down: metallic, follows the key
                    while (clockAcc >= 1.0) { clockAcc -= 1.0; step = (step + 1) % 15; }
                    out = (step == 0 ? 1.0f : -0.07f) * 0.6f;
                    lp += (out - lp) * 0.01f; out -= lp;
                    break;
                }
                case 5: case 6:   // envelope generator used as a waveform (32 log steps)
                {
                    constexpr double aclk = 1789773.0;
                    const double div = wv == 5 ? 256.0 : 512.0;
                    const double e = juce::jlimit (1.0, 65535.0, std::round (aclk / (div * freq)));
                    ph = wrap (ph + aclk / (div * e) / sr);
                    int s = (int) (ph * 32.0);
                    if (wv == 6) s = (int) (ph * 64.0), s = s < 32 ? s : 63 - s;
                    out = (float) (std::pow (10.0, (s - 31) * 1.5 / 20.0) * 2.0 - 1.0) * 0.5f;
                    break;
                }
                default:
                {
                    const float sqv = pulseBlep (quant (freq) / sr, 0.5);
                    clockAcc += quant (freq * 8.0) / sr;
                    while (clockAcc >= 1.0) { clockAcc -= 1.0; const juce::uint32 b = (lfsr ^ (lfsr >> 3)) & 1u; lfsr = (lfsr >> 1) | (b << 15); }
                    const float nz = (lfsr & 1u) ? 1.0f : -1.0f;
                    out = (sqv * (1.0f - c * 0.8f) + nz * (0.2f + c * 0.6f)) * 0.45f;
                    break;
                }
            }
            break;
        }
        case polyCounter:
        {
            const auto& pat = polyPatterns().p[juce::jlimit (0, 6, wv)];
            const double L = (double) pat.size();
            const bool tia = c < 0.2f;   // low character: the authentic 5-bit dividers (out of tune); else 16-bit dividers (in tune)
            const double base = tia ? 31400.0 : 1789773.0, maxN = tia ? 31.0 : 65535.0;
            double stepsPerCycle = wv == 0 ? 2.0 : L;
            const double n = juce::jlimit (0.0, maxN, std::round (base / (freq * stepsPerCycle)) - 1.0);
            clockAcc += (base / (n + 1.0)) / sr;
            while (clockAcc >= 1.0) { clockAcc -= 1.0; step = (step + 1) % (int) L; }
            out = pat[(size_t) step] ? 0.45f : -0.45f;
            lp += (out - lp) * 0.004f; out -= lp;
            break;
        }
        default: out = samplerNext (freq, c); break;
    }
    return out;
}

float Osc::fmNext (double freq, float c)
{
    const auto& p = fmPatches[juce::jlimit (0, numFm - 1, wv)];
    float env[4];
    if (opDecK[0] <= 0.0f)
        for (int i = 0; i < 4; ++i) { opDecK[i] = (float) std::exp (-p.decay[i] / sr); opDec[i] = 1.0f; }
    for (int i = 0; i < 4; ++i) { env[i] = p.sustain[i] + (1.0f - p.sustain[i]) * opDec[i]; opDec[i] *= opDecK[i]; }
    const float idx = 0.3f + 1.4f * c;
    auto op = [&] (int i, double mod)
    {
        opPh[i] = wrap (opPh[i] + freq * p.ratio[i] / sr);
        return (float) std::sin (twoPi * opPh[i] + mod) * env[i];
    };
    auto m = [&] (int i) { return (double) (opOut[i] * p.level[i] * idx * 2.0f); };   // modulator output in radians
    const double fbMod = p.feedback * 1.5 * (fb[0] + fb[1]) * 0.5;
    float o = 0.0f;
    opOut[0] = op (0, fbMod);
    fb[1] = fb[0]; fb[0] = opOut[0];
    switch (p.alg)
    {
        case 0: opOut[1] = op (1, m (0)); opOut[2] = op (2, m (1)); opOut[3] = op (3, m (2)); o = opOut[3] * p.level[3]; break;
        case 1: opOut[1] = op (1, 0); opOut[2] = op (2, m (0) + m (1)); opOut[3] = op (3, m (2)); o = opOut[3] * p.level[3]; break;
        case 2: opOut[1] = op (1, 0); opOut[2] = op (2, m (1)); opOut[3] = op (3, m (0) + m (2)); o = opOut[3] * p.level[3]; break;
        case 3: opOut[1] = op (1, m (0)); opOut[2] = op (2, 0); opOut[3] = op (3, m (1) + m (2)); o = opOut[3] * p.level[3]; break;
        case 4: opOut[1] = op (1, m (0)); opOut[2] = op (2, 0); opOut[3] = op (3, m (2)); o = opOut[1] * p.level[1] + opOut[3] * p.level[3]; break;
        case 5: opOut[1] = op (1, m (0)); opOut[2] = op (2, m (0)); opOut[3] = op (3, m (0)); o = (opOut[1] * p.level[1] + opOut[2] * p.level[2] + opOut[3] * p.level[3]) * 0.6f; break;
        case 6: opOut[1] = op (1, m (0)); opOut[2] = op (2, 0); opOut[3] = op (3, 0); o = (opOut[1] * p.level[1] + opOut[2] * p.level[2] + opOut[3] * p.level[3]) * 0.6f; break;
        default: opOut[1] = op (1, 0); opOut[2] = op (2, 0); opOut[3] = op (3, 0); o = (opOut[0] * p.level[0] + opOut[1] * p.level[1] + opOut[2] * p.level[2] + opOut[3] * p.level[3]) * 0.5f; break;
    }
    // 9-bit DAC with the crossover "ladder" step
    o = juce::jlimit (-1.0f, 1.0f, o * 0.6f);
    o = std::round (o * 256.0f) / 256.0f;
    o += (o >= 0.0f ? 1.0f : -1.0f) / 512.0f;
    return o * 0.8f;
}

float Osc::oplNext (double freq, float c)
{
    const auto& p = oplPatches[juce::jlimit (0, numOpl - 1, wv)];
    // 10-bit F-number + block quantization
    if (freq != cacheF)
    {
        cacheF = freq;
        int block = 0;
        while (block < 7 && freq * std::pow (2.0, 20 - block) / 49716.0 > 1023.0) ++block;
        const double fnum = std::round (freq * std::pow (2.0, 20 - block) / 49716.0);
        cacheA = fnum * 49716.0 / std::pow (2.0, 20 - block);
    }
    const double fq = cacheA;
    if (opDecK[0] <= 0.0f) { opDecK[0] = (float) std::exp (-p.modDecay / sr); opDec[0] = 1.0f; }
    const float menv = p.modSustain + (1.0f - p.modSustain) * opDec[0];
    opDec[0] *= opDecK[0];
    ph2 = wrap (ph2 + fq * p.modRatio / sr);
    const double fbm = p.feedback * 1.2 * (fb[0] + fb[1]) * 0.5;
    const float mo = (float) oplWave (p.modWave, wrap (ph2 + fbm / twoPi)) * menv;
    fb[1] = fb[0]; fb[0] = mo;
    ph = wrap (ph + fq * p.carRatio / sr);
    const double mod = mo * p.modLevel * (0.3 + 1.4 * c) * 2.0;
    float o = (float) oplWave (p.carWave, wrap (ph + mod / twoPi));
    lp += (o - lp) * 0.002f;   // the half / abs waves carry DC
    o -= lp;
    return std::round (o * 4096.0f) / 4096.0f * 0.55f;
}

float Osc::samplerNext (double freq, float c)
{
    if (bk == nullptr || bk->empty()) { done = true; return 0.0f; }
    const auto& s = (*bk)[(size_t) juce::jlimit (0, (int) bk->size() - 1, wv)];
    const auto& d = s.data;
    const double len = (double) d.size();
    const double ratio = freq / mtof (s.root);
    auto at = [&] (double i) -> float { const auto k = (size_t) juce::jlimit (0.0, len - 1.0, i); return d[k]; };

    switch (src)
    {
        case eraSample:
        {
            if (pos >= len - 1.0) { done = true; return 0.0f; }
            const auto i = (size_t) pos; const float f = (float) (pos - (double) i);
            const float y = d[i] + (d[juce::jmin (i + 1, d.size() - 1)] - d[i]) * f;
            pos += ratio * s.sampleRate / sr;
            return y;
        }
        case sampler8:   // 16.7 kHz 8-bit samples, period-quantized playback, no interpolation, fixed + LED filter
        {
            constexpr double paula = 3546895.0, base = 16726.0;
            const double period = juce::jlimit (16.0, 65535.0, std::round (paula / (base * ratio)));   // below 113 = beyond the real chip, kept playable
            tickAcc += (paula / period) / sr;
            while (tickAcc >= 1.0)
            {
                tickAcc -= 1.0;
                pos += s.sampleRate / base;
                hold = std::round (at (pos) * 127.0f) / 127.0f;
            }
            if (pos >= len) { done = true; return 0.0f; }
            lp += (hold - lp) * (float) (1.0 - std::exp (-twoPi * 4900.0 / sr));
            float y = lp;
            if (c > 0.5f) { const float k = (float) (1.0 - std::exp (-twoPi * 3300.0 / sr)); lp2 += (y - lp2) * k; prevOut += (lp2 - prevOut) * k; y = prevOut; }
            return y;
        }
        case sampler16:   // 32 kHz output, 16 kHz samples with 4-bit block compression, gaussian-like interpolation
        {
            constexpr double outRate = 32000.0, grid = 16000.0;
            tickAcc += outRate / sr;
            while (tickAcc >= 1.0)
            {
                tickAcc -= 1.0;
                pos += ratio * grid / outRate;   // pos in grid samples
                const double gi = std::floor (pos); const float f = (float) (pos - gi);
                auto g = [&] (double k)
                {
                    const double si = k * s.sampleRate / grid;
                    const float v = at (si);
                    const auto blk = (size_t) juce::jlimit (0.0, (double) s.blockPeak.size() - 1.0, si / 16.0);
                    const float stepSize = juce::jmax (1.0e-4f, s.blockPeak.empty() ? 0.01f : s.blockPeak[blk] / (8.0f + 24.0f * c));
                    return std::round (v / stepSize) * stepSize;
                };
                // cubic B-spline ~ the console's gaussian table: smooth and a little dull
                const float a = g (gi - 1), b = g (gi), cc = g (gi + 1), e = g (gi + 2);
                const float f2 = f * f, f3 = f2 * f;
                hold = ((1 - f) * (1 - f) * (1 - f) * a + (3 * f3 - 6 * f2 + 4) * b + (-3 * f3 + 3 * f2 + 3 * f + 1) * cc + f3 * e) / 6.0f;
            }
            if (pos * s.sampleRate / grid >= len) { done = true; return 0.0f; }
            return hold;
        }
        case sampler32:   // 22 kHz ADPCM samples, gaussian interpolation at 44.1 kHz
        {
            constexpr double grid = 22050.0;
            pos += ratio * grid / sr;
            if (pos * s.sampleRate / grid >= len) { done = true; return 0.0f; }
            const double gi = std::floor (pos); const float f = (float) (pos - gi);
            auto g = [&] (double k)
            {
                const double si = k * s.sampleRate / grid;
                const auto blk = (size_t) juce::jlimit (0.0, (double) s.blockPeak.size() - 1.0, si / 16.0);
                const float stepSize = juce::jmax (1.0e-5f, s.blockPeak.empty() ? 0.004f : s.blockPeak[blk] / (16.0f + 112.0f * c));
                return std::round (at (si) / stepSize) * stepSize;
            };
            const float a = g (gi - 1), b = g (gi), cc = g (gi + 1), e = g (gi + 2);
            const float f2 = f * f, f3 = f2 * f;
            return ((1 - f) * (1 - f) * (1 - f) * a + (3 * f3 - 6 * f2 + 4) * b + (-3 * f3 + 3 * f2 + 3 * f + 1) * cc + f3 * e) / 6.0f;
        }
        case handheldPcm:   // 8-bit at the handheld's mixing rate, no interpolation and no filter: gritty
        {
            const double grid = 10512.0 + (21024.0 - 10512.0) * c;
            tickAcc += grid / sr;
            while (tickAcc >= 1.0)
            {
                tickAcc -= 1.0;
                pos += ratio;
                hold = std::round (at (pos * s.sampleRate / grid) * 127.0f) / 127.0f;
            }
            if (pos * s.sampleRate / grid >= len) { done = true; return 0.0f; }
            return hold;
        }
        case console64:   // 22 kHz, linear interpolation, soft low-pass (the console's blur)
        {
            constexpr double grid = 22050.0;
            pos += ratio * grid / sr;
            if (pos * s.sampleRate / grid >= len) { done = true; return 0.0f; }
            const double gi = std::floor (pos); const float f = (float) (pos - gi);
            const float y = at (gi * s.sampleRate / grid) * (1 - f) + at ((gi + 1) * s.sampleRate / grid) * f;
            const float k = (float) (1.0 - std::exp (-twoPi * (2500.0 + 7500.0 * c) / sr));
            lp += (y - lp) * k; lp2 += (lp - lp2) * k;
            return lp2;
        }
        default:   // 90s ROMpler: 11-44 kHz 16-bit, linear interpolation, no anti-aliasing
        {
            const double grid = 11025.0 + (44100.0 - 11025.0) * c;
            pos += ratio * grid / sr;
            if (pos * s.sampleRate / grid >= len) { done = true; return 0.0f; }
            const double gi = std::floor (pos); const float f = (float) (pos - gi);
            return at (gi * s.sampleRate / grid) * (1 - f) + at ((gi + 1) * s.sampleRate / grid) * f;
        }
    }
}
} // namespace bitera::chips
