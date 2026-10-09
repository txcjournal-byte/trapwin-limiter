// DSP building blocks for Trap Limit
#pragma once
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>

namespace tl {

static inline float dbToLin(float db) { return std::pow(10.f, db * 0.05f); }
static inline float linToDb(float l)  { return l > 1e-9f ? 20.f * std::log10(l) : -180.f; }

// ------------------------------------------------------------------ Biquad
struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    inline float process(float xf) {
        double x = xf;
        double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float)y;
    }
    void setLowpass(double fs, double f, double q) {
        double w = 2 * M_PI * f / fs, c = std::cos(w), s = std::sin(w), al = s / (2 * q);
        double a0 = 1 + al;
        b0 = (1 - c) / 2 / a0; b1 = (1 - c) / a0; b2 = b0;
        a1 = -2 * c / a0; a2 = (1 - al) / a0;
    }
    void setHighpass(double fs, double f, double q) {
        double w = 2 * M_PI * f / fs, c = std::cos(w), s = std::sin(w), al = s / (2 * q);
        double a0 = 1 + al;
        b0 = (1 + c) / 2 / a0; b1 = -(1 + c) / a0; b2 = b0;
        a1 = -2 * c / a0; a2 = (1 - al) / a0;
    }
    // ITU BS.1770 K-weighting stages
    void setKShelf(double fs) {
        const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
        double K = std::tan(M_PI * f0 / fs);
        double Vh = std::pow(10.0, G / 20.0), Vb = std::pow(Vh, 0.4996667741545416);
        double a0 = 1.0 + K / Q + K * K;
        b0 = (Vh + Vb * K / Q + K * K) / a0;
        b1 = 2.0 * (K * K - Vh) / a0;
        b2 = (Vh - Vb * K / Q + K * K) / a0;
        a1 = 2.0 * (K * K - 1.0) / a0;
        a2 = (1.0 - K / Q + K * K) / a0;
    }
    void setKHighpass(double fs) {
        const double f0 = 38.13547087602444, Q = 0.5003270373238773;
        double K = std::tan(M_PI * f0 / fs);
        double a0 = 1.0 + K / Q + K * K;
        b0 = 1.0; b1 = -2.0; b2 = 1.0;
        a1 = 2.0 * (K * K - 1.0) / a0;
        a2 = (1.0 - K / Q + K * K) / a0;
    }
};

// ------------------------------------------------------------------ Delay line
struct Delay {
    std::vector<float> buf;
    int pos = 0, len = 1;
    void init(int maxLen) { buf.assign(std::max(maxLen, 1) + 1, 0.f); len = (int)buf.size(); pos = 0; }
    void clear() { std::fill(buf.begin(), buf.end(), 0.f); }
    void fill(float v) { std::fill(buf.begin(), buf.end(), v); }
    // push x, return sample delayed by d (0 <= d < len)
    inline float process(float x, int d) {
        buf[pos] = x;
        int r = pos - d; if (r < 0) r += len;
        float y = buf[r];
        if (++pos >= len) pos = 0;
        return y;
    }
};

// ------------------------------------------------------------------ FIR design helpers
static inline double besselI0(double x) {
    double s = 1, t = 1;
    for (int k = 1; k < 30; ++k) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; }
    return s;
}
static inline void designLowpass(std::vector<double>& h, int N, double fcNorm /*0..0.5 of fs*/, double beta) {
    h.resize(N);
    double M = (N - 1) / 2.0, sum = 0;
    for (int n = 0; n < N; ++n) {
        double m = n - M;
        double sinc = (m == 0) ? 2 * fcNorm : std::sin(2 * M_PI * fcNorm * m) / (M_PI * m);
        double r = m / M;
        double w = besselI0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / besselI0(beta);
        h[n] = sinc * w; sum += h[n];
    }
    for (auto& v : h) v /= sum;
}

// ------------------------------------------------------------------ Halfband 2x up / down (65 taps)
// Latency: 32 samples at the high rate for each filter.
static const int kHBTaps = 65;
struct HalfbandCoeffs {
    double h[kHBTaps];
    HalfbandCoeffs() {
        std::vector<double> v; designLowpass(v, kHBTaps, 0.25, 9.0);
        for (int i = 0; i < kHBTaps; ++i) h[i] = v[i];
    }
};
static const HalfbandCoeffs& hbCoeffs() { static HalfbandCoeffs c; return c; }

struct Upsampler2 {
    float hist[kHBTaps] = {}; int p = 0;   // input history (base rate), circular
    void reset() { std::memset(hist, 0, sizeof(hist)); p = 0; }
    // produces two high-rate samples. Output delay = 32 high-rate samples
    inline void process(float x, float& y0, float& y1) {
        const double* h = hbCoeffs().h;
        p = (p + 1) % kHBTaps; hist[p] = x;
        double s0 = 0, s1 = 0;
        // zero-stuffed signal u[2n]=x[n], u[2n+1]=0; y[m] = sum h[k] u[m-k]
        // even output m=2n: k even -> h[2j]*x[n-j]; odd output m=2n+1: k odd -> h[2j+1]*x[n-j]
        int idx = p;
        for (int j = 0; j < (kHBTaps + 1) / 2; ++j) {
            float xv = hist[idx];
            s0 += h[2 * j] * xv;
            if (2 * j + 1 < kHBTaps) s1 += h[2 * j + 1] * xv;
            idx = (idx == 0) ? kHBTaps - 1 : idx - 1;
        }
        y0 = (float)(2.0 * s0); y1 = (float)(2.0 * s1);
    }
};

struct Downsampler2 {
    float hist[kHBTaps * 2] = {}; int p = 0; // high-rate history
    void reset() { std::memset(hist, 0, sizeof(hist)); p = 0; }
    inline void push(float x) { p = (p + 1) % kHBTaps; hist[p] = x; }
    // consume two high-rate samples, return one base-rate sample
    // output is the filtered value at the even (first) input index -> exact 32-sample high-rate delay
    inline float process(float x0, float x1) {
        push(x0);
        const double* h = hbCoeffs().h;
        double s = 0; int idx = p;
        for (int k = 0; k < kHBTaps; ++k) {
            s += h[k] * hist[idx];
            idx = (idx == 0) ? kHBTaps - 1 : idx - 1;
        }
        push(x1);
        return (float)s;
    }
};

// ------------------------------------------------------------------ True peak (4x polyphase, 49 taps)
// Detection delay: 6 base-rate samples.
static const int kTPTaps = 49;
struct TPCoeffs {
    double h[kTPTaps];
    TPCoeffs() {
        std::vector<double> v; designLowpass(v, kTPTaps, 0.125 * 0.9, 7.0);
        for (int i = 0; i < kTPTaps; ++i) h[i] = v[i] * 4.0;
    }
};
static const TPCoeffs& tpCoeffs() { static TPCoeffs c; return c; }

struct TruePeak {
    float hist[13] = {}; int p = 0;
    void reset() { std::memset(hist, 0, sizeof(hist)); p = 0; }
    inline float process(float x) {
        const double* h = tpCoeffs().h;
        p = (p + 1) % 13; hist[p] = x;
        float mx = 0.f;
        for (int ph = 0; ph < 4; ++ph) {
            double s = 0; int idx = p;
            for (int k = 0; 4 * k + ph < kTPTaps; ++k) {
                s += h[4 * k + ph] * hist[idx];
                idx = (idx == 0) ? 12 : idx - 1;
            }
            float a = (float)std::fabs(s);
            if (a > mx) mx = a;
        }
        return mx;
    }
};

// ------------------------------------------------------------------ Envelope follower
struct EnvFollower {
    float att = 0, rel = 0, env = 0;
    void set(double fs, double attMs, double relMs) {
        att = (float)std::exp(-1.0 / (fs * attMs * 0.001));
        rel = (float)std::exp(-1.0 / (fs * relMs * 0.001));
    }
    inline float process(float x) {
        float c = x > env ? att : rel;
        env = x + (env - x) * c;
        return env;
    }
};

// ------------------------------------------------------------------ Sliding minimum (monotonic deque)
struct SlidingMin {
    std::vector<float> val; std::vector<long long> idx;
    int head = 0, tail = 0, cap = 1; long long n = 0;
    void init(int maxWin) { cap = maxWin + 2; val.assign(cap, 1.f); idx.assign(cap, 0); head = tail = 0; n = 0; }
    void clear() { head = tail = 0; n = 0; }
    inline float process(float x, int win) {
        // pop back while >= x
        while (tail != head) {
            int b = (tail - 1 + cap) % cap;
            if (val[b] >= x) tail = b; else break;
        }
        val[tail] = x; idx[tail] = n; tail = (tail + 1) % cap;
        while (idx[head] <= n - win) head = (head + 1) % cap;
        ++n;
        return val[head];
    }
};

// ------------------------------------------------------------------ Moving average
struct MovingAvg {
    std::vector<float> buf; int pos = 0, cap = 1, win = 1; double sum = 0;
    void init(int maxWin) { cap = maxWin + 1; buf.assign(cap, 1.f); pos = 0; win = 1; sum = 1.0; }
    void setWindow(int w) {
        w = std::max(1, std::min(w, cap - 1));
        if (w == win) return;
        win = w; recompute();
    }
    void recompute() {
        sum = 0; int i = pos;
        for (int k = 0; k < win; ++k) { i = (i - 1 + cap) % cap; sum += buf[i]; }
    }
    void fill(float v) { std::fill(buf.begin(), buf.end(), v); sum = v * win; }
    inline float process(float x) {
        int old = (pos - win + cap) % cap;
        sum += x - buf[old];
        buf[pos] = x;
        pos = (pos + 1) % cap;
        return (float)(sum / win);
    }
};

// ------------------------------------------------------------------ Loudness meter (BS.1770 / EBU R128)
struct LoudnessMeter {
    static const int kBins = 800;          // -70 .. +10 LU in 0.1 steps
    double fs = 48000;
    Biquad sh[2], hp[2];
    int blockLen = 4800, blockCount = 0;
    double blockAcc = 0;
    double blocks[30] = {}; int blockPos = 0, blocksFilled = 0;
    // histograms
    double iEnergy[kBins] = {}; unsigned iCount[kBins] = {};
    unsigned sCount[kBins] = {}; double sEnergy[kBins] = {};
    float momentary = -100, shortTerm = -100, integrated = -100, lra = 0;
    int lraTick = 0;

    void init(double sr) {
        fs = sr;
        for (int c = 0; c < 2; ++c) { sh[c].setKShelf(sr); hp[c].setKHighpass(sr); }
        blockLen = (int)std::lround(sr * 0.1);
        reset();
    }
    void reset() {
        for (int c = 0; c < 2; ++c) { sh[c].reset(); hp[c].reset(); }
        blockAcc = 0; blockCount = 0; blockPos = 0; blocksFilled = 0;
        std::memset(blocks, 0, sizeof(blocks));
        std::memset(iEnergy, 0, sizeof(iEnergy)); std::memset(iCount, 0, sizeof(iCount));
        std::memset(sEnergy, 0, sizeof(sEnergy)); std::memset(sCount, 0, sizeof(sCount));
        momentary = shortTerm = integrated = -100; lra = 0;
    }
    static inline double toLufs(double e) { return e > 1e-20 ? -0.691 + 10.0 * std::log10(e) : -100.0; }
    static inline int binOf(double l) { int b = (int)std::floor((l + 70.0) * 10.0); return b < 0 ? -1 : (b >= kBins ? kBins - 1 : b); }

    inline void process(float l, float r) {
        float a = hp[0].process(sh[0].process(l));
        float b = hp[1].process(sh[1].process(r));
        blockAcc += (double)a * a + (double)b * b;
        if (++blockCount >= blockLen) endBlock();
    }
    void endBlock() {
        blocks[blockPos] = blockAcc / blockCount;
        blockPos = (blockPos + 1) % 30; if (blocksFilled < 30) ++blocksFilled;
        blockAcc = 0; blockCount = 0;
        // momentary: last 4 blocks
        double m = 0; int nm = std::min(4, blocksFilled);
        for (int k = 1; k <= nm; ++k) m += blocks[(blockPos - k + 30) % 30];
        m /= std::max(1, nm);
        double s = 0;
        for (int k = 1; k <= blocksFilled; ++k) s += blocks[(blockPos - k + 30) % 30];
        s /= std::max(1, blocksFilled);
        momentary = (float)toLufs(m);
        shortTerm = (float)toLufs(s);
        if (nm == 4) {
            double lm = toLufs(m); int b = binOf(lm);
            if (lm > -70.0 && b >= 0) { iEnergy[b] += m; iCount[b]++; }
        }
        if (blocksFilled == 30) {
            double ls = toLufs(s); int b = binOf(ls);
            if (ls > -70.0 && b >= 0) { sEnergy[b] += s; sCount[b]++; }
        }
        computeIntegrated();
        if (++lraTick >= 10) { lraTick = 0; computeLRA(); }
    }
    void computeIntegrated() {
        double e = 0; unsigned long long n = 0;
        for (int b = 0; b < kBins; ++b) { e += iEnergy[b]; n += iCount[b]; }
        if (n == 0) { integrated = -100; return; }
        double gate = toLufs(e / n) - 10.0;
        int gb = std::max(0, binOf(gate));
        e = 0; n = 0;
        for (int b = gb; b < kBins; ++b) { e += iEnergy[b]; n += iCount[b]; }
        integrated = n ? (float)toLufs(e / n) : -100.f;
    }
    void computeLRA() {
        double e = 0; unsigned long long n = 0;
        for (int b = 0; b < kBins; ++b) { e += sEnergy[b]; n += sCount[b]; }
        if (n < 2) { lra = 0; return; }
        double gate = toLufs(e / n) - 20.0;
        int gb = std::max(0, binOf(gate));
        unsigned long long tot = 0;
        for (int b = gb; b < kBins; ++b) tot += sCount[b];
        if (tot < 2) { lra = 0; return; }
        unsigned long long lo = (unsigned long long)(tot * 0.10), hi = (unsigned long long)(tot * 0.95);
        unsigned long long acc = 0; int bl = gb, bh = gb; bool fl = false;
        for (int b = gb; b < kBins; ++b) {
            acc += sCount[b];
            if (!fl && acc > lo) { bl = b; fl = true; }
            if (acc >= hi) { bh = b; break; }
        }
        lra = (float)((bh - bl) * 0.1);
    }
};

// ------------------------------------------------------------------ Waveshapers
static inline float softClip(float x, float t) {
    float k = 0.6f * t, a = std::fabs(x);
    if (a <= k) return x;
    float y = k + (t - k) * std::tanh((a - k) / (t - k));
    return x < 0 ? -y : y;
}
static inline float hardClip(float x, float t) { return x > t ? t : (x < -t ? -t : x); }
static inline float tapeClip(float x, float t) { return t * std::tanh(x / t); }

// simple DC blocker
struct DCBlock {
    float x1 = 0, y1 = 0, R = 0.999f;
    void set(double fs, double f) { R = (float)std::exp(-2 * M_PI * f / fs); }
    void reset() { x1 = y1 = 0; }
    inline float process(float x) { float y = x - x1 + R * y1; x1 = x; y1 = y; return y; }
};

// smoothed parameter
struct Smooth {
    float cur = 0, target = 0, coef = 0.999f;
    void set(double fs, double ms) { coef = (float)std::exp(-1.0 / (fs * ms * 0.001)); }
    void reset(float v) { cur = target = v; }
    inline float next() { cur = target + (cur - target) * coef; return cur; }
};

} // namespace tl
