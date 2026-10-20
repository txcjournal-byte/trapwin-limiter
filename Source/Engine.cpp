#include "Engine.h"

namespace bitera
{
namespace
{
constexpr double twoPi = 6.283185307179586;
double mtof (double m) { return 440.0 * std::pow (2.0, (m - 69.0) / 12.0); }

float stepVolume (float v, int steps, bool logSteps)
{
    if (steps <= 0) return v;
    if (! logSteps) return std::round (v * (float) steps) / (float) steps;
    if (v <= 1.0e-4f) return 0.0f;
    const int k = juce::jlimit (0, steps, (int) std::lround (steps + 20.0 * std::log10 (v) / 2.0));
    return k == 0 ? 0.0f : std::pow (10.0f, -2.0f * (float) (steps - k) / 20.0f);
}
} // namespace

const char* filterName (int type)
{
    static const char* n[] = { "OFF", "LP 12", "LP 24 LADDER", "HP 12", "BP 12", "NOTCH" };
    return n[juce::jlimit (0, numFilterTypes - 1, type)];
}

//==============================================================================
float Engine::Env::next (float a, float d, float s, float r, double rate)
{
    switch (stage)
    {
        case idle: return 0.0f;
        case attack:
            level += a <= 0.0005f ? 1.0f : (float) (1.0 / (a * rate));
            if (level >= 1.0f) { level = 1.0f; stage = decay; }
            break;
        case decay:
            level = s + (level - s) * (float) std::exp (-4.6 / (juce::jmax (0.001f, d) * rate));
            if (std::abs (level - s) < 1.0e-4f) { level = s; stage = sustain; }
            break;
        case sustain: level = s; break;
        case release:
            level *= (float) std::exp (-6.9 / (juce::jmax (0.001f, r) * rate));
            if (level < 1.0e-4f) { level = 0.0f; stage = idle; }
            break;
    }
    return level;
}

float Engine::Filter::process (float x, int type, float fc, float res, double rate)
{
    if (type == fOff) return x;
    fc = juce::jlimit (20.0f, (float) (rate * 0.45), fc);
    if (type == fLp24)
    {
        // 4-pole ladder with saturating feedback (the classic transistor-ladder character)
        const float g = (float) (1.0 - std::exp (-twoPi * fc / rate));
        const float k = res * 3.9f;
        float in = std::tanh (x - k * z[3]);
        for (auto& st : z) { st += g * (in - st); in = st; }
        return z[3] * (1.0f + k * 0.35f);
    }
    // zero-delay state variable filter
    const float g = (float) std::tan (juce::MathConstants<double>::pi * fc / rate);
    const float k = 2.0f - 1.95f * juce::jlimit (0.0f, 1.0f, res);
    const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
    const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
    switch (type)
    {
        case fLp12: return v2;
        case fHp12: return x - k * v1 - v2;
        case fBp12: return v1;
        default:    return x - k * v1;   // notch
    }
}

//==============================================================================
const char* modSourceName (int s)
{
    static const char* n[] = { "OFF", "LFO 1", "LFO 2", "LFO 3", "MOD ENV 1", "MOD ENV 2", "VELOCITY", "KEY", "MOD WHEEL", "AFTERTOUCH",
                               "PITCH BEND", "RANDOM", "MACRO 1", "MACRO 2", "MACRO 3", "MACRO 4", "MACRO 5", "MACRO 6", "MACRO 7", "MACRO 8",
                               "XY 1 X", "XY 1 Y", "XY 2 X", "XY 2 Y" };
    return n[juce::jlimit (0, numModSources - 1, s)];
}

const char* modDestName (int d)
{
    static const char* n[] = { "PITCH", "LEVEL", "PAN", "CUTOFF", "RESONANCE", "CHARACTER", "FILTER ENV", "VIBRATO",
                               "FX A CTRL 1", "FX A MIX", "FX B CTRL 1", "FX B MIX", "FX C CTRL 1", "FX C MIX",
                               "MASTER VOLUME", "TONE", "SPACE", "MASTER FX 1", "MASTER FX 2", "MASTER FX 3", "MASTER FX 4" };
    return n[juce::jlimit (0, numModDests - 1, d)];
}

bool modDestIsVoice (int d) { return d <= mdVibrato; }

const char* lfoShapeName (int s)
{
    static const char* n[] = { "SINE", "TRIANGLE", "SAW UP", "SAW DOWN", "SQUARE", "SAMPLE & HOLD", "SMOOTH RANDOM" };
    return n[juce::jlimit (0, numLfoShapes - 1, s)];
}

const juce::StringArray& syncNames()
{
    static const juce::StringArray n { "FREE", "4 BARS", "2 BARS", "1 BAR", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4 T", "1/8 T", "1/16 T" };
    return n;
}

double syncBeats (int sync)
{
    static const double b[] = { 0.0, 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
    return b[juce::jlimit (0, 11, sync)];
}

const char* arpDirName (int d)
{
    static const char* n[] = { "UP", "DOWN", "UP-DOWN", "DOWN-UP", "AS PLAYED", "RANDOM", "CHORD", "CONVERGE" };
    return n[juce::jlimit (0, numArpDirs - 1, d)];
}

//==============================================================================
Engine::Engine() = default;

void Engine::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    maxBlockSize = juce::jmax (1, maxBlock);
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlockSize, 2 };
    tone.prepare (spec);
    tone.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    tone.setResonance (0.707f);
    reverb.prepare (spec);
    gain.reset (sampleRate, 0.03);
    for (auto& l : layers)
    {
        l.buffer.setSize (2, maxBlockSize, false, true, true);
        for (auto& f : l.fx) f.prepare (sampleRate, maxBlockSize);
    }
    for (auto& f : masterFx) f.prepare (sampleRate, maxBlockSize);
    reset();
}

void Engine::reset()
{
    for (auto& l : layers)
    {
        for (auto& v : l.voices) v.active = false;
        l.held.clear(); l.arpHeld.clear();
        l.lastPitch = -1.0f; l.step = -1; l.arpRunning = false; l.arpFinished = false; l.uiStep = -1;
        for (auto& f : l.fx) f.reset();
    }
    for (auto& f : masterFx) f.reset();
    bend = wheel = aftertouch = 0.0f; sustainPedal = false;
    tone.reset(); reverb.reset();
}

bool Engine::isSilent() const
{
    for (auto& l : layers)
    {
        if (l.arpRunning) return false;
        for (auto& v : l.voices) if (v.active) return false;
    }
    return true;
}

//==============================================================================
float Engine::lfoValue (int shape, double ph, float sh, float prev) const
{
    switch (shape)
    {
        case lsTriangle: return (float) (1.0 - 4.0 * std::abs (ph - 0.5));
        case lsSawUp: return (float) (2.0 * ph - 1.0);
        case lsSawDown: return (float) (1.0 - 2.0 * ph);
        case lsSquare: return ph < 0.5 ? 1.0f : -1.0f;
        case lsSampleHold: return sh;
        case lsSmoothRandom: { const float f = (float) (ph * ph * (3.0 - 2.0 * ph)); return prev + (sh - prev) * f; }
        default: return (float) std::sin (twoPi * ph);
    }
}

void Engine::modSums (const EngineParams& p, const float* src, int layer, bool voiceDests, float* out) const
{
    for (int d = 0; d < numModDests; ++d) out[d] = 0.0f;
    for (const auto& s : p.slot)
    {
        if (s.src == msOff || std::abs (s.amount) < 1.0e-5f) continue;
        if (modDestIsVoice (s.dst) != voiceDests) continue;
        const bool masterDest = s.dst >= mdVolume;
        if (! masterDest && s.layer != 0 && s.layer != layer + 1) continue;
        out[s.dst] += s.amount * src[s.src];
    }
}

//==============================================================================
void Engine::startVoice (int li, Voice& v, int note, float vel, float fromPitch, const EngineParams& p, int stepIdx)
{
    const auto& lp = p.layer[li];
    v.note = note; v.velocity = vel; v.held = true; v.sustained = false;
    v.target = (float) note;
    v.pitch = (lp.glide > 0.0005f && fromPitch >= 0.0f) ? fromPitch : (float) note;
    v.age = ++layers[li].counter; v.t = 0.0; v.gateLeft = -1.0;
    const int wave = (stepIdx >= 0 && lp.stepWave[stepIdx] >= 0) ? lp.stepWave[stepIdx] : lp.wave;
    v.panOffset = stepIdx >= 0 ? lp.stepPan[stepIdx] : 0.0f;
    v.osc.start (lp.source, wave, sr, bank);
    v.amp = {}; v.flt = {};
    v.amp.gate (true); v.flt.gate (true);
    for (auto& e : v.menv) { e = {}; e.gate (true); }
    v.filter.reset();
    v.vel = 1.0f - lp.velSens * (1.0f - vel);
    v.random = v.rng.nextFloat() * 2.0f - 1.0f;
    for (int k = 0; k < numLfos; ++k)
    {
        v.lfoPh[k] = 0.0;
        v.lfoSH[k] = v.rng.nextFloat() * 2.0f - 1.0f;
        v.lfoPrev[k] = v.lfoSH[k];
    }
    v.active = true;
}

void Engine::releaseAll (int li)
{
    for (auto& v : layers[li].voices)
        if (v.active) { v.held = false; v.amp.gate (false); v.flt.gate (false); for (auto& e : v.menv) e.gate (false); }
}

void Engine::noteOn (int li, int note, float vel, const EngineParams& p, bool fromArp, int stepIdx)
{
    auto& L = layers[li];
    const auto& lp = p.layer[li];
    if (lp.mode != vmPoly)
    {
        L.held.erase (std::remove (L.held.begin(), L.held.end(), note), L.held.end());
        L.held.push_back (note);
        Voice* cur = nullptr;
        for (auto& v : L.voices) if (v.active && (cur == nullptr || v.age > cur->age)) cur = &v;
        if (cur != nullptr && cur->held && lp.mode == vmLegato && ! fromArp)
        {
            cur->note = note; cur->target = (float) note;
            if (lp.glide <= 0.0005f) cur->pitch = (float) note;
            L.lastPitch = (float) note;
            return;
        }
        const float from = cur != nullptr ? cur->pitch : L.lastPitch;
        for (auto& v : L.voices) if (&v != cur) v.active = false;
        startVoice (li, cur != nullptr ? *cur : L.voices[0], note, vel, from, p, stepIdx);
        L.lastPitch = (float) note;
        return;
    }
    Voice* slot = nullptr;
    for (auto& v : L.voices) if (! v.active) { slot = &v; break; }
    if (slot == nullptr)
    {
        for (auto& v : L.voices) if (! v.held && (slot == nullptr || v.age < slot->age)) slot = &v;
        if (slot == nullptr) for (auto& v : L.voices) if (slot == nullptr || v.age < slot->age) slot = &v;
    }
    startVoice (li, *slot, note, vel, L.lastPitch, p, stepIdx);
    slot->arpNote = fromArp;
    L.lastPitch = (float) note;
}

void Engine::noteOff (int li, int note, const EngineParams& p)
{
    auto& L = layers[li];
    const auto& lp = p.layer[li];
    if (lp.mode != vmPoly)
    {
        L.held.erase (std::remove (L.held.begin(), L.held.end(), note), L.held.end());
        for (auto& v : L.voices)
        {
            if (! v.active || ! v.held || v.note != note) continue;
            if (! L.held.empty() && lp.arpMode == amOff)
            {
                v.note = L.held.back(); v.target = (float) v.note;
                if (lp.glide <= 0.0005f) v.pitch = v.target;
                L.lastPitch = v.target;
            }
            else
            {
                v.held = false;
                v.amp.gate (false); v.flt.gate (false); for (auto& e : v.menv) e.gate (false);
            }
        }
        return;
    }
    for (auto& v : L.voices)
        if (v.active && v.held && v.note == note)
        {
            v.held = false;
            if (sustainPedal && ! v.arpNote) { v.sustained = true; continue; }
            v.amp.gate (false); v.flt.gate (false); for (auto& e : v.menv) e.gate (false);
        }
}

//==============================================================================
double Engine::stepSamples (const LayerParams& lp, const EngineParams& p, int step) const
{
    double beats = syncBeats (lp.arpSync);
    if (beats <= 0.0) beats = 0.25;
    double len = 60.0 / juce::jmax (20.0, p.bpm) * beats * sr;
    const double sw = juce::jlimit (0.0f, 0.9f, lp.arpSwing) * 0.5;
    return (step % 2 == 0) ? len * (1.0 + sw) : len * (1.0 - sw);
}

void Engine::arpStep (int li, const EngineParams& p)
{
    auto& L = layers[li];
    const auto& lp = p.layer[li];
    const int steps = juce::jlimit (1, maxSteps, lp.arpSteps);
    int next = L.step + 1;
    if (next >= steps)
    {
        if (lp.arpEnd == aeLoop) next = 0;
        else { L.arpFinished = true; L.uiStep = -1; return; }   // one-shot / hold: no new notes
    }
    L.step = next;
    L.uiStep = next;
    if (! lp.stepOn[next] || L.arpHeld.empty()) return;

    // the notes of this step
    std::vector<std::pair<int, float>> notes;
    auto sorted = L.arpHeld;
    std::sort (sorted.begin(), sorted.end(), [] (auto& a, auto& b) { return a.first < b.first; });
    if (lp.arpMode == amSeq)
    {
        for (auto& n : sorted) notes.push_back ({ n.first + lp.stepNote[next], n.second });
    }
    else
    {
        std::vector<std::pair<int, float>> pool;
        const int octs = juce::jlimit (1, 4, lp.arpOctaves);
        auto base = lp.arpDir == adPlayed ? L.arpHeld : sorted;
        for (int o = 0; o < octs; ++o) for (auto& n : base) pool.push_back ({ n.first + 12 * o, n.second });
        if (lp.arpDir == adChord) notes = pool;
        else
        {
            std::vector<std::pair<int, float>> seq;
            switch (lp.arpDir)
            {
                case adDown: seq.assign (pool.rbegin(), pool.rend()); break;
                case adUpDown: seq = pool; for (int i = (int) pool.size() - 2; i > 0; --i) seq.push_back (pool[(size_t) i]); break;
                case adDownUp: seq.assign (pool.rbegin(), pool.rend()); for (int i = 1; i < (int) pool.size() - 1; ++i) seq.push_back (pool[(size_t) i]); break;
                case adConverge:
                    for (int a = 0, b = (int) pool.size() - 1; a <= b; ++a, --b) { seq.push_back (pool[(size_t) a]); if (a != b) seq.push_back (pool[(size_t) b]); }
                    break;
                case adRandom: seq.push_back (pool[(size_t) L.rng.nextInt ((int) pool.size())]); break;
                default: seq = pool; break;
            }
            if (! seq.empty()) notes.push_back (seq[(size_t) (L.arpIndex++ % (int) seq.size())]);
        }
    }

    for (auto& n : notes)
    {
        const int note = juce::jlimit (0, 127, n.first);
        const float chordScale = notes.size() > 1 ? 1.0f / std::sqrt ((float) notes.size()) : 1.0f;
        const float vel = juce::jlimit (0.01f, 1.0f, n.second * lp.stepVel[next] * chordScale);
        noteOn (li, note, vel, p, true, next);
        // schedule the note-off by the gate of this step
        for (auto& v : L.voices)
            if (v.active && v.note == note && v.age == L.counter)
                v.gateLeft = juce::jmax (16.0, stepSamples (lp, p, next) * lp.arpGate * lp.stepGate[next] * 1.0);
    }
}

void Engine::renderLayer (int li, int start, int num, const EngineParams& p)
{
    auto& L = layers[li];
    const auto& lp = p.layer[li];
    if (lp.arpMode == amOff || ! lp.on)
    {
        if (L.arpRunning) { L.arpRunning = false; L.uiStep = -1; }
        renderVoices (li, start, num, p);
        return;
    }
    int pos = start;
    const int end = start + num;
    while (pos < end)
    {
        // next event: the next step, or the earliest arp note-off
        int chunk = end - pos;
        if (L.arpRunning && ! L.arpFinished)
            chunk = juce::jmin (chunk, juce::jmax (1, (int) std::ceil (stepSamples (lp, p, juce::jmax (0, L.step)) - L.stepPos)));
        for (auto& v : L.voices)
            if (v.active && v.held && v.gateLeft > 0.0) chunk = juce::jmin (chunk, juce::jmax (1, (int) std::ceil (v.gateLeft)));
        renderVoices (li, pos, chunk, p);
        pos += chunk;
        for (auto& v : L.voices)
            if (v.active && v.held && v.gateLeft > 0.0)
            {
                v.gateLeft -= chunk;
                if (v.gateLeft <= 0.5)
                {
                    // hold mode keeps the last note while keys are down
                    const bool keep = lp.arpEnd == aeHold && L.arpFinished && ! L.arpHeld.empty();
                    if (! keep) { v.held = false; v.amp.gate (false); v.flt.gate (false); for (auto& e : v.menv) e.gate (false); }
                    v.gateLeft = -1.0;
                }
            }
        if (L.arpRunning && ! L.arpFinished)
        {
            L.stepPos += chunk;
            const double len = stepSamples (lp, p, juce::jmax (0, L.step));
            if (L.stepPos >= len - 0.5)
            {
                L.stepPos -= len;
                if (L.stepPos < 0) L.stepPos = 0;
                arpStep (li, p);
            }
        }
    }
}

void Engine::renderVoices (int li, int start, int num, const EngineParams& p)
{
    if (num <= 0) return;
    auto& Lr = layers[li];
    const auto& lp = p.layer[li];
    auto* outL = Lr.buffer.getWritePointer (0, start);
    auto* outR = Lr.buffer.getWritePointer (1, start);
    const float glideK = lp.glide > 0.0005f ? (float) (1.0 - std::exp (-1.0 / (lp.glide * sr))) : 1.0f;
    const int steps = chips::volumeSteps (lp.source);
    const bool logSteps = chips::volumeLog (lp.source);
    constexpr int modChunk = 32;

    for (auto& v : Lr.voices)
    {
        if (! v.active) continue;
        for (int c0 = 0; c0 < num && v.active; c0 += modChunk)
        {
            const int cn = juce::jmin (modChunk, num - c0);
            // ---- modulation sources of this voice ----
            float src[numModSources];
            std::copy (std::begin (gSrc), std::end (gSrc), src);
            for (int k = 0; k < numLfos; ++k)
            {
                const auto& lf = p.lfo[k];
                if (lf.retrig)
                {
                    const double beats = syncBeats (lf.sync);
                    const double rate = beats > 0.0 ? p.bpm / 60.0 / beats : lf.rate;
                    v.lfoPh[k] += rate * cn / sr;
                    if (v.lfoPh[k] >= 1.0) { v.lfoPh[k] -= std::floor (v.lfoPh[k]); v.lfoPrev[k] = v.lfoSH[k]; v.lfoSH[k] = v.rng.nextFloat() * 2.0f - 1.0f; }
                    float val = lfoValue (lf.shape, v.lfoPh[k], v.lfoSH[k], v.lfoPrev[k]);
                    if (lf.unipolar) val = 0.5f + 0.5f * val;
                    src[msLfo1 + k] = val;
                }
                if (lf.fade > 0.001f) src[msLfo1 + k] *= (float) juce::jmin (1.0, v.t / lf.fade);
            }
            for (int k = 0; k < numModEnvs; ++k)
            {
                float e = 0.0f;
                for (int s = 0; s < cn; s += 8) e = v.menv[k].next (p.menv[k].a, p.menv[k].d, p.menv[k].s, p.menv[k].r, sr / 8.0);
                src[msEnv1 + k] = e;
            }
            src[msVelocity] = v.velocity;
            src[msKey] = ((float) v.note - 60.0f) / 60.0f;
            src[msRandom] = v.random;
            float md[numModDests];
            modSums (p, src, li, true, md);

            const float level = juce::jmax (0.0f, lp.level * lp.level * v.vel * (1.0f + md[mdLevel]));
            const float pan = juce::jlimit (-1.0f, 1.0f, lp.pan + v.panOffset + md[mdPan]);
            const float panL = std::cos ((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
            const float panR = std::sin ((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
            const float chr = juce::jlimit (0.0f, 1.0f, lp.character + md[mdChar]);
            const float reso = juce::jlimit (0.0f, 1.0f, lp.reso + md[mdReso]);
            const float fenv = juce::jlimit (-1.0f, 1.0f, lp.fltEnv + md[mdFltEnv]);
            const float vib = juce::jmax (0.0f, lp.vibDepth + md[mdVibrato] * 100.0f);
            const float cutMul = std::pow (2.0f, md[mdCutoff] * 8.0f);
            const float pitchMod = md[mdPitch] * 24.0f;

            float fc = lp.cutoff;
            for (int i = c0; i < c0 + cn; ++i)
            {
                v.pitch += (v.target - v.pitch) * glideK;
                const double t = v.t;
                const float vibFade = (float) juce::jmin (1.0, t / 0.35);
                const double semis = v.pitch + lp.coarse + lp.fine * 0.01f + bend * p.bendRange + pitchMod
                                   + lp.pitchEnv * std::exp (-t / juce::jmax (0.001f, lp.pitchDecay))
                                   + vib * 0.01f * vibFade * std::sin (twoPi * lp.vibRate * t);
                float x = v.osc.next (mtof (semis), chr);
                const float fe = v.flt.next (lp.fa, lp.fd, lp.fs, lp.fr, sr);
                if (lp.filter != fOff)
                {
                    if (((i - c0) & 7) == 0)
                        fc = lp.cutoff * cutMul * std::pow (2.0f, (fenv * fe * 96.0f + lp.keyTrack * ((float) semis - 60.0f)) / 12.0f);
                    x = v.filter.process (x, lp.filter, fc, reso, sr);
                }
                const float env = stepVolume (v.amp.next (lp.a, lp.d, lp.s, lp.r, sr), steps, logSteps);
                const float y = x * env * level;
                outL[i] += y * panL;
                outR[i] += y * panR;
                v.t += 1.0 / sr;
                if (v.amp.stage == Env::idle || v.osc.finished()) { v.active = false; break; }
            }
        }
    }
}

//==============================================================================
void Engine::globalSources (float* src) const
{
    for (int i = 0; i < numModSources; ++i) src[i] = 0.0f;
}

void Engine::process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const EngineParams& p)
{
    const int n = buffer.getNumSamples();
    if (n > maxBlockSize)
    {
        // hosts may send bigger blocks than announced: split
        int done = 0;
        while (done < n)
        {
            const int k = juce::jmin (maxBlockSize, n - done);
            juce::AudioBuffer<float> sub (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), done, k);
            juce::MidiBuffer m;
            for (const auto meta : midi)
                if (meta.samplePosition >= done && meta.samplePosition < done + k) m.addEvent (meta.getMessage(), meta.samplePosition - done);
            process (sub, m, p);
            done += k;
        }
        return;
    }

    // ---- global modulation sources (block rate) ----
    for (int i = 0; i < numModSources; ++i) gSrc[i] = 0.0f;
    for (int k = 0; k < numLfos; ++k)
    {
        const auto& lf = p.lfo[k];
        const double beats = syncBeats (lf.sync);
        const double rate = beats > 0.0 ? p.bpm / 60.0 / beats : lf.rate;
        if (beats > 0.0 && p.playing) gLfoPh[k] = std::fmod (p.ppq / beats, 1.0);   // locked to the song position
        else gLfoPh[k] += rate * n / sr;
        if (gLfoPh[k] >= 1.0) { gLfoPh[k] -= std::floor (gLfoPh[k]); gLfoPrev[k] = gLfoSH[k]; gLfoSH[k] = rng.nextFloat() * 2.0f - 1.0f; }
        float val = lfoValue (lf.shape, gLfoPh[k], gLfoSH[k], gLfoPrev[k]);
        if (lf.unipolar) val = 0.5f + 0.5f * val;
        gSrc[msLfo1 + k] = val;
    }
    gSrc[msWheel] = wheel; gSrc[msAftertouch] = aftertouch; gSrc[msBend] = bend;
    for (int m = 0; m < numMacros; ++m) { gSrc[msMacro1 + m] = p.macro[m]; lastMacro[m] = p.macro[m]; }
    for (int k = 0; k < 4; ++k) gSrc[msXY1X + k] = p.xy[k];

    for (auto& l : layers) l.buffer.clear (0, n);

    // ---- MIDI, sample accurate ----
    int pos = 0;
    auto renderAll = [&] (int to) { if (to > pos) { for (int li = 0; li < numLayers; ++li) renderLayer (li, pos, to - pos, p); pos = to; } };
    for (const auto meta : midi)
    {
        renderAll (juce::jlimit (0, n, meta.samplePosition));
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            const int note = m.getNoteNumber();
            for (int li = 0; li < numLayers; ++li)
            {
                const auto& lp = p.layer[li];
                if (! lp.on || note < lp.keyLo || note > lp.keyHi) continue;
                auto& L = layers[li];
                if (lp.arpMode != amOff)
                {
                    if (L.arpHeld.empty() || ! L.arpRunning || L.arpFinished)
                    {
                        if (L.arpFinished) releaseAll (li);
                        L.step = -1; L.arpIndex = 0; L.arpRunning = true; L.arpFinished = false;
                        L.stepPos = 0.0;
                        L.arpHeld.clear();
                        L.arpHeld.push_back ({ note, m.getFloatVelocity() });
                        arpStep (li, p);   // first step right away
                    }
                    else
                        L.arpHeld.push_back ({ note, m.getFloatVelocity() });
                }
                else
                    noteOn (li, note, m.getFloatVelocity(), p);
            }
        }
        else if (m.isNoteOff())
        {
            const int note = m.getNoteNumber();
            for (int li = 0; li < numLayers; ++li)
            {
                const auto& lp = p.layer[li];
                auto& L = layers[li];
                if (lp.arpMode != amOff)
                {
                    L.arpHeld.erase (std::remove_if (L.arpHeld.begin(), L.arpHeld.end(), [note] (auto& x) { return x.first == note; }), L.arpHeld.end());
                    if (L.arpHeld.empty() && ! sustainPedal)
                    {
                        L.arpRunning = false; L.uiStep = -1;
                        if (lp.arpEnd == aeHold) releaseAll (li);
                    }
                }
                else
                    noteOff (li, note, p);
            }
        }
        else if (m.isPitchWheel()) { bend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f; gSrc[msBend] = bend; }
        else if (m.isController() && m.getControllerNumber() == 1) { wheel = (float) m.getControllerValue() / 127.0f; gSrc[msWheel] = wheel; }
        else if (m.isChannelPressure()) { aftertouch = (float) m.getChannelPressureValue() / 127.0f; gSrc[msAftertouch] = aftertouch; }
        else if (m.isAftertouch()) { aftertouch = (float) m.getAfterTouchValue() / 127.0f; gSrc[msAftertouch] = aftertouch; }
        else if (m.isSustainPedalOn()) sustainPedal = true;
        else if (m.isSustainPedalOff())
        {
            sustainPedal = false;
            for (int li = 0; li < numLayers; ++li)
            {
                for (auto& v : layers[li].voices)
                    if (v.active && v.sustained) { v.sustained = false; v.amp.gate (false); v.flt.gate (false); for (auto& e : v.menv) e.gate (false); }
                if (layers[li].arpHeld.empty()) { layers[li].arpRunning = false; layers[li].uiStep = -1; }
            }
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            for (int li = 0; li < numLayers; ++li) { layers[li].held.clear(); layers[li].arpHeld.clear(); layers[li].arpRunning = false; layers[li].uiStep = -1; releaseAll (li); }
    }
    renderAll (n);

    // ---- layer FX + sum ----
    const int chans = buffer.getNumChannels();
    for (int li = 0; li < numLayers; ++li)
    {
        auto& L = layers[li];
        float md[numModDests];
        modSums (p, gSrc, li, false, md);
        for (int k = 0; k < layerFxSlots; ++k)
            L.fx[k].process (L.buffer.getWritePointer (0), L.buffer.getWritePointer (1), n, p.layer[li].fx[k], p.bpm,
                             md[mdFxA1 + 2 * k], md[mdFxAMix + 2 * k]);
        if (chans > 1) { buffer.addFrom (0, 0, L.buffer, 0, 0, n); buffer.addFrom (1, 0, L.buffer, 1, 0, n); }
        else { buffer.addFrom (0, 0, L.buffer, 0, 0, n, 0.5f); buffer.addFrom (0, 0, L.buffer, 1, 0, n, 0.5f); }
    }

    // ---- master: FX slots, tone, space, volume ----
    float mm[numModDests];
    modSums (p, gSrc, -1, false, mm);
    for (int k = 0; k < masterFxSlots; ++k)
        masterFx[k].process (buffer.getWritePointer (0), chans > 1 ? buffer.getWritePointer (1) : nullptr, n, p.masterFx[k], p.bpm, mm[mdMFx1 + k]);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    const float toneV = juce::jlimit (0.0f, 1.0f, p.tone + mm[mdTone]);
    tone.setCutoffFrequency (200.0f * std::pow (100.0f, toneV));
    if (toneV < 0.995f) tone.process (ctx);
    const float space = juce::jlimit (0.0f, 1.0f, p.space + mm[mdSpace]);
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = 0.78f; rp.damping = 0.45f; rp.width = 1.0f;
    rp.wetLevel = space * 0.55f; rp.dryLevel = 1.0f - space * 0.35f;
    reverb.setParameters (rp);
    if (space > 0.001f) reverb.process (ctx);
    gain.setTargetValue (juce::Decibels::decibelsToGain (juce::jlimit (-60.0f, 12.0f, p.volumeDb + mm[mdVolume] * 24.0f)));
    gain.applyGain (buffer, n);

    // output safety: transparent below -3 dBFS, soft knee above, never past -0.2 dBFS
    for (int c = 0; c < buffer.getNumChannels(); ++c)
    {
        auto* d = buffer.getWritePointer (c);
        for (int i = 0; i < n; ++i)
        {
            const float a = std::abs (d[i]);
            if (a > 0.7f) d[i] = std::copysign (0.7f + 0.28f * std::tanh ((a - 0.7f) / 0.28f), d[i]);
        }
    }
}
} // namespace bitera
