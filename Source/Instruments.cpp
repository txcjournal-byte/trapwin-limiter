#include "Instruments.h"

namespace bitera
{
namespace
{
constexpr double SR = 32000.0;
constexpr double twoPi = 6.283185307179586;
double mtof (double m) { return 440.0 * std::pow (2.0, (m - 69.0) / 12.0); }

struct Rng { juce::uint32 s = 12345; float next() { s = s * 1664525u + 1013904223u; return (float) ((s >> 8) & 0xffff) / 32768.0f - 1.0f; } };

void finish (chips::Sample& smp, std::vector<float> x, const char* name, int root, int loopStart = -1, int loopEnd = -1)
{
    float pk = 0.0f;
    for (auto v : x) pk = juce::jmax (pk, std::abs (v));
    if (pk > 0.0f) for (auto& v : x) v *= 0.85f / pk;
    smp.data = std::move (x);
    smp.sampleRate = SR;
    smp.root = root;
    smp.name = name;
    smp.loopStart = loopStart; smp.loopEnd = loopEnd;
    smp.blockPeak.assign ((smp.data.size() + 15) / 16, 0.0f);
    for (size_t k = 0; k < smp.data.size(); ++k) smp.blockPeak[k / 16] = juce::jmax (smp.blockPeak[k / 16], std::abs (smp.data[k]));
}

// a loop length (in samples) close to 0.25 s that holds a whole number of cycles of f0
std::pair<int, double> loopFor (double f0)
{
    int bestL = 8000; double bestErr = 1e9, bestF = f0;
    for (int L = 6000; L <= 10000; ++L)
    {
        const double k = std::round (f0 * L / SR);
        const double f = k * SR / L;
        const double err = std::abs (std::log2 (f / f0));
        if (err < bestErr) { bestErr = err; bestL = L; bestF = f; }
    }
    return { bestL, bestF };
}

// sustained instrument: harmonic amplitudes amps[h-1], an attack that settles before the loop, seamless loop
void sustained (chips::Sample& smp, const char* name, int root, const std::vector<double>& amps, double attack, double breath = 0.0,
                double brightAttack = 0.0)
{
    const auto [L, f] = loopFor (mtof (root));
    const int attackN = (int) (SR * juce::jmax (0.05, attack + 0.05));
    const int total = attackN + 2 * L;
    std::vector<float> x ((size_t) total, 0.0f);
    Rng rng;
    float nlp = 0.0f;
    for (int i = 0; i < total; ++i)
    {
        const double t = i / SR;
        const double env = juce::jmin (1.0, t / juce::jmax (0.001, attack));
        double v = 0.0;
        for (size_t h = 0; h < amps.size(); ++h)
        {
            const double hf = f * (double) (h + 1);
            if (hf > SR * 0.45) break;
            // brighter attack: upper partials decay to their steady level before the loop
            const double extra = brightAttack > 0.0 && i < attackN ? 1.0 + brightAttack * (h / 8.0) * (1.0 - (double) i / attackN) : 1.0;
            v += amps[h] * extra * std::sin (twoPi * hf * t);
        }
        if (breath > 0.0 && i < attackN) { nlp += (rng.next() - nlp) * 0.3f; v += breath * nlp * (1.0 - (double) i / attackN) * 3.0; }
        x[(size_t) i] = (float) (v * env);
    }
    finish (smp, std::move (x), name, root, attackN, attackN + L);
}

std::vector<double> formantHarmonics (double f0, int n, std::initializer_list<std::pair<double, double>> formants, double tilt)
{
    std::vector<double> a ((size_t) n);
    for (int h = 1; h <= n; ++h)
    {
        const double hf = f0 * h;
        double g = 0.08;
        for (auto [ff, bw] : formants) g += std::exp (-std::pow ((hf - ff) / bw, 2.0));
        a[(size_t) h - 1] = g / std::pow ((double) h, tilt);
    }
    return a;
}

// struck / plucked: inharmonic modes with their own decays
void modal (chips::Sample& smp, const char* name, int root, std::initializer_list<std::array<double, 3>> modes, double dur, double noiseClick = 0.0)
{
    const int n = (int) (SR * dur);
    std::vector<float> x ((size_t) n, 0.0f);
    const double f0 = mtof (root);
    Rng rng;
    for (int i = 0; i < n; ++i)
    {
        const double t = i / SR;
        double v = 0.0;
        for (auto& m : modes)
            if (f0 * m[0] < SR * 0.45) v += m[1] * std::exp (-t * m[2]) * std::sin (twoPi * f0 * m[0] * t);
        if (noiseClick > 0.0) v += noiseClick * rng.next() * std::exp (-t * 300.0);
        x[(size_t) i] = (float) (v * juce::jmin (1.0, t * 2000.0));
    }
    finish (smp, std::move (x), name, root);
}

// Karplus-Strong plucked string
void plucked (chips::Sample& smp, const char* name, int root, double dur, double bright, double damp)
{
    const double f0 = mtof (root);
    const int n = (int) (SR * dur);
    // the two-point average adds half a sample of delay: the line holds period - 0.5 samples
    const int N = juce::jmax (2, (int) std::lround (SR / f0 - 0.5));
    std::vector<float> line ((size_t) N);
    Rng rng;
    float lp = 0.0f;
    for (auto& v : line) { lp += (rng.next() - lp) * (float) bright; v = lp; }
    std::vector<float> x ((size_t) n);
    size_t idx = 0;
    float prev = line.back();
    for (int i = 0; i < n; ++i)
    {
        const float cur = line[idx];
        const float out = (float) ((cur + prev) * 0.5 * damp);
        prev = cur;
        line[idx] = out;
        idx = (idx + 1) % (size_t) N;
        x[(size_t) i] = out;
    }
    finish (smp, std::move (x), name, root);
}

void fmOneShot (chips::Sample& smp, const char* name, int root, double ratio, double index, double idxDecay, double ampDecay, double dur,
                double tineRatio = 0.0, double tineLevel = 0.0)
{
    const double f0 = mtof (root);
    const int n = (int) (SR * dur);
    std::vector<float> x ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        const double t = i / SR;
        const double mod = std::sin (twoPi * f0 * ratio * t) * index * std::exp (-t * idxDecay);
        double v = std::sin (twoPi * f0 * t + mod) * std::exp (-t * ampDecay);
        if (tineLevel > 0.0) v += tineLevel * std::sin (twoPi * f0 * tineRatio * t) * std::exp (-t * 14.0);
        x[(size_t) i] = (float) (v * juce::jmin (1.0, t * 1500.0));
    }
    finish (smp, std::move (x), name, root);
}

void drum (chips::Sample& smp, int kind)
{
    static const char* names[] = { "Kick", "Snare", "Closed Hat", "Open Hat", "Crash", "Tom", "Clap", "Rim" };
    const double durs[] = { 0.6, 0.35, 0.12, 0.6, 1.8, 0.5, 0.35, 0.08 };
    const int n = (int) (SR * durs[kind]);
    std::vector<float> x ((size_t) n);
    Rng rng;
    double ph = 0.0;
    float hp = 0.0f, bp1 = 0.0f, bp2 = 0.0f;
    static const double metal[6] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
    double mph[6] {};
    for (int i = 0; i < n; ++i)
    {
        const double t = i / SR;
        double v = 0.0;
        switch (kind)
        {
            case 0: { const double f = 45.0 + 110.0 * std::exp (-t * 30.0); ph += f / SR; v = std::sin (twoPi * ph) * std::exp (-t * 6.0) + 0.3 * rng.next() * std::exp (-t * 400.0); break; }
            case 1: { ph += 190.0 * (1.0 + 0.3 * std::exp (-t * 40.0)) / SR; const float nz = rng.next(); hp += (nz - hp) * 0.25f;
                      v = 0.5 * std::sin (twoPi * ph) * std::exp (-t * 25.0) + 0.8 * (nz - hp) * std::exp (-t * 14.0); break; }
            case 2: case 3: case 4:
            {
                double m = 0.0;
                for (int k = 0; k < 6; ++k) { mph[k] += metal[k] * (kind == 4 ? 1.7 : 2.0) / SR; m += std::fmod (mph[k], 1.0) < 0.5 ? 1.0 : -1.0; }
                const float nz = rng.next();
                const double sig = m / 6.0 * 0.6 + nz * (kind == 4 ? 0.7 : 0.4);
                hp += ((float) sig - hp) * 0.55f;
                const double dec = kind == 2 ? 45.0 : kind == 3 ? 6.0 : 1.8;
                v = ((float) sig - hp) * std::exp (-t * dec);
                break;
            }
            case 5: { const double f = 110.0 + 60.0 * std::exp (-t * 15.0); ph += f / SR; v = std::sin (twoPi * ph) * std::exp (-t * 8.0) + 0.15 * rng.next() * std::exp (-t * 60.0); break; }
            case 6:
            {
                const float nz = rng.next();
                bp1 += (nz - bp1) * 0.35f; bp2 += (bp1 - bp2) * 0.35f;
                const double bursts = (t < 0.011 ? 1.0 : 0.0) + (t > 0.012 && t < 0.022 ? 0.9 : 0.0) + (t > 0.024 ? std::exp (-(t - 0.024) * 18.0) : 0.0);
                v = (bp1 - bp2) * 3.0 * bursts;
                break;
            }
            default: { ph += 1700.0 / SR; v = (std::sin (twoPi * ph) + 0.5 * rng.next()) * std::exp (-t * 70.0); break; }
        }
        x[(size_t) i] = (float) v;
    }
    finish (smp, std::move (x), names[kind], 60);
}

std::vector<double> sawAmps (int n, double tilt) { std::vector<double> a; for (int h = 1; h <= n; ++h) a.push_back (1.0 / std::pow (h, tilt)); return a; }
} // namespace

const char* instrumentName (int i)
{
    static const char* n[] = { "Grand Piano", "E.Piano", "Strings", "Choir Aah", "Choir Ooh", "Brass", "Trumpet", "Flute", "Clarinet", "Sax",
                               "Organ", "Finger Bass", "Slap Bass", "Nylon Guitar", "Steel Guitar", "Drive Guitar", "Harp", "Pizzicato",
                               "Marimba", "Vibraphone", "Glockenspiel", "Music Box", "Steel Drum", "Orchestra Hit", "Timpani", "Kick", "Snare",
                               "Closed Hat", "Open Hat", "Crash", "Tom", "Clap", "Rim", "Square Lead", "Saw Lead", "Sine Pad" };
    return n[juce::jlimit (0, numInstruments - 1, i)];
}

void addInstruments (chips::Bank& bank)
{
    const size_t base = bank.size();
    bank.resize (base + (size_t) numInstruments);
    auto at = [&] (int i) -> chips::Sample& { return bank[base + (size_t) i]; };

    // piano: stiff-string partials, each decaying at its own rate, plus the hammer
    {
        const double f0 = mtof (60), B = 0.0004;
        const int n = (int) (SR * 3.0);
        std::vector<float> x ((size_t) n);
        Rng rng;
        for (int i = 0; i < n; ++i)
        {
            const double t = i / SR;
            double v = 0.0;
            for (int h = 1; h <= 14; ++h)
            {
                const double hf = f0 * h * std::sqrt (1.0 + B * h * h);
                v += (1.0 / h) * std::exp (-t * (0.8 + 0.45 * h)) * (std::sin (twoPi * hf * t) + 0.6 * std::sin (twoPi * hf * 1.0012 * t));
            }
            v += 0.2 * rng.next() * std::exp (-t * 200.0);
            x[(size_t) i] = (float) (v * juce::jmin (1.0, t * 3000.0));
        }
        finish (at (insPiano), std::move (x), instrumentName (insPiano), 60);
    }
    fmOneShot (at (insEPiano), instrumentName (insEPiano), 60, 1.0, 1.6, 3.5, 1.2, 2.8, 14.0, 0.25);
    sustained (at (insStrings), instrumentName (insStrings), 60, formantHarmonics (mtof (60), 30, { { 600, 500 }, { 2500, 900 } }, 0.9), 0.25);
    sustained (at (insChoirAah), instrumentName (insChoirAah), 60, formantHarmonics (mtof (60), 30, { { 730, 120 }, { 1090, 140 }, { 2440, 220 } }, 0.6), 0.3, 0.05);
    sustained (at (insChoirOoh), instrumentName (insChoirOoh), 60, formantHarmonics (mtof (60), 30, { { 300, 90 }, { 870, 120 }, { 2240, 220 } }, 0.7), 0.3, 0.04);
    sustained (at (insBrass), instrumentName (insBrass), 60, formantHarmonics (mtof (60), 30, { { 1100, 700 } }, 0.7), 0.06, 0.0, 1.5);
    sustained (at (insTrumpet), instrumentName (insTrumpet), 72, formantHarmonics (mtof (72), 20, { { 1500, 800 } }, 0.5), 0.04, 0.02, 1.2);
    sustained (at (insFlute), instrumentName (insFlute), 72, { 1.0, 0.22, 0.08, 0.03 }, 0.08, 0.12);
    sustained (at (insClarinet), instrumentName (insClarinet), 60, { 1.0, 0.03, 0.55, 0.02, 0.35, 0.02, 0.2, 0.01, 0.12, 0.01, 0.07 }, 0.05, 0.03);
    sustained (at (insSax), instrumentName (insSax), 60, formantHarmonics (mtof (60), 20, { { 500, 250 }, { 1500, 500 } }, 0.5), 0.05, 0.05, 0.8);
    sustained (at (insOrgan), instrumentName (insOrgan), 60, { 1.0, 0.8, 0.5, 0.6, 0.0, 0.35, 0.0, 0.3 }, 0.01);
    plucked (at (insFingerBass), instrumentName (insFingerBass), 36, 2.5, 0.25, 0.9995);
    fmOneShot (at (insSlapBass), instrumentName (insSlapBass), 36, 1.0, 3.0, 9.0, 2.0, 1.6);
    plucked (at (insNylonGuitar), instrumentName (insNylonGuitar), 48, 2.5, 0.35, 0.9993);
    plucked (at (insSteelGuitar), instrumentName (insSteelGuitar), 48, 2.5, 0.75, 0.9995);
    {
        // drive guitar: a saw through a hard clipper, steady and looped
        std::vector<double> a = sawAmps (24, 0.6);
        sustained (at (insDriveGuitar), instrumentName (insDriveGuitar), 48, a, 0.01, 0.0, 1.0);
        for (auto& v : at (insDriveGuitar).data) v = (float) (std::tanh (v * 4.0) * 0.8);
    }
    plucked (at (insHarp), instrumentName (insHarp), 60, 2.5, 0.6, 0.9996);
    plucked (at (insPizzicato), instrumentName (insPizzicato), 60, 0.7, 0.4, 0.998);
    modal (at (insMarimba), instrumentName (insMarimba), 60, { { 1.0, 1.0, 5.0 }, { 3.93, 0.35, 18.0 }, { 9.2, 0.12, 40.0 } }, 1.4, 0.1);
    modal (at (insVibraphone), instrumentName (insVibraphone), 60, { { 1.0, 1.0, 1.4 }, { 4.0, 0.3, 5.0 }, { 10.0, 0.1, 12.0 } }, 3.0);
    modal (at (insGlock), instrumentName (insGlock), 72, { { 1.0, 1.0, 2.5 }, { 2.71, 0.4, 7.0 }, { 5.15, 0.2, 12.0 }, { 8.1, 0.1, 20.0 } }, 2.0, 0.05);
    modal (at (insMusicBox), instrumentName (insMusicBox), 72, { { 1.0, 1.0, 2.0 }, { 3.0, 0.25, 9.0 }, { 5.4, 0.15, 14.0 } }, 2.0, 0.08);
    modal (at (insSteelDrum), instrumentName (insSteelDrum), 60, { { 1.0, 1.0, 3.0 }, { 2.0, 0.6, 4.0 }, { 3.01, 0.35, 6.0 }, { 4.2, 0.15, 9.0 } }, 1.6);
    {
        // orchestra hit: brass + strings chord with a noise smack
        const int n = (int) (SR * 1.0);
        std::vector<float> x ((size_t) n);
        Rng rng;
        const int chord[] = { 48, 55, 60, 64, 67, 72 };
        for (int i = 0; i < n; ++i)
        {
            const double t = i / SR;
            double v = 0.0;
            for (int m : chord) for (int h = 1; h <= 10; ++h) v += std::sin (twoPi * mtof (m) * h * t) / h * (h > 4 ? std::exp (-t * 6.0) : 1.0);
            v = v * std::exp (-t * 4.5) * 0.3 + 0.5 * rng.next() * std::exp (-t * 30.0);
            x[(size_t) i] = (float) (v * juce::jmin (1.0, t * 2000.0));
        }
        finish (at (insOrchHit), std::move (x), instrumentName (insOrchHit), 60);
    }
    modal (at (insTimpani), instrumentName (insTimpani), 48, { { 1.0, 1.0, 2.0 }, { 1.5, 0.5, 3.0 }, { 1.99, 0.35, 4.0 }, { 2.44, 0.2, 6.0 } }, 2.0, 0.3);
    for (int k = 0; k < 8; ++k) drum (at (insKick + k), k);
    {
        std::vector<double> sq; for (int h = 1; h <= 30; ++h) sq.push_back (h % 2 ? 1.0 / h : 0.0);
        sustained (at (insSquareLead), instrumentName (insSquareLead), 60, sq, 0.01);
    }
    sustained (at (insSawLead), instrumentName (insSawLead), 60, sawAmps (30, 1.0), 0.01);
    sustained (at (insSinePad), instrumentName (insSinePad), 60, { 1.0, 0.3, 0.1, 0.05 }, 0.4);
}
} // namespace bitera
