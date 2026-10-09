// Offline DSP test harness for Trap Limit (native build)
#define DISTRHO_PLUGIN_TARGET_STATIC 1
#include "DistrhoPluginMain.cpp"
#include "../plugin/TrapLimitPlugin.cpp"
#include <cstdio>
#include <vector>
#include <cmath>
#include <chrono>

USE_NAMESPACE_DISTRHO

static const double SR = 48000.0;
static const uint32_t BS = 512;

struct Host {
    PluginExporter* p;
    Host() {
        d_nextBufferSize = BS; d_nextSampleRate = SR;
        p = new PluginExporter(nullptr, nullptr, nullptr, nullptr);
        p->setSampleRate(SR); p->setBufferSize(BS);
        p->activate();
    }
    ~Host() { p->deactivate(); delete p; }
    void set(uint32_t i, float v) { p->setParameterValue(i, v); }
    float get(uint32_t i) { return p->getParameterValue(i); }
    void process(std::vector<float>& L, std::vector<float>& R, std::vector<float>& oL, std::vector<float>& oR) {
        oL.assign(L.size(), 0); oR.assign(R.size(), 0);
        for (size_t s = 0; s < L.size(); s += BS) {
            uint32_t n = (uint32_t)std::min<size_t>(BS, L.size() - s);
            const float* in[2] = { &L[s], &R[s] };
            float* out[2] = { &oL[s], &oR[s] };
            p->run(in, out, n);
        }
    }
};

static float peakDb(const std::vector<float>& a, size_t from = 0) {
    float m = 0; for (size_t i = from; i < a.size(); ++i) m = std::max(m, std::fabs(a[i])); return 20 * std::log10(std::max(m, 1e-9f));
}
static float truePeakDb(const std::vector<float>& a, size_t from = 0) {
    tl::TruePeak tp; float m = 0;
    for (size_t i = 0; i < a.size(); ++i) { float v = tp.process(a[i]); if (i >= from) m = std::max(m, v); }
    return 20 * std::log10(std::max(m, 1e-9f));
}
static bool finite(const std::vector<float>& a) { for (float v : a) if (!std::isfinite(v)) return false; return true; }

static int fails = 0;
#define CHECK(cond, ...) do { bool ok_ = (cond); std::printf("[%s] ", ok_ ? "PASS" : "FAIL"); std::printf(__VA_ARGS__); std::printf("\n"); if (!ok_) ++fails; } while (0)

// trap-ish test signal: 808 (50Hz decaying sine w/ pitch drop), kick click, hats, snare noise
static void makeTrap(std::vector<float>& L, std::vector<float>& R, double secs) {
    size_t n = (size_t)(secs * SR); L.assign(n, 0); R.assign(n, 0);
    uint32_t seed = 1; auto rnd = [&]() { seed = seed * 1664525u + 1013904223u; return ((seed >> 9) / 4194304.0f) - 1.f; };
    const double bpm = 140, beat = 60.0 / bpm;
    for (size_t i = 0; i < n; ++i) {
        double t = i / SR;
        double tb = std::fmod(t, beat * 2);          // 808 every 2 beats
        double f = 55.0 + 60.0 * std::exp(-tb * 30);
        static double ph = 0; ph += 2 * M_PI * f / SR;
        float s808 = (float)(0.9 * std::sin(ph) * std::exp(-tb * 1.2));
        double tk = std::fmod(t, beat);
        float kick = (float)(0.6 * std::sin(2 * M_PI * 120 * tk * std::exp(-tk * 20)) * std::exp(-tk * 25));
        double th = std::fmod(t, beat / 4);
        float hat = (float)(0.25 * rnd() * std::exp(-th * 120));
        double ts = std::fmod(t + beat, beat * 2);
        float snare = (float)(0.5 * rnd() * std::exp(-ts * 18));
        float m = s808 + kick + snare;
        L[i] = m + hat * 0.8f; R[i] = m + hat * 1.1f;
    }
}

int main()
{
    std::vector<float> L, R, oL, oR;

    // ---- 1. latency & transparency at low level ----
    {
        Host h; h.set(kClip, 0); h.set(kOversample, 0);
        uint32_t lat = h.p->getLatency();
        size_t n = 48000; L.assign(n, 0); R.assign(n, 0);
        L[1000] = 0.1f; R[1000] = 0.1f;
        h.process(L, R, oL, oR);
        size_t idx = 0; float m = 0; for (size_t i = 0; i < n; ++i) if (std::fabs(oL[i]) > m) { m = std::fabs(oL[i]); idx = i; }
        CHECK(idx == 1000 + lat, "latency: impulse at %zu, expected %u (reported latency %u samples = %.2f ms)", idx, 1000 + lat, lat, lat * 1000.0 / SR);
        CHECK(std::fabs(m - 0.1f) < 0.002f, "unity at low level: peak %.4f", m);
        for (int os = 1; os <= 2; ++os) {
            Host h2; h2.set(kClip, 0); h2.set(kOversample, (float)os);
            h2.process(L, R, oL, oR);
            size_t id2 = 0; float m2 = 0; for (size_t i = 0; i < n; ++i) if (std::fabs(oL[i]) > m2) { m2 = std::fabs(oL[i]); id2 = i; }
            CHECK(id2 == 1000 + lat, "latency constant with %s oversampling: impulse at %zu", kOversampleNames[os], id2);
        }
    }

    // ---- 2. loudness calibration: 1 kHz sine -20 dBFS both channels => -20 LUFS ----
    {
        Host h; h.set(kLimit, 0); h.set(kClip, 0);
        size_t n = (size_t)(SR * 6); L.resize(n); R.resize(n);
        for (size_t i = 0; i < n; ++i) L[i] = R[i] = (float)(0.1 * std::sin(2 * M_PI * 997 * i / SR));
        h.process(L, R, oL, oR);
        float li = h.get(kOutLufsI), ls = h.get(kOutLufsS), lm = h.get(kOutLufsM);
        CHECK(std::fabs(li + 20.f) < 0.2f, "LUFS calibration: integrated %.2f (short %.2f, momentary %.2f), expected -20.0", li, ls, lm);
    }

    // ---- 3. limiting: hot sine + gain, ceiling respected ----
    for (int tp = 0; tp <= 1; ++tp) {
        Host h; h.set(kGain, 12); h.set(kClip, 0); h.set(kTruePeak, (float)tp); h.set(kCeiling, -1.0f);
        size_t n = (size_t)(SR * 2); L.resize(n); R.resize(n);
        for (size_t i = 0; i < n; ++i) L[i] = R[i] = (float)(0.8 * std::sin(2 * M_PI * 3333.3 * i / SR) * (0.5 + 0.5 * std::sin(2 * M_PI * 2 * i / SR)));
        h.process(L, R, oL, oR);
        float pk = peakDb(oL, 4800), tpk = truePeakDb(oL, 4800);
        CHECK(pk <= -1.0f + 0.01f, "ceiling -1 dB (TP %s): sample peak %.3f dB, true peak %.3f dBTP, GR now %.1f dB", tp ? "on" : "off", pk, tpk, h.get(kOutGR));
        if (tp) CHECK(tpk <= -0.8f, "true-peak mode keeps inter-sample peaks near ceiling: %.3f dBTP", tpk);
    }

    // ---- 4. trap signal through all modes / OS / features ----
    makeTrap(L, R, 8.0);
    std::printf("\nTrap test signal: in peak %.2f dB, in TP %.2f dBTP\n", peakDb(L), truePeakDb(L));
    const char* modeN[4] = { "Transparent", "Punchy", "Loud", "Dirty" };
    for (int mode = 0; mode < 4; ++mode) for (int os = 0; os < 3; ++os) {
        Host h; h.set(kGain, 8); h.set(kMode, (float)mode); h.set(kOversample, (float)os);
        auto t0 = std::chrono::steady_clock::now();
        h.process(L, R, oL, oR);
        double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        float tpk = std::max(truePeakDb(oL, 4800), truePeakDb(oR, 4800));
        CHECK(finite(oL) && finite(oR) && peakDb(oL) <= -0.29f && tpk < 0.3f,
              "%-11s %s: LUFS-I %.1f, TP %.2f dBTP, LRA %.1f, CPU %.2f%% of realtime",
              modeN[mode], kOversampleNames[os], h.get(kOutLufsI), tpk, h.get(kOutLRA), 100.0 * el / 8.0);
    }
    // features
    struct F { const char* n; uint32_t id; float v; } feats[] = {
        {"808 boost 80%", k808, 80}, {"sub split", kSubSplit, 1}, {"mono sub", kMonoSub, 1}, {"HPF sidechain", kHpfSc, 1},
        {"M/S", kMS, 1}, {"width 200%", kStereo, 200}, {"punch 100%", kPunch, 100}, {"sat 100%", kSat, 100},
        {"hard clip", kClipShape, 1}, {"tape clip", kClipShape, 2}, {"dither", kDither, 1}, {"link 0%", kLink, 0},
        {"auto rel off", kAutoRel, 0}, {"lookahead 5ms", kLookahead, 5}, {"attack 0.05", kAttack, 0.05f}, {"release 1000", kRelease, 1000},
    };
    for (auto& f : feats) {
        Host h; h.set(kGain, 8); h.set(f.id, f.v);
        h.process(L, R, oL, oR);
        float tpk = std::max(truePeakDb(oL, 4800), truePeakDb(oR, 4800));
        CHECK(finite(oL) && finite(oR) && peakDb(oL) <= -0.29f && tpk < 0.0f, "%-14s LUFS-I %.1f TP %.2f", f.n, h.get(kOutLufsI), tpk);
    }

    // ---- 5. bypass = delayed dry, delta, unity, stream preview ----
    {
        Host h; h.set(kGain, 8); h.set(kBypass, 1);
        h.process(L, R, oL, oR);
        uint32_t lat = h.p->getLatency(); double err = 0;
        for (size_t i = 10000; i < L.size(); ++i) err = std::max(err, (double)std::fabs(oL[i] - L[i - lat]));
        CHECK(err < 1e-6, "bypass is latency-compensated dry (max err %.2e)", err);
    }
    {
        Host h; h.set(kGain, 8); h.set(kStream, 1);
        h.process(L, R, oL, oR);
        std::vector<float> tail(oL.end() - 48000, oL.end()), tailR(oR.end() - 48000, oR.end());
        tl::LoudnessMeter lm; lm.init(SR); for (int k = 0; k < 4; ++k) for (size_t i = 0; i < tail.size(); ++i) lm.process(tail[i], tailR[i]);
        CHECK(std::fabs(lm.integrated + 14.f) < 1.5f, "stream preview -14: output loudness %.1f LUFS (meter shows pre-preview %.1f)", lm.integrated, h.get(kOutLufsI));
    }
    {
        Host h; h.set(kGain, 8); h.set(kDelta, 1);
        h.process(L, R, oL, oR);
        CHECK(finite(oL) && peakDb(oL) <= 0.01f, "delta output finite, peak %.1f dB", peakDb(oL));
    }

    // ---- 6. other sample rates ----
    for (double sr : { 44100.0, 96000.0, 192000.0 }) {
        d_nextBufferSize = BS; d_nextSampleRate = sr;
        PluginExporter p(nullptr, nullptr, nullptr, nullptr);
        p.setSampleRate(sr); p.activate(); p.setParameterValue(kGain, 10); p.setParameterValue(kOversample, 2);
        std::vector<float> a((size_t)sr * 2), b, c(a.size()), d(a.size());
        for (size_t i = 0; i < a.size(); ++i) a[i] = (float)(0.9 * std::sin(2 * M_PI * 60 * i / sr) + 0.3 * std::sin(2 * M_PI * 5000 * i / sr));
        for (size_t s = 0; s < a.size(); s += BS) { uint32_t n = (uint32_t)std::min<size_t>(BS, a.size() - s); const float* in[2] = { &a[s], &a[s] }; float* out[2] = { &c[s], &d[s] }; p.run(in, out, n); }
        CHECK(finite(c) && peakDb(c, (size_t)(sr * 0.1)) <= -0.29f, "%.0f Hz: latency %u samples, peak %.2f dB", sr, p.getLatency(), peakDb(c, (size_t)(sr * 0.1)));
        p.deactivate();
    }

    // ---- 7. no subnormal (denormal) samples in the output, e.g. in the silent tail after a signal ----
    {
        auto countSub = [](const std::vector<float>& a) { size_t n = 0; for (float v : a) if (v != 0.f && std::fabs(v) < 1.17549435e-38f) ++n; return n; };
        struct F { const char* n; uint32_t id; float v; } cfg[] = {
            {"default", kGain, 0}, {"808 boost", k808, 80}, {"sub split", kSubSplit, 1}, {"mono sub", kMonoSub, 1},
            {"HPF sidechain", kHpfSc, 1}, {"punch", kPunch, 100}, {"dirty mode", kMode, 3}, {"no clip, 4x", kOversample, 2},
        };
        for (auto& f : cfg) {
            Host h; h.set(f.id, f.v);
            size_t n = (size_t)(SR * 6); std::vector<float> a(n, 0.f), b(n, 0.f);
            for (size_t i = 0; i < (size_t)SR / 2; ++i) a[i] = b[i] = (float)(0.8 * std::sin(2 * M_PI * 55 * i / SR) * std::exp(-3.0 * i / SR));
            // tiny input values like a host can deliver after a fade-out
            for (size_t i = (size_t)SR; i < (size_t)SR * 2; ++i) a[i] = b[i] = 1e-39f;
            h.process(a, b, oL, oR);
            const size_t sub = countSub(oL) + countSub(oR);
            CHECK(sub == 0, "no subnormals in output (%s): %zu found", f.n, sub);
        }
    }

    std::printf("\n%s: %d failure(s)\n", fails ? "FAILED" : "ALL PASSED", fails);
    return fails ? 1 : 0;
}
