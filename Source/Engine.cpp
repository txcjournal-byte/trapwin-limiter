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
Engine::Engine() = default;

void Engine::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, maxBlock), 2 };
    tone.prepare (spec);
    tone.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    tone.setResonance (0.707f);
    reverb.prepare (spec);
    gain.reset (sampleRate, 0.03);
    reset();
}

void Engine::reset()
{
    for (auto& v : voices) { v.active = false; for (auto& l : v.layers) l.active = false; }
    heldNotes.clear();
    lastPitch = -1.0f; bend = 0.0f; sustainPedal = false;
    tone.reset(); reverb.reset();
}

bool Engine::isSilent() const
{
    for (auto& v : voices) if (v.active) return false;
    return true;
}

void Engine::startVoice (Voice& v, int note, float vel, float fromPitch, const EngineParams& p)
{
    v.active = false;
    v.note = note; v.velocity = vel; v.held = true; v.sustained = false;
    v.target = (float) note;
    v.pitch = (p.glide > 0.0005f && fromPitch >= 0.0f) ? fromPitch : (float) note;
    v.age = ++counter; v.t = 0.0;
    for (int i = 0; i < numLayers; ++i)
    {
        const auto& lp = p.layer[i];
        auto& l = v.layers[i];
        l.active = lp.on && note >= lp.keyLo && note <= lp.keyHi;
        if (! l.active) continue;
        l.osc.start (lp.source, lp.wave, sr, bank);
        l.amp = {}; l.flt = {};
        l.amp.gate (true); l.flt.gate (true);
        l.filter.reset();
        l.vel = 1.0f - lp.velSens * (1.0f - vel);
        v.active = true;
    }
}

void Engine::noteOn (int note, float vel, const EngineParams& p)
{
    if (p.mode != vmPoly)
    {
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        heldNotes.push_back (note);
        Voice* cur = nullptr;
        for (auto& v : voices) if (v.active && (cur == nullptr || v.age > cur->age)) cur = &v;
        if (cur != nullptr && cur->held && p.mode == vmLegato)
        {
            cur->note = note; cur->target = (float) note;
            if (p.glide <= 0.0005f) cur->pitch = (float) note;
            lastPitch = (float) note;
            return;
        }
        const float from = cur != nullptr ? cur->pitch : lastPitch;
        for (auto& v : voices) if (&v != cur) v.active = false;   // mono: one voice only
        startVoice (cur != nullptr ? *cur : voices[0], note, vel, from, p);
        lastPitch = (float) note;
        return;
    }

    Voice* slot = nullptr;
    for (auto& v : voices) if (! v.active) { slot = &v; break; }
    if (slot == nullptr)   // steal: the oldest released voice, else the oldest
    {
        for (auto& v : voices) if (! v.held && (slot == nullptr || v.age < slot->age)) slot = &v;
        if (slot == nullptr) for (auto& v : voices) if (slot == nullptr || v.age < slot->age) slot = &v;
    }
    startVoice (*slot, note, vel, lastPitch, p);
    lastPitch = (float) note;
}

void Engine::noteOff (int note, const EngineParams& p)
{
    if (p.mode != vmPoly)
    {
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        for (auto& v : voices)
        {
            if (! v.active || ! v.held) continue;
            if (! heldNotes.empty())
            {
                if (v.note == note) { v.note = heldNotes.back(); v.target = (float) v.note; if (p.glide <= 0.0005f) v.pitch = v.target; lastPitch = v.target; }
            }
            else if (v.note == note)
            {
                v.held = false;
                for (auto& l : v.layers) if (l.active) { l.amp.gate (false); l.flt.gate (false); }
            }
        }
        return;
    }
    for (auto& v : voices)
        if (v.active && v.held && v.note == note)
        {
            v.held = false;
            if (sustainPedal) { v.sustained = true; continue; }
            for (auto& l : v.layers) if (l.active) { l.amp.gate (false); l.flt.gate (false); }
        }
}

void Engine::render (int start, int num, juce::AudioBuffer<float>& buf, const EngineParams& p)
{
    if (num <= 0) return;
    const int chans = buf.getNumChannels();
    auto* L = buf.getWritePointer (0, start);
    auto* R = chans > 1 ? buf.getWritePointer (1, start) : nullptr;
    const float glideK = p.glide > 0.0005f ? (float) (1.0 - std::exp (-1.0 / (p.glide * sr))) : 1.0f;

    for (auto& v : voices)
    {
        if (! v.active) continue;
        bool any = false;
        for (int li = 0; li < numLayers; ++li)
        {
            auto& l = v.layers[li];
            if (! l.active) continue;
            const auto& lp = p.layer[li];
            const float panL = std::cos ((lp.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
            const float panR = std::sin ((lp.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
            const int steps = chips::volumeSteps (lp.source);
            const bool logSteps = chips::volumeLog (lp.source);
            const float lvl = lp.level * lp.level * l.vel;
            float pitch = v.pitch;
            double t = v.t;
            float fc = lp.cutoff;
            for (int i = 0; i < num; ++i)
            {
                pitch += (v.target - pitch) * glideK;
                const float vibFade = (float) juce::jmin (1.0, t / 0.35);
                const double semis = pitch + lp.coarse + lp.fine * 0.01f + bend * p.bendRange
                                   + lp.pitchEnv * std::exp (-t / juce::jmax (0.001f, lp.pitchDecay))
                                   + lp.vibDepth * 0.01f * vibFade * std::sin (twoPi * lp.vibRate * t);
                float x = l.osc.next (mtof (semis), lp.character);
                const float fe = l.flt.next (lp.fa, lp.fd, lp.fs, lp.fr, sr);
                if (lp.filter != fOff && (i & 15) == 0)
                    fc = lp.cutoff * std::pow (2.0f, (lp.fltEnv * fe * 96.0f + lp.keyTrack * ((float) semis - 60.0f)) / 12.0f);
                x = l.filter.process (x, lp.filter, fc, lp.reso, sr);
                const float env = stepVolume (l.amp.next (lp.a, lp.d, lp.s, lp.r, sr), steps, logSteps);
                const float y = x * env * lvl;
                L[i] += y * panL;
                if (R != nullptr) R[i] += y * panR;
                t += 1.0 / sr;
                if (l.amp.stage == Env::idle || l.osc.finished()) { l.active = false; break; }
            }
            if (l.active) any = true;
        }
        v.pitch += (v.target - v.pitch) * (1.0f - std::pow (1.0f - glideK, (float) num));
        v.t += num / sr;
        if (! any) v.active = false;
    }
}

void Engine::process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const EngineParams& p)
{
    int pos = 0;
    const int n = buffer.getNumSamples();
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, n, meta.samplePosition);
        render (pos, at - pos, buffer, p);
        pos = at;
        const auto m = meta.getMessage();
        if (m.isNoteOn()) noteOn (m.getNoteNumber(), m.getFloatVelocity(), p);
        else if (m.isNoteOff()) noteOff (m.getNoteNumber(), p);
        else if (m.isPitchWheel()) bend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
        else if (m.isSustainPedalOn()) sustainPedal = true;
        else if (m.isSustainPedalOff())
        {
            sustainPedal = false;
            for (auto& v : voices)
                if (v.active && v.sustained) { v.sustained = false; for (auto& l : v.layers) if (l.active) { l.amp.gate (false); l.flt.gate (false); } }
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            heldNotes.clear();
            for (auto& v : voices) { v.held = false; for (auto& l : v.layers) if (l.active) { l.amp.gate (false); l.flt.gate (false); } }
        }
    }
    render (pos, n - pos, buffer, p);

    // master: tone (low-pass), space (reverb), volume
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    tone.setCutoffFrequency (200.0f * std::pow (100.0f, p.tone));
    if (p.tone < 0.995f) tone.process (ctx);
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = 0.78f; rp.damping = 0.45f; rp.width = 1.0f;
    rp.wetLevel = p.space * 0.55f; rp.dryLevel = 1.0f - p.space * 0.35f;
    reverb.setParameters (rp);
    if (p.space > 0.001f) reverb.process (ctx);
    gain.setTargetValue (juce::Decibels::decibelsToGain (p.volumeDb));
    gain.applyGain (buffer, n);
}
} // namespace bitera
