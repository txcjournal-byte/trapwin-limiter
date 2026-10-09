// Trap Limit - limiter / clipper for trap & underground music
#include "DistrhoPlugin.hpp"
#include "Params.hpp"
#include "Dsp.hpp"
#include <chrono>
#include <random>
#if defined(__SSE__) || defined(_M_X64) || defined(__x86_64__)
# include <xmmintrin.h>
#endif

START_NAMESPACE_DISTRHO

using namespace tl;

static const int kOSLatency = 48;   // fixed oversampling-stage latency (4x path), padded for 1x/2x
static const int kTPLatency = 6;

// Flush-to-zero for the duration of run(): denormal numbers in the filter/envelope
// tails are slow on many CPUs and must never reach the host (pluginval checks this).
struct ScopedFlushDenormals {
#if defined(__SSE__) || defined(_M_X64) || defined(__x86_64__)
    unsigned int csr;
    ScopedFlushDenormals() : csr(_mm_getcsr()) { _mm_setcsr(csr | 0x8040); }  // FTZ | DAZ
    ~ScopedFlushDenormals() { _mm_setcsr(csr); }
#elif defined(__aarch64__)
    unsigned long long fpcr;
    ScopedFlushDenormals() { __asm__ __volatile__("mrs %0, fpcr" : "=r"(fpcr)); unsigned long long n = fpcr | (1ULL << 24); __asm__ __volatile__("msr fpcr, %0" :: "r"(n)); }
    ~ScopedFlushDenormals() { __asm__ __volatile__("msr fpcr, %0" :: "r"(fpcr)); }
#endif
};

static inline float flushTiny(float v) { return std::fabs(v) < 1e-30f ? 0.f : v; }

class TrapLimitPlugin : public Plugin
{
public:
    DisplayHistory history;

    TrapLimitPlugin()
        : Plugin(kParamCount, kNumPresets, 0)
    {
        for (uint32_t i = 0; i < kNumInputParams; ++i) p[i] = kParamInfo[i].def;
        std::memset(&history, 0, sizeof(history));
        initDsp(getSampleRate());
    }

protected:
    // ------------------------------------------------------------ info
    const char* getLabel() const override { return "TrapWinLimiter"; }
    const char* getDescription() const override { return "Clipper + true-peak limiter for trap and underground music."; }
    const char* getMaker() const override { return "TrapWin"; }
    const char* getHomePage() const override { return ""; }
    const char* getLicense() const override { return "Proprietary"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }

    void initAudioPort(bool input, uint32_t index, AudioPort& port) override
    {
        port.groupId = kPortGroupStereo;
        Plugin::initAudioPort(input, index, port);
    }

    void initParameter(uint32_t index, Parameter& param) override
    {
        if (index < kNumInputParams)
        {
            const ParamInfo& pi = kParamInfo[index];
            param.name = pi.name;
            param.symbol = pi.symbol;
            param.unit = pi.unit;
            param.ranges.min = pi.min;
            param.ranges.max = pi.max;
            param.ranges.def = pi.def;
            param.hints = kParameterIsAutomatable;
            switch (pi.kind) {
            case PK_Bool: param.hints |= kParameterIsBoolean | kParameterIsInteger; break;
            case PK_Enum: param.hints |= kParameterIsInteger; break;
            case PK_Trigger: param.hints = kParameterIsTrigger | kParameterIsInteger | kParameterIsBoolean; break;
            default: if (pi.log) param.hints |= kParameterIsLogarithmic; break;
            }
            if (index == kBypass) param.initDesignation(kParameterDesignationBypass);
            if (index == kOversample) setEnum(param, kOversampleNames, 3);
            if (index == kStream)     setEnum(param, kStreamNames, 4);
            if (index == kClipShape)  setEnum(param, kClipShapeNames, 3);
            if (index == kMode)       setEnum(param, kModeNames, 4);
            return;
        }
        param.hints = kParameterIsOutput;
        param.ranges.def = -100.f;
        switch (index) {
        case kOutInL:  param.name = "In L";  param.symbol = "in_l";  param.ranges.min = -100; param.ranges.max = 12; break;
        case kOutInR:  param.name = "In R";  param.symbol = "in_r";  param.ranges.min = -100; param.ranges.max = 12; break;
        case kOutOutL: param.name = "Out L"; param.symbol = "out_l"; param.ranges.min = -100; param.ranges.max = 12; break;
        case kOutOutR: param.name = "Out R"; param.symbol = "out_r"; param.ranges.min = -100; param.ranges.max = 12; break;
        case kOutGR:   param.name = "Gain Reduction"; param.symbol = "gr"; param.ranges.min = -60; param.ranges.max = 0; param.ranges.def = 0; break;
        case kOutLufsM: param.name = "Momentary"; param.symbol = "lufs_m"; param.ranges.min = -100; param.ranges.max = 10; break;
        case kOutLufsS: param.name = "Short Term"; param.symbol = "lufs_s"; param.ranges.min = -100; param.ranges.max = 10; break;
        case kOutLufsI: param.name = "Integrated"; param.symbol = "lufs_i"; param.ranges.min = -100; param.ranges.max = 10; break;
        case kOutLRA:  param.name = "LRA"; param.symbol = "lra"; param.ranges.min = 0; param.ranges.max = 80; param.ranges.def = 0; break;
        case kOutTP:   param.name = "True Peak Max"; param.symbol = "tp"; param.ranges.min = -100; param.ranges.max = 12; break;
        case kOutCPU:  param.name = "CPU"; param.symbol = "cpu"; param.ranges.min = 0; param.ranges.max = 100; param.ranges.def = 0; break;
        case kOutClipGR: param.name = "Clipper Reduction"; param.symbol = "clip_gr"; param.ranges.min = -60; param.ranges.max = 0; param.ranges.def = 0; break;
        case kOutLatency: param.name = "Latency"; param.symbol = "latency_ms"; param.ranges.min = 0; param.ranges.max = 100; param.ranges.def = 0; break;
        }
    }

    static void setEnum(Parameter& param, const char* const* names, int n)
    {
        ParameterEnumerationValue* v = new ParameterEnumerationValue[n];
        for (int i = 0; i < n; ++i) { v[i].label = names[i]; v[i].value = (float)i; }
        param.enumValues.count = (uint8_t)n;
        param.enumValues.restrictedMode = true;
        param.enumValues.values = v;
    }

    void initProgramName(uint32_t index, String& name) override { name = kPresets[index].name; }

    void loadProgram(uint32_t index) override
    {
        for (uint32_t i = 0; i < kNumInputParams; ++i)
            if (i != kBypass && i != kResetMeters) p[i] = kParamInfo[i].def;
        const Preset& pr = kPresets[index];
        for (int k = 0; k < pr.n; ++k) p[pr.vals[k].id] = pr.vals[k].v;
        paramsChanged();
    }

    float getParameterValue(uint32_t index) const override
    {
        if (index < kNumInputParams) return p[index];
        return out[index - kNumInputParams];
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (index >= kNumInputParams) return;
        if (index == kResetMeters) {
            if (value > 0.5f) resetMetersFlag = true;
            p[index] = 0.f;
            return;
        }
        p[index] = value;
        paramsChanged();
    }

    // ------------------------------------------------------------ dsp setup
    void sampleRateChanged(double sr) override { initDsp(sr); }

    void activate() override { resetDsp(); }

    void initDsp(double sr)
    {
        fs = sr > 0 ? sr : 48000.0;
        laMax = (int)std::ceil(0.005 * fs) + 1;
        dLim = kTPLatency + laMax - 1;
        totalLatency = kOSLatency + dLim;
        setLatency((uint32_t)totalLatency);
        out[kOutLatency - kNumInputParams] = (float)(1000.0 * totalLatency / fs);

        for (int c = 0; c < 2; ++c) {
            dryDelay[c].init(totalLatency + 4);
            preDelay[c].init(totalLatency + 4);
            osPad[c].init(kOSLatency + 4);
            audioDelay[c].init(dLim + 4);
            detDelay[c].init(kTPLatency + 4);
            gainDelay[c].init(laMax + 4); gainDelay[c].fill(1.f);
            minWin[c].init(laMax + 2);
            avg[c].init(laMax + 2);
        }
        loud.init(fs);
        punchFast.set(fs, 0.5, 40); punchSlow.set(fs, 15, 180);
        for (int c = 0; c < 2; ++c) { scEnvAll[c].set(fs, 1, 120); scEnvLow[c].set(fs, 1, 120); }
        gainSm.set(fs, 20); driveSm.set(fs, 20); widthSm.set(fs, 20); streamSm.set(fs, 300); bypassSm.set(fs, 8);
        histLen = std::max(1, (int)(fs * 4.0 / kHistoryLen));
        cpuAvg = 0;
        paramsChanged();
        resetDsp();
    }

    void configureOsFilters()
    {
        const int osf = osFactor();
        const double fsOs = fs * osf;
        if (fsOs == lastFsOs) return;
        lastFsOs = fsOs;
        for (int c = 0; c < 2; ++c) {
            for (int s = 0; s < 2; ++s) {
                xLP[c][s].setLowpass(fsOs, 120.0, 0.7071067811865476);
                xHP[c][s].setHighpass(fsOs, 120.0, 0.7071067811865476);
            }
            dc808[c].set(fsOs, 25.0);
            dcTape[c].set(fsOs, 10.0);
        }
    }

    void resetDsp()
    {
        for (int c = 0; c < 2; ++c) {
            dryDelay[c].clear(); preDelay[c].clear(); osPad[c].clear(); audioDelay[c].clear();
            detDelay[c].clear(); gainDelay[c].fill(1.f); minWin[c].clear();
            avg[c].fill(1.f);
            g[c] = 1.f; avgGrDb[c] = 0.f;
            tpDet[c].reset(); tpOut[c].reset();
            scHP[c].setHighpass(fs, 100.0, 0.7071); scHP[c].reset();
            for (int s = 0; s < 2; ++s) { up1[c].reset(); up2[c].reset(); dn1[c].reset(); dn2[c].reset(); xLP[c][s].reset(); xHP[c][s].reset(); }
            dc808[c].reset(); dcTape[c].reset();
        }
        punchFast.env = punchSlow.env = 0;
        lastFsOs = 0; configureOsFilters();
        gainSm.reset(dbToLin(p[kGain]));
        driveSm.reset(dbToLin(p[kDrive] + modeDriveAdd()));
        widthSm.reset(p[kStereo] * 0.01f);
        streamSm.reset(1.f);
        bypassSm.reset(p[kBypass] > 0.5f ? 1.f : 0.f);
        loud.reset();
        tpMax = 0; histPos = 0; histCount = 0; histPeak = 0; histGr = 0;
    }

    int osFactor() const { int o = (int)(p[kOversample] + 0.5f); return o == 0 ? 1 : (o == 1 ? 2 : 4); }
    int mode() const { return (int)(p[kMode] + 0.5f); }
    float modeDriveAdd() const { return mode() == 2 ? 2.f : 0.f; }

    void paramsChanged()
    {
        const int m = mode();
        relMul   = m == 0 ? 1.f : m == 1 ? 0.7f : m == 2 ? 0.5f : 0.6f;
        punchAdd = m == 1 ? 0.35f : 0.f;
        satAdd   = m == 3 ? 0.45f : 0.f;
        attMul   = m == 1 ? 1.6f : 1.f;

        ceilLin = dbToLin(p[kCeiling]);
        laSamp = std::max(1, std::min(laMax - 1, (int)std::lround(p[kLookahead] * 0.001 * fs)));
        attSamp = std::max(1, std::min(laSamp, (int)std::lround(p[kAttack] * attMul * 0.001 * fs)));
        relMs = p[kRelease] * relMul;
        link = p[kLink] * 0.01f;
        sat = std::min(1.f, p[kSat] * 0.01f + satAdd);
        amt808 = p[k808] * 0.01f;
        punch = std::min(1.f, p[kPunch] * 0.01f + punchAdd);
        clipShape = (int)(p[kClipShape] + 0.5f);
        gainSm.target = dbToLin(p[kGain]);
        driveSm.target = dbToLin(p[kDrive] + modeDriveAdd());
        widthSm.target = p[kStereo] * 0.01f;
        bypassSm.target = p[kBypass] > 0.5f ? 1.f : 0.f;
        configureOsFilters();
    }

    // ------------------------------------------------------------ nonlinear stage (runs at OS rate)
    inline void nonlinear(float& l, float& r, float drive)
    {
        float x[2] = { l, r };
        const bool needLow = amt808 > 0.f || p[kSubSplit] > 0.5f || p[kMonoSub] > 0.5f;
        float low[2] = { 0, 0 }, high[2] = { x[0], x[1] };
        if (needLow) {
            for (int c = 0; c < 2; ++c) {
                low[c]  = xLP[c][1].process(xLP[c][0].process(x[c]));
                high[c] = xHP[c][1].process(xHP[c][0].process(x[c]));
            }
            if (p[kMonoSub] > 0.5f) { float m = 0.5f * (low[0] + low[1]); low[0] = low[1] = m; }
            if (amt808 > 0.f) {
                for (int c = 0; c < 2; ++c) {
                    float lo = low[c];
                    float even = dc808[c].process(std::fabs(lo));          // octave-up harmonic
                    float odd = lo * lo * lo / (1.f + 4.f * lo * lo);      // 3rd harmonic, bounded
                    low[c] = lo + amt808 * (0.9f * even + 1.2f * odd);
                }
            }
        }
        const bool clipOn = p[kClip] > 0.5f;
        for (int c = 0; c < 2; ++c) {
            float lo = low[c] * drive, hi = high[c] * drive;
            float y;
            if (needLow && p[kSubSplit] > 0.5f && clipOn) {
                lo = softClip(lo, ceilLin * 0.85f);
                y = hi;
            } else {
                y = needLow ? hi + lo : x[c] * drive;
                lo = 0.f;
            }
            if (sat > 0.f) {
                const float k = 1.f + 1.2f * sat;
                float s = std::tanh(y * k) / std::tanh(k);   // peak-preserving, adds density
                if (mode() == 3) s = dcTape[c].process(s + 0.08f * sat * s * s);
                y = (1.f - sat) * y + sat * s;
            }
            if (clipOn) {
                const float pre = std::fabs(y);
                switch (clipShape) {
                case 1:  y = hardClip(y, ceilLin); break;
                case 2:  y = tapeClip(y, ceilLin); break;
                default: y = softClip(y, ceilLin); break;
                }
                if (pre > 0.05f) clipMinRatio = std::min(clipMinRatio, std::fabs(y) / pre);
            }
            x[c] = y + lo;
        }
        l = x[0]; r = x[1];
    }

    // ------------------------------------------------------------ process
    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        const ScopedFlushDenormals noDenormals;
        const auto t0 = std::chrono::steady_clock::now();
        if (resetMetersFlag) { resetMetersFlag = false; loud.reset(); tpMax = 0; }

        const float* inL = inputs[0]; const float* inR = inputs[1];
        float* outL = outputs[0]; float* outR = outputs[1];
        const int osf = osFactor();
        const int osPadLen = osf == 1 ? kOSLatency : (osf == 2 ? kOSLatency - 32 : 0);
        const bool limOn = p[kLimit] > 0.5f, tpOn = p[kTruePeak] > 0.5f, hpfOn = p[kHpfSc] > 0.5f;
        const bool msOn = p[kMS] > 0.5f, unity = p[kUnity] > 0.5f, delta = p[kDelta] > 0.5f, dither = p[kDither] > 0.5f;
        const int streamIdx = (int)(p[kStream] + 0.5f);
        const float relCoef = std::exp(-1.f / (float)(fs * relMs * 0.001));
        const float avgCoef = std::exp(-1.f / (float)(fs * 0.3));
        const bool autoRel = p[kAutoRel] > 0.5f;
        for (int c = 0; c < 2; ++c) avg[c].setWindow(attSamp);

        float pkIn[2] = {0,0}, pkOut[2] = {0,0}, minGain = 1.f;
        clipMinRatio = 1.f;

        for (uint32_t i = 0; i < frames; ++i)
        {
            const float dryL = inL[i], dryR = inR[i];
            const float gIn = gainSm.next(), drive = driveSm.next(), width = widthSm.next();
            float l = dryL * gIn, r = dryR * gIn;
            pkIn[0] = std::max(pkIn[0], std::fabs(l)); pkIn[1] = std::max(pkIn[1], std::fabs(r));

            // stereo width
            { float m = 0.5f * (l + r), s = 0.5f * (l - r) * width; l = m + s; r = m - s; }

            // punch (transient shaper on linked envelope)
            if (punch > 0.f) {
                float a = 0.5f * (std::fabs(l) + std::fabs(r));
                float ef = punchFast.process(a), es = punchSlow.process(a);
                float ratio = es > 1e-6f ? ef / es : 1.f;
                float boost = 1.f + punch * std::min(std::max(ratio - 1.f, 0.f), 2.f) * 0.6f;
                l *= boost; r *= boost;
            }

            const float preL = l, preR = r;

            // nonlinear stage with oversampling
            if (osf == 1) {
                nonlinear(l, r, drive);
            } else if (osf == 2) {
                float a0, a1, b0, b1;
                up1[0].process(l, a0, a1); up1[1].process(r, b0, b1);
                nonlinear(a0, b0, drive); nonlinear(a1, b1, drive);
                l = dn1[0].process(a0, a1); r = dn1[1].process(b0, b1);
            } else {
                float a[4], b[4], t0a, t1a, t0b, t1b;
                up1[0].process(l, t0a, t1a); up1[1].process(r, t0b, t1b);
                up2[0].process(t0a, a[0], a[1]); up2[0].process(t1a, a[2], a[3]);
                up2[1].process(t0b, b[0], b[1]); up2[1].process(t1b, b[2], b[3]);
                for (int k = 0; k < 4; ++k) nonlinear(a[k], b[k], drive);
                float d0a = dn2[0].process(a[0], a[1]), d1a = dn2[0].process(a[2], a[3]);
                float d0b = dn2[1].process(b[0], b[1]), d1b = dn2[1].process(b[2], b[3]);
                l = dn1[0].process(d0a, d1a); r = dn1[1].process(d0b, d1b);
            }
            l = osPad[0].process(l, osPadLen); r = osPad[1].process(r, osPadLen);

            // ---- limiter ----
            float x[2] = { l, r };
            if (msOn) { float m = 0.5f * (l + r), s = 0.5f * (l - r); x[0] = m; x[1] = s; }

            float det[2], lowShare[2] = { 0.f, 0.f };
            for (int c = 0; c < 2; ++c) {
                det[c] = tpOn ? tpDet[c].process(x[c]) : std::fabs(detDelay[c].process(x[c], kTPLatency));
                if (hpfOn) {
                    const float hp = scHP[c].process(x[c]);
                    const float eAll = scEnvAll[c].process(std::fabs(x[c]));
                    const float eLow = scEnvLow[c].process(std::fabs(x[c] - hp));
                    lowShare[c] = eAll > 1e-6f ? std::min(1.f, eLow / eAll) : 0.f;
                }
            }
            if (msOn) {
                // M/S: both chains must keep L/R (= |M|+|S|) under the ceiling
                const float lr = det[0] + det[1];
                det[0] = det[1] = lr;
            }
            const float dmax = std::max(det[0], det[1]);
            float y[2], gS = 1.f;
            for (int c = 0; c < 2; ++c) {
                const float pk = link * dmax + (1.f - link) * det[c];
                const float req = pk > ceilLin ? ceilLin / pk : 1.f;
                const float m = minWin[c].process(req, laSamp);
                const float s = avg[c].process(m);
                if (s < g[c]) g[c] = s;
                else {
                    float rt = relMs;
                    if (autoRel) {
                        const float cur = linToDb(g[c]);
                        avgGrDb[c] = cur + (avgGrDb[c] - cur) * avgCoef;
                        const float factor = cur < -0.01f ? std::min(1.f, std::max(0.f, avgGrDb[c] / cur)) : 1.f;
                        rt *= 0.3f + 1.2f * factor;
                    }
                    if (hpfOn) rt *= 1.f - 0.75f * lowShare[c];   // sub-driven GR recovers fast (anti-pumping)
                    if (msOn && c == 1) rt *= 0.5f;               // side recovers faster -> keeps width
                    const float rc = (rt == relMs && !autoRel) ? relCoef : std::exp(-1.f / (float)(fs * rt * 0.001));
                    g[c] = s + (g[c] - s) * rc;
                }
                const float gd = gainDelay[c].process(g[c], laMax - laSamp);
                const float xd = audioDelay[c].process(x[c], dLim);
                y[c] = limOn ? xd * gd : xd;
                if (limOn) { minGain = std::min(minGain, gd); gS = std::min(gS, gd); }
            }
            if (msOn) { float m = y[0], s = y[1]; y[0] = m + s; y[1] = m - s; }
            if (limOn) { y[0] = hardClip(y[0], ceilLin); y[1] = hardClip(y[1], ceilLin); }

            if (unity) { float u = 1.f / std::max(1e-6f, gIn * drive); y[0] *= u; y[1] *= u; }

            // ---- meters on output ----
            pkOut[0] = std::max(pkOut[0], std::fabs(y[0])); pkOut[1] = std::max(pkOut[1], std::fabs(y[1]));
            loud.process(y[0], y[1]);
            tpMax = std::max(tpMax, std::max(tpOut[0].process(y[0]), tpOut[1].process(y[1])));

            // history for waveform display
            histPeak = std::max(histPeak, std::max(std::fabs(y[0]), std::fabs(y[1])));
            histGr = std::min(histGr, linToDb(gS));
            if (++histCount >= histLen) {
                int wp = histPos;
                history.peak[wp] = histPeak; history.gr[wp] = histGr;
                histPos = (wp + 1) % kHistoryLen; history.writePos = histPos;
                histPeak = 0; histGr = 0; histCount = 0;
            }

            // delta: what the clipper/limiter removed
            const float pdL = preDelay[0].process(preL, totalLatency);
            const float pdR = preDelay[1].process(preR, totalLatency);
            if (delta) { y[0] = tapeClip(pdL - y[0], 1.f); y[1] = tapeClip(pdR - y[1], 1.f); }

            // streaming preview (only turns down)
            if (streamIdx > 0) {
                float ref = loud.integrated > -70.f ? loud.integrated : loud.shortTerm;
                streamSm.target = ref > -70.f ? std::min(1.f, dbToLin(kStreamTargets[streamIdx] - ref)) : 1.f;
            } else streamSm.target = 1.f;
            const float sg = streamSm.next();
            y[0] *= sg; y[1] *= sg;

            if (dither) {
                const float lsb = 1.f / 32768.f;
                y[0] += (dist(rng) - dist(rng)) * lsb;
                y[1] += (dist(rng) - dist(rng)) * lsb;
            }

            // bypass crossfade (latency compensated)
            const float ddL = dryDelay[0].process(dryL, totalLatency);
            const float ddR = dryDelay[1].process(dryR, totalLatency);
            float bp = bypassSm.next();
            if (bp > 0.9999f) bp = 1.f; else if (bp < 0.0001f) bp = 0.f;
            outL[i] = flushTiny(y[0] + (ddL - y[0]) * bp);
            outR[i] = flushTiny(y[1] + (ddR - y[1]) * bp);
        }

        // ---- output parameters ----
        float* o = out;
        o[kOutInL - kNumInputParams] = linToDb(pkIn[0]);
        o[kOutInR - kNumInputParams] = linToDb(pkIn[1]);
        o[kOutOutL - kNumInputParams] = linToDb(pkOut[0]);
        o[kOutOutR - kNumInputParams] = linToDb(pkOut[1]);
        o[kOutGR - kNumInputParams] = std::max(-60.f, linToDb(minGain));
        o[kOutClipGR - kNumInputParams] = std::max(-60.f, linToDb(clipMinRatio));
        o[kOutLufsM - kNumInputParams] = loud.momentary;
        o[kOutLufsS - kNumInputParams] = loud.shortTerm;
        o[kOutLufsI - kNumInputParams] = loud.integrated;
        o[kOutLRA - kNumInputParams] = loud.lra;
        o[kOutTP - kNumInputParams] = linToDb(tpMax);

        const double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        const double avail = frames / fs;
        if (avail > 0) cpuAvg = cpuAvg * 0.9 + 0.1 * (100.0 * el / avail);
        o[kOutCPU - kNumInputParams] = (float)cpuAvg;
    }

private:
    float p[kNumInputParams];
    float out[kParamCount - kNumInputParams] = {};
    bool resetMetersFlag = false;

    double fs = 48000, lastFsOs = 0;
    int laMax = 241, dLim = 246, totalLatency = 294, laSamp = 72, attSamp = 24, histLen = 500;
    float ceilLin = 1, relMs = 80, link = 1, sat = 0, amt808 = 0, punch = 0;
    float relMul = 1, punchAdd = 0, satAdd = 0, attMul = 1;
    int clipShape = 0;

    Delay dryDelay[2], preDelay[2], osPad[2], audioDelay[2], detDelay[2], gainDelay[2];
    SlidingMin minWin[2];
    MovingAvg avg[2];
    float g[2] = {1, 1}, avgGrDb[2] = {0, 0};
    TruePeak tpDet[2], tpOut[2];
    EnvFollower scEnvAll[2], scEnvLow[2];
    Biquad scHP[2], xLP[2][2], xHP[2][2];
    DCBlock dc808[2], dcTape[2];
    Upsampler2 up1[2], up2[2];
    Downsampler2 dn1[2], dn2[2];
    EnvFollower punchFast, punchSlow;
    Smooth gainSm, driveSm, widthSm, streamSm, bypassSm;
    LoudnessMeter loud;
    float tpMax = 0;
    float clipMinRatio = 1.f;
    int histPos = 0, histCount = 0; float histPeak = 0, histGr = 0;
    double cpuAvg = 0;
    std::minstd_rand rng { 12345 };
    std::uniform_real_distribution<float> dist { 0.f, 1.f };

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrapLimitPlugin)
};

Plugin* createPlugin() { return new TrapLimitPlugin(); }

END_NAMESPACE_DISTRHO

DisplayHistory* tlGetHistory(void* ptr)
{
    if (!ptr) return nullptr;
    return &static_cast<DISTRHO_NAMESPACE::TrapLimitPlugin*>(static_cast<DISTRHO_NAMESPACE::Plugin*>(ptr))->history;
}
