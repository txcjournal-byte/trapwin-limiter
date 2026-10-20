#include "Fx.h"

namespace bitera
{
namespace
{
constexpr double twoPi = 6.283185307179586;
const char* divNames[] = { "1/32", "1/16 T", "1/16", "1/8 T", "1/16 D", "1/8", "1/4 T", "1/8 D", "1/4", "1/2 T", "1/4 D", "1/2", "1/1" };
const double divBeats[] = { 0.125, 1.0 / 6.0, 0.25, 1.0 / 3.0, 0.375, 0.5, 2.0 / 3.0, 0.75, 1.0, 4.0 / 3.0, 1.5, 2.0, 4.0 };
constexpr int numDivs = 13;
int divIndex (float v) { return juce::jlimit (0, numDivs - 1, (int) std::lround (v * (numDivs - 1))); }

// 8-tap FIR presets for the console echo feedback path (console style: from bright to very dark)
const float firs[8][8] = {
    { 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0.5f, 0.3f, 0.2f, 0, 0, 0, 0, 0 },
    { 0.3f, 0.25f, 0.2f, 0.15f, 0.1f, 0, 0, 0 },
    { 0.2f, 0.18f, 0.16f, 0.14f, 0.12f, 0.1f, 0.06f, 0.04f },
    { 0.125f, 0.125f, 0.125f, 0.125f, 0.125f, 0.125f, 0.125f, 0.125f },
    { 0.7f, -0.2f, 0.3f, 0.1f, 0.05f, 0.03f, 0.01f, 0.01f },
    { -0.1f, 0.3f, 0.5f, 0.3f, -0.1f, 0.05f, 0.03f, 0.02f },
    { 0.05f, 0.1f, 0.15f, 0.2f, 0.2f, 0.15f, 0.1f, 0.05f },
};

float svf (float x, float fc, float res, double rate, float& ic1, float& ic2, int mode)
{
    const float g = (float) std::tan (juce::MathConstants<double>::pi * juce::jlimit (20.0f, (float) (rate * 0.45), fc) / rate);
    const float k = 2.0f - 1.9f * res;
    const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
    const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
    return mode == 0 ? v2 : mode == 1 ? v1 : x - k * v1 - v2;
}
} // namespace

const char* fxName (int t)
{
    static const char* n[] = { "OFF", "BITE CRUSHER", "DRIVE", "CHORUS", "PHASER", "FLANGER", "DELAY", "CONSOLE ECHO", "REVERB",
                               "WOW & FLUTTER", "EQ", "COMPRESSOR", "RING MOD", "TREMOLO / PAN", "STEREO WIDTH", "RADIO", "LIMITER", "FILTER" };
    return n[juce::jlimit (0, numFxTypes - 1, t)];
}

const char* fxParamName (int t, int i)
{
    static const char* n[numFxTypes][3] = {
        { "-", "-", "-" }, { "BITS", "RATE", "JITTER" }, { "DRIVE", "MODE", "TONE" }, { "RATE", "DEPTH", "WIDTH" },
        { "RATE", "DEPTH", "FEEDBACK" }, { "RATE", "DEPTH", "FEEDBACK" }, { "TIME", "FEEDBACK", "MODE" },
        { "TIME", "FEEDBACK", "FILTER" }, { "SIZE", "DAMPING", "WIDTH" }, { "WOW", "FLUTTER", "HISS" },
        { "LOW", "MID", "HIGH" }, { "THRESHOLD", "RATIO", "SPEED" }, { "FREQ", "SHAPE", "SPREAD" },
        { "RATE", "DEPTH", "MODE" }, { "WIDTH", "HAAS", "BASS MONO" }, { "BAND", "NOISE", "CRACKLE" },
        { "CEILING", "RELEASE", "DRIVE" }, { "CUTOFF", "RESONANCE", "TYPE" } };
    return n[juce::jlimit (0, numFxTypes - 1, t)][juce::jlimit (0, 2, i)];
}

float fxDefault (int t, int i)
{
    switch (t)
    {
        case fxBite: return i == 0 ? 0.45f : i == 1 ? 0.55f : 0.0f;
        case fxDrive: return i == 0 ? 0.3f : i == 1 ? 0.0f : 0.7f;
        case fxDelay: return i == 0 ? 0.6f : i == 1 ? 0.35f : 0.0f;
        case fxConsoleEcho: return i == 0 ? 0.4f : i == 1 ? 0.45f : 0.3f;
        case fxReverb: return i == 0 ? 0.6f : i == 1 ? 0.4f : 1.0f;
        case fxEq: return 0.5f;
        case fxCompressor: return i == 0 ? 0.6f : i == 1 ? 0.3f : 0.5f;
        case fxWidth: return i == 0 ? 0.65f : i == 1 ? 0.0f : 0.5f;
        case fxLimiter: return i == 0 ? 0.95f : i == 1 ? 0.3f : 0.0f;
        case fxFilter: return i == 0 ? 0.7f : i == 1 ? 0.2f : 0.0f;
        case fxWowFlutter: return i == 0 ? 0.3f : i == 1 ? 0.2f : 0.1f;
        default: return 0.5f;
    }
}

juce::String fxParamText (int t, int i, float v)
{
    using juce::String;
    auto pct = [v] { return String (juce::roundToInt (v * 100.0f)) + " %"; };
    auto hz = [] (double f) { return f < 1000.0 ? String (f, f < 10.0 ? 2 : 0) + " Hz" : String (f / 1000.0, 1) + " kHz"; };
    auto db = [] (double d) { return (d > 0 ? "+" : "") + String (d, 1) + " dB"; };
    switch (t)
    {
        case fxBite: return i == 0 ? String (juce::roundToInt (2 + v * 14)) + " bit" : i == 1 ? hz (1000.0 * std::pow (44.1, v)) : pct();
        case fxDrive:
            if (i == 1) { static const char* m[] = { "SOFT", "HARD", "FUZZ", "TAPE", "FOLD" }; return m[juce::jlimit (0, 4, (int) (v * 4.99f))]; }
            return i == 0 ? pct() : hz (1000.0 * std::pow (18.0, v));
        case fxChorus: case fxPhaser: case fxFlanger: return i == 0 ? hz (0.05 * std::pow (100.0, v)) : pct();
        case fxDelay:
            if (i == 0) return divNames[divIndex (v)];
            if (i == 2) { static const char* m[] = { "STEREO", "PING-PONG", "TAPE" }; return m[juce::jlimit (0, 2, (int) (v * 2.99f))]; }
            return pct();
        case fxConsoleEcho: return i == 0 ? String (16 * juce::jlimit (1, 15, 1 + (int) (v * 14.99f))) + " ms" : i == 2 ? "FIR " + String (juce::jlimit (0, 7, (int) (v * 7.99f)) + 1) : pct();
        case fxEq: return db ((v - 0.5) * 24.0);
        case fxCompressor: return i == 0 ? db (-40.0 + v * 40.0) : i == 1 ? String (1.0 + v * 19.0, 1) + ":1" : pct();
        case fxRingMod: return i == 0 ? hz (20.0 * std::pow (250.0, v)) : pct();
        case fxTremolo: return i == 0 ? hz (0.1 * std::pow (200.0, v)) : i == 2 ? String (v < 0.5f ? "TREMOLO" : "AUTO-PAN") : pct();
        case fxWidth: return i == 0 ? String (juce::roundToInt (v * 200.0f)) + " %" : i == 1 ? String (v * 20.0, 1) + " ms" : pct();
        case fxRadio: return i == 0 ? hz (300.0 * std::pow (10.0, v)) : pct();
        case fxLimiter: return i == 0 ? db (-24.0 + v * 24.0) : i == 1 ? String (juce::roundToInt (10 + v * 490)) + " ms" : db (v * 18.0);
        case fxFilter:
            if (i == 2) { static const char* m[] = { "LOW-PASS", "BAND-PASS", "HIGH-PASS" }; return m[juce::jlimit (0, 2, (int) (v * 2.99f))]; }
            return i == 0 ? hz (20.0 * std::pow (1000.0, v)) : pct();
        default: return pct();
    }
}

//==============================================================================
void Fx::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    dl.assign ((size_t) (sr * 2.5) + 4, 0.0f);
    dr.assign ((size_t) (sr * 2.5) + 4, 0.0f);
    reverb.prepare ({ sampleRate, (juce::uint32) juce::jmax (1, maxBlock), 2 });
    for (auto& band : eq) for (auto& f : band) f.reset();
    reset();
}

void Fx::reset()
{
    std::fill (dl.begin(), dl.end(), 0.0f);
    std::fill (dr.begin(), dr.end(), 0.0f);
    wpos = 0; ph = ph2 = 0.0;
    for (auto& s : st) s = 0.0f;
    for (auto& a : ap) for (auto& s : a) s = 0.0f;
    hold[0] = hold[1] = 0.0f; holdAcc = 0.0f;
    reverb.reset();
    for (auto& band : eq) for (auto& f : band) f.reset();
    for (auto& e : eqLast) e = -1.0f;
}

float Fx::readDelay (const std::vector<float>& buf, double back) const
{
    const auto size = (int) buf.size();
    back = juce::jlimit (1.0, (double) size - 2.0, back);
    double rp = (double) wpos - back;
    while (rp < 0) rp += size;
    const int i0 = (int) rp; const float f = (float) (rp - i0);
    const int i1 = (i0 + 1) % size;
    return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * f;
}

void Fx::process (float* L, float* R, int num, const FxParams& fp, double bpm, float p1Mod, float mixMod)
{
    if (fp.type == fxOff || num <= 0 || dl.empty()) return;
    if (fp.type != lastType) { reset(); lastType = fp.type; }
    const float p0 = juce::jlimit (0.0f, 1.0f, fp.p[0] + p1Mod), p1 = fp.p[1], p2 = fp.p[2];
    const float mix = juce::jlimit (0.0f, 1.0f, fp.mix + mixMod);
    const auto size = (int) dl.size();
    if (R == nullptr) R = L;

    switch (fp.type)
    {
        case fxBite:
        {
            const float levels = std::pow (2.0f, 2.0f + p0 * 14.0f) * 0.5f;
            const float rate = 1000.0f * std::pow (44.1f, p1);
            const bool crushBits = p0 < 0.97f, crushRate = rate < (float) sr * 0.85f;
            if (! crushBits && ! crushRate) break;   // 16 bit at full rate = clean: no stair-step artefacts at all
            for (int i = 0; i < num; ++i)
            {
                if (! crushRate)
                {
                    const float a = std::round (L[i] * levels) / levels, c = std::round (R[i] * levels) / levels;
                    L[i] += (a - L[i]) * mix;
                    if (R != L) R[i] += (c - R[i]) * mix;
                    continue;
                }
                holdAcc += rate / (float) sr * (1.0f + p2 * (rng.nextFloat() - 0.5f));
                if (holdAcc >= 1.0f)
                {
                    holdAcc -= std::floor (holdAcc);
                    hold[0] = crushBits ? std::round (L[i] * levels) / levels : L[i];
                    hold[1] = crushBits ? std::round (R[i] * levels) / levels : R[i];
                }
                L[i] += (hold[0] - L[i]) * mix;
                if (R != L) R[i] += (hold[1] - R[i]) * mix;
            }
            break;
        }
        case fxDrive:
        {
            const float g = std::pow (40.0f, p0);
            const int mode = juce::jlimit (0, 4, (int) (p1 * 4.99f));
            const float fc = 1000.0f * std::pow (18.0f, p2);
            auto shape = [mode] (float x)
            {
                switch (mode)
                {
                    case 1: return juce::jlimit (-1.0f, 1.0f, x);
                    case 2: return std::tanh (x + 0.3f * std::abs (x)) * 0.9f;
                    case 3: return x / (1.0f + std::abs (x)) * 1.4f;
                    case 4: return std::sin (x * 1.5f);
                    default: return std::tanh (x);
                }
            };
            for (int i = 0; i < num; ++i)
            {
                float yl = shape (L[i] * g), yr = shape (R[i] * g);
                yl = svf (yl, fc, 0.1f, sr, st[0], st[1], 0);
                if (R != L) yr = svf (yr, fc, 0.1f, sr, st[2], st[3], 0);
                const float comp = 1.0f / std::sqrt (g);
                L[i] += (yl * juce::jmax (comp, 0.35f) - L[i]) * mix;
                if (R != L) R[i] += (yr * juce::jmax (comp, 0.35f) - R[i]) * mix;
            }
            break;
        }
        case fxChorus: case fxFlanger:
        {
            const bool fl = fp.type == fxFlanger;
            const double rate = 0.05 * std::pow (100.0, (double) p0);
            const double base = (fl ? 0.001 : 0.012) * sr, depth = (fl ? 0.005 : 0.008) * sr * p1;
            const float fb = fl ? (p2 * 0.9f) : 0.0f;
            const double spread = fl ? 0.25 : 0.25 + 0.25 * p2;
            for (int i = 0; i < num; ++i)
            {
                ph = std::fmod (ph + rate / sr, 1.0);
                const float a = readDelay (dl, base + depth * (0.5 + 0.5 * std::sin (twoPi * ph)));
                const float b = readDelay (dr, base + depth * (0.5 + 0.5 * std::sin (twoPi * (ph + spread))));
                dl[(size_t) wpos] = L[i] + a * fb;
                dr[(size_t) wpos] = R[i] + b * fb;
                wpos = (wpos + 1) % size;
                L[i] = L[i] * (1.0f - mix * 0.5f) + a * mix * 0.7f;
                if (R != L) R[i] = R[i] * (1.0f - mix * 0.5f) + b * mix * 0.7f;
            }
            break;
        }
        case fxPhaser:
        {
            const double rate = 0.05 * std::pow (100.0, (double) p0);
            for (int i = 0; i < num; ++i)
            {
                ph = std::fmod (ph + rate / sr, 1.0);
                for (int c = 0; c < (R != L ? 2 : 1); ++c)
                {
                    const double lfo = 0.5 + 0.5 * std::sin (twoPi * (ph + c * 0.25));
                    const double fc = 200.0 * std::pow (20.0, lfo * p1);
                    const float t = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin (fc, sr * 0.45) / sr);
                    const float coef = (t - 1.0f) / (t + 1.0f);
                    float* x = c == 0 ? &L[i] : &R[i];
                    float y = *x + st[4 + c] * p2 * 0.85f;
                    for (int k = 0; k < 6; ++k)
                    {
                        const float out = coef * y + ap[c][k];
                        ap[c][k] = y - coef * out;
                        y = out;
                    }
                    st[4 + c] = y;
                    *x = *x * (1.0f - mix * 0.5f) + y * mix * 0.5f;
                }
            }
            break;
        }
        case fxDelay: case fxConsoleEcho:
        {
            const bool echo = fp.type == fxConsoleEcho;
            const double time = echo ? 0.016 * juce::jlimit (1, 15, 1 + (int) (p0 * 14.99f))
                                     : 60.0 / juce::jmax (30.0, bpm) * divBeats[divIndex (p0)];
            const double back = juce::jlimit (1.0, (double) size - 4.0, time * sr);
            const float fb = p1 * 0.95f;
            const int mode = echo ? 0 : juce::jlimit (0, 2, (int) (p2 * 2.99f));
            const float* fir = firs[juce::jlimit (0, 7, (int) (p2 * 7.99f))];
            for (int i = 0; i < num; ++i)
            {
                double b = back;
                if (mode == 2) { ph = std::fmod (ph + 0.6 / sr, 1.0); b *= 1.0 + 0.004 * std::sin (twoPi * ph); }
                float a = readDelay (dl, b), c = readDelay (dr, b);
                if (echo)
                {
                    // FIR in the feedback path (8 taps, one sample apart at 32 kHz ~ 1.4 samples here)
                    float fa = 0, fc = 0;
                    for (int k = 0; k < 8; ++k) { fa += fir[k] * readDelay (dl, b + k * sr / 32000.0); fc += fir[k] * readDelay (dr, b + k * sr / 32000.0); }
                    a = fa; c = fc;
                }
                if (mode == 2) { st[0] += (a - st[0]) * 0.25f; st[1] += (c - st[1]) * 0.25f; a = std::tanh (st[0] * 1.2f); c = std::tanh (st[1] * 1.2f); }
                if (mode == 1)
                {
                    dl[(size_t) wpos] = (L[i] + R[i]) * 0.5f + c * fb;
                    dr[(size_t) wpos] = a * fb;
                }
                else
                {
                    dl[(size_t) wpos] = L[i] + a * fb;
                    dr[(size_t) wpos] = R[i] + c * fb;
                }
                wpos = (wpos + 1) % size;
                L[i] += a * mix;
                if (R != L) R[i] += c * mix;
            }
            break;
        }
        case fxReverb:
        {
            juce::dsp::Reverb::Parameters rp;
            rp.roomSize = 0.3f + 0.6f * p0; rp.damping = p1; rp.width = p2;
            rp.wetLevel = mix * 0.3f; rp.dryLevel = 1.0f - mix * 0.4f;
            reverb.setParameters (rp);
            if (R != L) { float* ch[2] = { L, R }; juce::dsp::AudioBlock<float> blk (ch, 2, (size_t) num); reverb.process (juce::dsp::ProcessContextReplacing<float> (blk)); }
            else { float* ch[1] = { L }; juce::dsp::AudioBlock<float> blk (ch, 1, (size_t) num); reverb.process (juce::dsp::ProcessContextReplacing<float> (blk)); }
            break;
        }
        case fxWowFlutter:
        {
            const float hiss = p2 * p2 * 0.03f;
            for (int i = 0; i < num; ++i)
            {
                ph = std::fmod (ph + 0.55 / sr, 1.0);
                ph2 = std::fmod (ph2 + 7.3 / sr, 1.0);
                const double back = 0.008 * sr + p0 * 0.006 * sr * (0.5 + 0.5 * std::sin (twoPi * ph)) + p1 * 0.0006 * sr * std::sin (twoPi * ph2);
                dl[(size_t) wpos] = L[i]; dr[(size_t) wpos] = R[i];
                wpos = (wpos + 1) % size;
                st[0] += ((rng.nextFloat() * 2.0f - 1.0f) - st[0]) * 0.3f;
                const float a = readDelay (dl, back) + st[0] * hiss, c = readDelay (dr, back) + st[0] * hiss;
                L[i] += (a - L[i]) * mix;
                if (R != L) R[i] += (c - R[i]) * mix;
            }
            break;
        }
        case fxEq:
        {
            const float gains[3] = { (fp.p[0] + p1Mod - 0.5f) * 24.0f, (p1 - 0.5f) * 24.0f, (p2 - 0.5f) * 24.0f };
            for (int b = 0; b < 3; ++b)
                if (std::abs (gains[b] - eqLast[b]) > 0.05f)
                {
                    eqLast[b] = gains[b];
                    const float g = juce::Decibels::decibelsToGain (gains[b]);
                    auto coef = b == 0 ? juce::dsp::IIR::Coefficients<float>::makeLowShelf (sr, 120.0, 0.7, g)
                              : b == 1 ? juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, 1200.0, 0.8, g)
                                       : juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 6000.0, 0.7, g);
                    eq[b][0].coefficients = coef; eq[b][1].coefficients = coef;
                }
            for (int i = 0; i < num; ++i)
            {
                float a = L[i], c = R[i];
                for (int b = 0; b < 3; ++b) { a = eq[b][0].processSample (a); if (R != L) c = eq[b][1].processSample (c); }
                L[i] += (a - L[i]) * mix;
                if (R != L) R[i] += (c - R[i]) * mix;
            }
            break;
        }
        case fxCompressor:
        {
            const float thr = -40.0f + p0 * 40.0f, ratio = 1.0f + p1 * 19.0f;
            const float att = (float) std::exp (-1.0 / ((0.0005 + 0.03 * (1.0 - p2)) * sr));
            const float rel = (float) std::exp (-1.0 / ((0.05 + 0.45 * (1.0 - p2)) * sr));
            const float makeup = juce::Decibels::decibelsToGain (-thr * (1.0f - 1.0f / ratio) * 0.3f);
            for (int i = 0; i < num; ++i)
            {
                const float lvl = juce::jmax (std::abs (L[i]), std::abs (R[i]));
                st[0] = lvl > st[0] ? att * st[0] + (1 - att) * lvl : rel * st[0] + (1 - rel) * lvl;
                const float dbv = juce::Decibels::gainToDecibels (st[0] + 1.0e-6f);
                const float gr = dbv > thr ? (thr + (dbv - thr) / ratio) - dbv : 0.0f;
                const float g = juce::Decibels::decibelsToGain (gr) * makeup;
                L[i] += (L[i] * g - L[i]) * mix;
                if (R != L) R[i] += (R[i] * g - R[i]) * mix;
            }
            break;
        }
        case fxRingMod:
        {
            const double f = 20.0 * std::pow (250.0, (double) p0);
            for (int i = 0; i < num; ++i)
            {
                ph = std::fmod (ph + f / sr, 1.0);
                ph2 = std::fmod (ph2 + f * (1.0 + p2 * 0.05) / sr, 1.0);
                auto osc = [p1] (double x) { const float s = (float) std::sin (twoPi * x); return s * (1.0f - p1) + (s >= 0 ? 1.0f : -1.0f) * p1; };
                L[i] += (L[i] * osc (ph) - L[i]) * mix;
                if (R != L) R[i] += (R[i] * osc (ph2) - R[i]) * mix;
            }
            break;
        }
        case fxTremolo:
        {
            const double f = 0.1 * std::pow (200.0, (double) p0);
            const bool pan = p2 >= 0.5f;
            for (int i = 0; i < num; ++i)
            {
                ph = std::fmod (ph + f / sr, 1.0);
                const float s = (float) std::sin (twoPi * ph);
                if (pan && R != L)
                {
                    const float gl = 1.0f - p1 * mix * (0.5f + 0.5f * s), gr = 1.0f - p1 * mix * (0.5f - 0.5f * s);
                    L[i] *= gl; R[i] *= gr;
                }
                else
                {
                    const float g = 1.0f - p1 * mix * (0.5f + 0.5f * s);
                    L[i] *= g; if (R != L) R[i] *= g;
                }
            }
            break;
        }
        case fxWidth:
        {
            if (R == L) break;
            const float w = p0 * 2.0f;
            const double haas = p1 * 0.02 * sr;
            for (int i = 0; i < num; ++i)
            {
                dr[(size_t) wpos] = R[i];
                const float rr = haas > 1.0 ? readDelay (dr, haas) : R[i];
                wpos = (wpos + 1) % size;
                const float m = (L[i] + rr) * 0.5f;
                float s = (L[i] - rr) * 0.5f * w;
                st[0] += (s - st[0]) * (float) (1.0 - std::exp (-twoPi * 200.0 / sr));
                s -= st[0] * p2;   // bass mono: remove low sides
                L[i] += ((m + s) - L[i]) * mix;
                R[i] += ((m - s) - R[i]) * mix;
            }
            break;
        }
        case fxRadio:
        {
            const float fc = 300.0f * std::pow (10.0f, p0);
            for (int i = 0; i < num; ++i)
            {
                const float n = (rng.nextFloat() * 2.0f - 1.0f) * p1 * 0.05f;
                const float crackle = (rng.nextFloat() < p2 * 0.0008f) ? (rng.nextFloat() - 0.5f) * 0.6f : 0.0f;
                float a = svf (L[i] + n, fc, 0.55f, sr, st[0], st[1], 1) * 2.0f + crackle;
                float c = svf (R[i] + n, fc, 0.55f, sr, st[2], st[3], 1) * 2.0f + crackle;
                a = std::tanh (a * 1.5f); c = std::tanh (c * 1.5f);
                L[i] += (a - L[i]) * mix;
                if (R != L) R[i] += (c - R[i]) * mix;
            }
            break;
        }
        case fxLimiter:
        {
            const float ceil = juce::Decibels::decibelsToGain (-24.0f + p0 * 24.0f);
            const float drive = juce::Decibels::decibelsToGain (p2 * 18.0f);
            const float rel = (float) std::exp (-1.0 / ((0.01 + p1 * 0.49) * sr));
            if (st[0] <= 0.0f) st[0] = 1.0f;
            for (int i = 0; i < num; ++i)
            {
                const float a = L[i] * drive, c = R[i] * drive;
                const float pk = juce::jmax (std::abs (a), std::abs (c));
                const float want = pk > ceil ? ceil / pk : 1.0f;
                st[0] = want < st[0] ? want : rel * st[0] + (1 - rel) * want;
                L[i] += (juce::jlimit (-ceil, ceil, a * st[0]) - L[i]) * mix;
                if (R != L) R[i] += (juce::jlimit (-ceil, ceil, c * st[0]) - R[i]) * mix;
            }
            break;
        }
        case fxFilter:
        {
            const float fc = 20.0f * std::pow (1000.0f, p0);
            const int mode = juce::jlimit (0, 2, (int) (p2 * 2.99f));
            for (int i = 0; i < num; ++i)
            {
                const float a = svf (L[i], fc, p1, sr, st[0], st[1], mode);
                L[i] += (a - L[i]) * mix;
                if (R != L) { const float c = svf (R[i], fc, p1, sr, st[2], st[3], mode); R[i] += (c - R[i]) * mix; }
            }
            break;
        }
        default: break;
    }
}
} // namespace bitera
