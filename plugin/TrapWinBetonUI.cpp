// TRAPWIN TW-2 True Peak Limiter - "BETON" concrete rack UI (NanoVG)
#include "DistrhoUI.hpp"
#include "Params.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <cctype>
#include <vector>
#include <algorithm>
#ifdef _WIN32
# include <windows.h>
#else
# include <sys/stat.h>
# include <dirent.h>
#endif
#ifndef TL_MOCK
# include "res/ConcretePng.h"
# include "res/MonoBold.h"
# include "res/SerifBold.h"
#endif

START_NAMESPACE_DISTRHO

namespace {
const float W = 1980.f, H = 356.f;
const float EAR_R = W - 100.f;
const float PI = 3.14159265f;

struct Rect { float x, y, w, h; bool hit(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; } };

double nowSec()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

// ---- layout ----
const float WIN_X0 = 973.f, WIN_X1 = 1450.f;           // centre window
struct KnobDef { int id; float cx, cy, r; const char* name; };
const KnobDef kKnobs[] = {
    { kGain,    315.f, 145.f, 56.f, "INPUT"   },
    { kCeiling, 570.f, 145.f, 56.f, "CEILING" },
    { kDrive,   760.f,  92.f, 24.f, "DRIVE"   },
    { kRelease, 760.f, 238.f, 24.f, "RELEASE" },
    { kSat,    1688.f,  92.f, 24.f, "SAT"     },
    { k808,    1688.f, 238.f, 24.f, "808 BOOST" },
};
const int kNumKnobs = 6;
const float BTN_W = 56.f, BTN_H = 44.f;
const float BTN_Y[4] = { 100.f, 152.f, 204.f, 256.f };
const float CLIP_X = 905.f, MODE_X = 1515.f;
const char* const kClipLabels[4] = { "TAPE", "HARD", "SOFT", "OFF" };
const char* const kModeLabels[4] = { "TRANS", "PUNCH", "LOUD", "DIRTY" };
const float TOG_X = 1832.f;

// settings page items
enum ItemKind { IK_Cont, IK_Bool, IK_Cycle, IK_Action };
struct Item { const char* name; int id; ItemKind kind; int n; };
const Item kItems[16] = {
    { "LOOKAHEAD", kLookahead, IK_Cont, 0 }, { "ATTACK", kAttack, IK_Cont, 0 },
    { "STEREO LINK", kLink, IK_Cont, 0 },    { "WIDTH", kStereo, IK_Cont, 0 },
    { "PUNCH", kPunch, IK_Cont, 0 },         { "AUTO RELEASE", kAutoRel, IK_Bool, 0 },
    { "LIMITER", kLimit, IK_Bool, 0 },       { "OVERSAMPLE", kOversample, IK_Cycle, 3 },
    { "SUB SPLIT", kSubSplit, IK_Bool, 0 },  { "HPF SIDECHAIN", kHpfSc, IK_Bool, 0 },
    { "MID / SIDE", kMS, IK_Bool, 0 },       { "DITHER 16 BIT", kDither, IK_Bool, 0 },
    { "STREAM PREVIEW", kStream, IK_Cycle, 4 }, { "0 dB MATCH", kUnity, IK_Bool, 0 },
    { "DELTA", kDelta, IK_Bool, 0 },         { "RESET METERS", -1, IK_Action, 0 },
};
} // namespace

class TrapWinBetonUI : public UI
{
public:
    TrapWinBetonUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        loadSharedResources();
#ifndef TL_MOCK
#endif
        for (int i = 0; i < kNumInputParams; ++i) vals[i] = kParamInfo[i].def;
        for (int i = kNumInputParams; i < kParamCount; ++i) vals[i] = -100.f;
        vals[kOutGR] = 0.f; vals[kOutClipGR] = 0.f; vals[kOutLRA] = 0.f; vals[kOutCPU] = 0.f; vals[kOutLatency] = 0.f;
        // HiDPI fix (v2.4): on Windows with display scaling (125 %, 150 %...) the host creates the
        // native window at DEFAULT_SIZE * scaleFactor, but the widget stayed at the unscaled
        // DEFAULT_SIZE. Drawing still filled the window, yet mouse coordinates arrived in the
        // larger window space, so every click was mapped to the wrong place (nothing reacted,
        // no blue click ring) until the user resized the window by hand.
        // Size the widget in real pixels right away so drawing and mouse events use one scale.
        const double sf = getScaleFactor();
        setGeometryConstraints((uint)(990 * sf), (uint)(178 * sf), true);   // min = 50 % of the 1980x356 design, still readable
        if (sf != 1.0)
            setSize((uint)(DISTRHO_UI_DEFAULT_WIDTH * sf), (uint)(DISTRHO_UI_DEFAULT_HEIGHT * sf));
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index < kParamCount) vals[index] = value;
        repaint();
    }
    void programLoaded(uint32_t) override { repaint(); }
    void uiIdle() override { repaint(); }

    // ------------------------------------------------------------ helpers
    float scaleF() const { return getWidth() / W; }

    void setParam(int id, float v)
    {
        const ParamInfo& pi = kParamInfo[id];
        v = std::fmax(pi.min, std::fmin(pi.max, v));
        if (pi.kind != PK_Float) v = std::round(v);
        vals[id] = v;
        setParameterValue(id, v);
    }
    void setParamGesture(int id, float v) { editParameter(id, true); setParam(id, v); editParameter(id, false); }

    float toNorm(int id, float v) const
    {
        const ParamInfo& pi = kParamInfo[id];
        if (pi.log) return std::log(v / pi.min) / std::log(pi.max / pi.min);
        return (v - pi.min) / (pi.max - pi.min);
    }
    float fromNorm(int id, float n) const
    {
        const ParamInfo& pi = kParamInfo[id];
        n = std::fmax(0.f, std::fmin(1.f, n));
        if (pi.log) return pi.min * std::pow(pi.max / pi.min, n);
        return pi.min + n * (pi.max - pi.min);
    }
    void formatValue(int id, char* buf, size_t n) const
    {
        const ParamInfo& pi = kParamInfo[id];
        const float v = vals[id];
        if (pi.kind == PK_Bool) { std::snprintf(buf, n, "%s", v > 0.5f ? "ON" : "OFF"); return; }
        if (id == kOversample) { std::snprintf(buf, n, "%s", kOversampleNames[(int)(v + 0.5f)]); return; }
        if (id == kStream) { std::snprintf(buf, n, "%s", kStreamNames[(int)(v + 0.5f)]); return; }
        if (!std::strcmp(pi.unit, "dB"))      std::snprintf(buf, n, "%+.1f dB", v);
        else if (!std::strcmp(pi.unit, "ms")) { if (v < 10.f) std::snprintf(buf, n, "%.2f ms", v); else std::snprintf(buf, n, "%.0f ms", v); }
        else if (!std::strcmp(pi.unit, "%"))  std::snprintf(buf, n, "%.0f %%", v);
        else std::snprintf(buf, n, "%.0f", v);
    }
    void toast(const char* msg) { std::snprintf(toastMsg, sizeof(toastMsg), "%s", msg); toastTime = nowSec(); }

    // ------------------------------------------------------------ text / shapes
    void font() { fontMono(); }
    void fontMono() {
#ifndef TL_MOCK
        if (fontMonoId >= 0) fontFaceId(fontMonoId);
#else
        fontFace("mono-bold");
#endif
    }
    void fontLogo() {
#ifndef TL_MOCK
        if (fontLogoId >= 0) fontFaceId(fontLogoId);
#else
        fontFace("serif-bold");
#endif
    }
    void rrect(float x, float y, float w, float h, float r) { beginPath(); roundedRect(x, y, w, h, r); }
    void label(float x, float y, const char* s, float size, Color c, int align = ALIGN_CENTER | ALIGN_MIDDLE, float spacing = 0.f)
    {
        font(); fontSize(size); textLetterSpacing(spacing); fillColor(c); textAlign(align); text(x, y, s, nullptr);
        textLetterSpacing(0.f);
    }
    // engraved panel text: light shadow below, dark text
    void engraved(float x, float y, const char* s, float size, float spacing = 0.f, int align = ALIGN_CENTER | ALIGN_MIDDLE)
    {
        label(x, y + 1.2f, s, size, Color(255, 255, 255, 0.55f), align, spacing);
        label(x, y, s, size, Color(28, 31, 36), align, spacing);
    }
    void glowLabel(float x, float y, const char* s, float size, Color c, int align = ALIGN_CENTER | ALIGN_MIDDLE)
    {
        font(); fontSize(size); textAlign(align);
        fontBlur(7.f); fillColor(Color(c.red, c.green, c.blue, 0.6f)); text(x, y, s, nullptr);
        fontBlur(0.f); fillColor(c); text(x, y, s, nullptr);
    }

    void brushed(float x, float y, float w, float h, Color top, Color bottom, unsigned seed)
    {
        beginPath(); rect(x, y, w, h); fillPaint(linearGradient(0, y, 0, y + h, top, bottom)); fill();
        unsigned s = seed;
        for (float yy = y + 1; yy < y + h; yy += 2.3f) {
            s = s * 1664525u + 1013904223u;
            const float r = (s >> 8) / 16777216.f;
            beginPath(); rect(x, yy, w, 1.f);
            fillColor(r > 0.5f ? Color(255, 255, 255, 0.05f + 0.08f * (r - 0.5f)) : Color(0, 0, 0, 0.025f + 0.05f * r));
            fill();
        }
    }

    // ================================================================ BETON (concrete) theme
    const float RAIL = 58.f;

    void monoLabel(float x, float y, const char* s, float size, Color c, int align = ALIGN_CENTER | ALIGN_MIDDLE, float spacing = 0.f)
    {
        fontMono(); fontSize(size); textLetterSpacing(spacing); fillColor(c); textAlign(align); text(x, y, s, nullptr);
        textLetterSpacing(0.f);
    }
    // printed (pad-printed ink on concrete)
    void ink(float x, float y, const char* s, float size, float spacing = 0.f, int align = ALIGN_CENTER | ALIGN_MIDDLE)
    {
        monoLabel(x, y + 1.f, s, size, Color(255, 255, 255, 0.35f), align, spacing);
        monoLabel(x, y, s, size, Color(34, 34, 36), align, spacing);
    }

    void railScrew(float cx, float cy)
    {
        beginPath(); circle(cx, cy, 12); fillColor(Color(18, 19, 21)); fill();
        beginPath(); circle(cx, cy, 10);
        fillPaint(radialGradient(cx - 3, cy - 3, 1, 12, Color(150, 152, 156), Color(62, 64, 68))); fill();
        strokeColor(Color(25, 26, 28)); strokeWidth(2.4f); lineCap(ROUND);
        beginPath(); moveTo(cx - 6, cy + 2.5f); lineTo(cx + 6, cy - 2.5f); stroke();
    }

    void ledBlue(float cx, float cy, bool on)
    {
        if (on) {
            beginPath(); circle(cx, cy, 22); fillPaint(radialGradient(cx, cy, 4, 22, Color(60, 130, 255, 0.55f), Color(60, 130, 255, 0.f))); fill();
        }
        beginPath(); circle(cx, cy, 9); fillColor(Color(20, 22, 26)); fill();
        beginPath(); circle(cx, cy, 7);
        fillPaint(radialGradient(cx - 2, cy - 2, 1, 8, on ? Color(200, 225, 255) : Color(70, 80, 100), on ? Color(30, 100, 255) : Color(20, 28, 45))); fill();
    }

    // tiny deterministic random (for wear / dirt, stable between frames)
    static float rnd(unsigned& s) { s = s * 1664525u + 1013904223u; return (s >> 8) / 16777216.f; }

    // soft contact shadow on the concrete
    void dropShadow(float x, float y, float w, float h, float r, float dy, float blur, float a)
    {
        rrect(x - blur, y + dy - blur * 0.5f, w + blur * 2, h + blur * 2, r + blur);
        fillPaint(boxGradient(x, y + dy, w, h, r, blur * 1.6f, Color(0, 0, 0, a), Color(0, 0, 0, 0.f))); fill();
    }

    // vertical pill switch (black body, white knob) - 3D
    void pillV(float cx, float cy, bool up)
    {
        const float w = 40, h = 70, x = cx - w / 2, y = cy - h / 2;
        dropShadow(x, y, w, h, w / 2, 6, 8, 0.55f);
        // recessed slot in the concrete
        rrect(x - 3, y - 3, w + 6, h + 6, w / 2 + 3); fillPaint(linearGradient(0, y - 3, 0, y + h + 3, Color(0, 0, 0, 0.35f), Color(255, 255, 255, 0.45f))); fill();
        rrect(x, y, w, h, w / 2); fillPaint(linearGradient(0, y, 0, y + h, Color(10, 10, 12), Color(52, 54, 58))); fill();
        rrect(x, y, w, h, w / 2); fillPaint(boxGradient(x, y, w, h, w / 2, 8, Color(0, 0, 0, 0.f), Color(0, 0, 0, 0.6f))); fill();
        const float ky = up ? y + w / 2 : y + h - w / 2;
        beginPath(); circle(cx + 1, ky + 5, 16); fillPaint(radialGradient(cx + 1, ky + 5, 8, 18, Color(0, 0, 0, 0.7f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); circle(cx, ky + 2, 15); fillColor(Color(150, 152, 156)); fill();                 // side of the knob
        beginPath(); circle(cx, ky, 14); fillPaint(radialGradient(cx - 4, ky - 5, 2, 16, Color(255, 255, 255), Color(196, 198, 202))); fill();
        beginPath(); arc(cx, ky, 12.5f, PI * 1.1f, PI * 1.6f, CW); strokeColor(Color(255, 255, 255, 0.9f)); strokeWidth(1.4f); stroke();
        unsigned sd = (unsigned)(cx * 7 + cy); // a little grime on the knob
        for (int i = 0; i < 3; ++i) { const float ox = (rnd(sd) - 0.5f) * 16, oy = (rnd(sd) - 0.5f) * 16; beginPath(); circle(cx + ox, ky + oy, 2 + rnd(sd) * 3); fillColor(Color(90, 80, 60, 0.12f)); fill(); }
    }

    // horizontal pill switch (blue when on) - 3D
    void pillH(float cx, float cy, bool on)
    {
        const float w = 70, h = 36, x = cx - w / 2, y = cy - h / 2;
        dropShadow(x, y, w, h, h / 2, 6, 8, 0.5f);
        rrect(x - 3, y - 3, w + 6, h + 6, h / 2 + 3); fillPaint(linearGradient(0, y - 3, 0, y + h + 3, Color(0, 0, 0, 0.35f), Color(255, 255, 255, 0.45f))); fill();
        rrect(x, y, w, h, h / 2);
        fillPaint(on ? linearGradient(0, y, 0, y + h, Color(20, 45, 110), Color(50, 95, 190))
                     : linearGradient(0, y, 0, y + h, Color(10, 10, 12), Color(52, 54, 58)));
        fill();
        rrect(x, y, w, h, h / 2); fillPaint(boxGradient(x, y, w, h, h / 2, 7, Color(0, 0, 0, 0.f), Color(0, 0, 0, 0.55f))); fill();
        const float kx = on ? x + w - h / 2 : x + h / 2;
        if (on) { beginPath(); circle(kx, cy, 24); fillPaint(radialGradient(kx, cy, 4, 24, Color(60, 140, 255, 0.55f), Color(60, 140, 255, 0.f))); fill(); }
        beginPath(); circle(kx + 1, cy + 5, 15); fillPaint(radialGradient(kx + 1, cy + 5, 7, 17, Color(0, 0, 0, 0.65f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); circle(kx, cy + 2, 14); fillColor(on ? Color(25, 80, 200) : Color(150, 152, 156)); fill();
        beginPath(); circle(kx, cy, 13);
        fillPaint(radialGradient(kx - 3, cy - 4, 1, 15, on ? Color(190, 225, 255) : Color(255, 255, 255), on ? Color(35, 115, 255) : Color(196, 198, 202))); fill();
        beginPath(); arc(kx, cy, 11.5f, PI * 1.1f, PI * 1.6f, CW); strokeColor(Color(255, 255, 255, 0.85f)); strokeWidth(1.3f); stroke();
    }

    // square push button with real depth: concrete recess, side wall, top face
    void squareButton(float cx, float cy, bool on)
    {
        const float s = 42.f, x = cx - s / 2, y = cy - s / 2 - 1.f;
        const float depth = on ? 2.f : 5.f;           // pressed buttons sit lower
        // recess cut into the concrete
        rrect(x - 4, y - 4, s + 8, s + 8 + 2, 7); fillPaint(linearGradient(0, y - 4, 0, y + s + 6, Color(0, 0, 0, 0.45f), Color(255, 255, 255, 0.5f))); fill();
        rrect(x - 2, y - 2, s + 4, s + 6, 6); fillColor(Color(8, 8, 9)); fill();
        // side wall
        rrect(x, y + depth, s, s, 5); fillColor(on ? Color(15, 55, 160) : Color(6, 6, 7)); fill();
        // top face (lifted by depth)
        const float ty = y + (on ? 1.f : -1.f);
        if (on) {
            rrect(x - 12, ty - 12, s + 24, s + 24, 14);
            fillPaint(boxGradient(x, ty, s, s, 5, 16, Color(50, 120, 255, 0.55f), Color(50, 120, 255, 0.f))); fill();
            rrect(x, ty, s, s, 5); fillPaint(linearGradient(0, ty, 0, ty + s, Color(110, 175, 255), Color(28, 92, 240))); fill();
        } else {
            rrect(x, ty, s, s, 5); fillPaint(linearGradient(0, ty, 0, ty + s, Color(58, 60, 64), Color(18, 19, 21))); fill();
        }
        // bevel: light top edge, dark bottom edge
        rrect(x + 1, ty + 1, s - 2, s - 2, 4.5f); strokePaint(linearGradient(0, ty, 0, ty + s, Color(255, 255, 255, on ? 0.45f : 0.22f), Color(0, 0, 0, 0.45f))); strokeWidth(1.6f); stroke();
        // glossy top half
        rrect(x + 4, ty + 3, s - 8, s * 0.38f, 4); fillColor(Color(255, 255, 255, on ? 0.20f : 0.07f)); fill();
        // subtle wear: scuffed corners + a few specks of dust
        unsigned sd = (unsigned)(cx * 13 + cy * 3);
        for (int i = 0; i < 4; ++i) {
            const float ox = x + 4 + rnd(sd) * (s - 8), oy = ty + 4 + rnd(sd) * (s - 8);
            beginPath(); circle(ox, oy, 0.8f + rnd(sd) * 1.2f); fillColor(Color(200, 190, 170, on ? 0.15f : 0.22f)); fill();
        }
        beginPath(); arc(x + 5, ty + s - 5, 4, PI * 0.5f, PI, CW); strokeColor(Color(200, 195, 185, 0.18f)); strokeWidth(1.2f); stroke();
    }

    static float knobAngleB(float n) { return (135.f + 270.f * n) * PI / 180.f; }

    void betonKnob(float cx, float cy, float r, float n)
    {
        const float h = r > 40 ? 9.f : 6.f;            // visible height of the knob body
        // cast shadow (down-right) + ambient occlusion ring
        beginPath(); circle(cx + 7, cy + h + 9, r + 10);
        fillPaint(radialGradient(cx + 7, cy + h + 9, r * 0.6f, r + 16, Color(0, 0, 0, 0.55f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); circle(cx, cy + h, r + 4);
        fillPaint(radialGradient(cx, cy + h, r - 2, r + 6, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        // cylinder side wall
        beginPath(); rect(cx - r, cy, 2 * r, h); circle(cx, cy + h, r);
        fillPaint(linearGradient(cx - r, 0, cx + r, 0, Color(40, 41, 44), Color(8, 8, 9))); fill();
        // ribs on the side wall
        const int ribs = r > 40 ? 64 : 34;
        for (int i = 0; i < ribs; ++i) {
            const float a = PI * 0.05f + (PI * 0.9f) * i / (ribs / 2);
            if (a > PI * 0.95f) break;
            const float px = cx + std::cos(a) * r;
            beginPath(); moveTo(px, cy + std::sin(a) * r * 0.2f); lineTo(px, cy + h + std::sin(a) * r);
            strokeColor(Color(90, 92, 98, 0.35f)); strokeWidth(1.2f); stroke();
        }
        // top face of the skirt (knurled ring)
        beginPath(); circle(cx, cy, r);
        fillPaint(linearGradient(cx - r * 0.7f, cy - r, cx + r * 0.7f, cy + r, Color(62, 64, 70), Color(14, 14, 16))); fill();
        strokeWidth(r > 40 ? 2.f : 1.4f);
        for (int i = 0; i < ribs; ++i) {
            const float a = i * 2 * PI / ribs;
            const float lit = 0.5f + 0.5f * std::cos(a + PI * 0.75f);   // light from top-left
            beginPath(); moveTo(cx + std::cos(a) * r * 0.79f, cy + std::sin(a) * r * 0.79f); lineTo(cx + std::cos(a) * (r - 1.5f), cy + std::sin(a) * (r - 1.5f));
            strokeColor(Color(150, 152, 158, 0.15f + 0.45f * lit)); stroke();
        }
        // rim highlight / shade
        beginPath(); arc(cx, cy, r - 0.8f, PI * 1.0f, PI * 1.6f, CW); strokeColor(Color(255, 255, 255, 0.35f)); strokeWidth(1.5f); stroke();
        beginPath(); arc(cx, cy, r - 0.8f, PI * 0.0f, PI * 0.6f, CW); strokeColor(Color(0, 0, 0, 0.6f)); strokeWidth(1.5f); stroke();
        // grime packed into the knurling (subtle)
        unsigned sd = (unsigned)(cx * 31 + cy * 7);
        for (int i = 0; i < 7; ++i) {
            const float a = rnd(sd) * 2 * PI, rr = r * (0.82f + rnd(sd) * 0.12f);
            beginPath(); circle(cx + std::cos(a) * rr, cy + std::sin(a) * rr, r * (0.06f + rnd(sd) * 0.06f));
            fillColor(Color(120, 108, 85, 0.10f + rnd(sd) * 0.10f)); fill();
        }
        // raised brushed-metal cap with its own little wall
        const float rc = r * 0.72f, ch = r > 40 ? 4.f : 2.5f;
        beginPath(); circle(cx + 1, cy + ch + 1, rc + 1.5f); fillColor(Color(0, 0, 0, 0.7f)); fill();
        beginPath(); circle(cx, cy + ch, rc); fillPaint(linearGradient(cx - rc, 0, cx + rc, 0, Color(120, 114, 100), Color(60, 56, 48))); fill();
        beginPath(); circle(cx, cy, rc);
        fillPaint(linearGradient(cx - rc, cy - rc, cx + rc, cy + rc, Color(222, 216, 200), Color(112, 106, 94))); fill();
        for (int i = 1; i < 11; ++i) {
            beginPath(); circle(cx, cy, rc * i / 11.f);
            strokeColor(i % 2 ? Color(255, 255, 255, 0.08f) : Color(0, 0, 0, 0.07f)); strokeWidth(1.f); stroke();
        }
        // conical reflections
        beginPath(); moveTo(cx, cy); arc(cx, cy, rc, PI * 1.12f, PI * 1.42f, CW); closePath(); fillColor(Color(255, 255, 255, 0.26f)); fill();
        beginPath(); moveTo(cx, cy); arc(cx, cy, rc, PI * 0.12f, PI * 0.42f, CW); closePath(); fillColor(Color(255, 255, 255, 0.12f)); fill();
        beginPath(); moveTo(cx, cy); arc(cx, cy, rc, PI * 0.62f, PI * 0.88f, CW); closePath(); fillColor(Color(0, 0, 0, 0.10f)); fill();
        beginPath(); arc(cx, cy, rc - 0.6f, PI * 1.0f, PI * 1.65f, CW); strokeColor(Color(255, 255, 255, 0.55f)); strokeWidth(1.2f); stroke();
        // dirt on the cap: a couple of smudges and fine scratches (kept subtle)
        for (int i = 0; i < 3; ++i) {
            const float a = rnd(sd) * 2 * PI, d = rnd(sd) * rc * 0.75f, rad = rc * (0.12f + rnd(sd) * 0.18f);
            const float sx = cx + std::cos(a) * d, sy = cy + std::sin(a) * d;
            beginPath(); circle(sx, sy, rad); fillPaint(radialGradient(sx, sy, 0, rad, Color(70, 60, 45, 0.16f), Color(70, 60, 45, 0.f))); fill();
        }
        strokeWidth(0.7f);
        for (int i = 0; i < 5; ++i) {
            const float a = rnd(sd) * 2 * PI, d = rnd(sd) * rc * 0.8f, len = rc * (0.15f + rnd(sd) * 0.25f), ang = rnd(sd) * PI;
            const float sx = cx + std::cos(a) * d, sy = cy + std::sin(a) * d;
            beginPath(); moveTo(sx, sy); lineTo(sx + std::cos(ang) * len, sy + std::sin(ang) * len);
            strokeColor(i % 2 ? Color(255, 255, 255, 0.22f) : Color(40, 35, 25, 0.20f)); stroke();
        }
        // engraved pointer (slightly worn)
        const float pa = knobAngleB(n);
        lineCap(ROUND);
        strokeColor(Color(255, 255, 255, 0.35f)); strokeWidth(r > 40 ? 4.5f : 3.f);
        beginPath(); moveTo(cx + std::cos(pa) * rc * 0.18f + 0.8f, cy + std::sin(pa) * rc * 0.18f + 1.2f); lineTo(cx + std::cos(pa) * rc * 0.95f + 0.8f, cy + std::sin(pa) * rc * 0.95f + 1.2f); stroke();
        strokeColor(Color(18, 18, 20, 0.92f)); strokeWidth(r > 40 ? 4.f : 2.6f);
        beginPath(); moveTo(cx + std::cos(pa) * rc * 0.18f, cy + std::sin(pa) * rc * 0.18f); lineTo(cx + std::cos(pa) * rc * 0.95f, cy + std::sin(pa) * rc * 0.95f); stroke();
    }

    void betonTicks(float cx, float cy, float r0, float r1, int n)
    {
        strokeColor(Color(34, 34, 36)); lineCap(BUTT);
        for (int i = 0; i < n; ++i) {
            const float a = knobAngleB(i / (float)(n - 1));
            strokeWidth((i == 0 || i == n - 1 || i == (n - 1) / 2) ? 3.f : 2.f);
            beginPath(); moveTo(cx + std::cos(a) * r0, cy + std::sin(a) * r0); lineTo(cx + std::cos(a) * r1, cy + std::sin(a) * r1); stroke();
        }
    }

    void betonBig(const KnobDef& k, const char* lo, const char* mid, const char* hi)
    {
        betonTicks(k.cx, k.cy, k.r + 12, k.r + 24, 11);
        ink(k.cx + std::cos(knobAngleB(0)) * (k.r + 40), k.cy + std::sin(knobAngleB(0)) * (k.r + 40) + 4, lo, 17);
        ink(k.cx + std::cos(knobAngleB(1)) * (k.r + 40), k.cy + std::sin(knobAngleB(1)) * (k.r + 40) + 4, hi, 17);
        ink(k.cx, k.cy - k.r - 38, mid, 17);
        betonKnob(k.cx, k.cy, k.r + 4, toNorm(k.id, vals[k.id]));
        ink(k.cx, 290, k.name, 32, 4);
    }

    void betonSmall(const KnobDef& k, const char* lo, const char* hi)
    {
        betonTicks(k.cx, k.cy, k.r + 9, k.r + 17, 7);
        ink(k.cx + std::cos(knobAngleB(0)) * (k.r + 26) - 4, k.cy + std::sin(knobAngleB(0)) * (k.r + 26) + 6, lo, 15, 0, ALIGN_RIGHT | ALIGN_MIDDLE);
        ink(k.cx + std::cos(knobAngleB(1)) * (k.r + 26) + 4, k.cy + std::sin(knobAngleB(1)) * (k.r + 26) + 6, hi, 15, 0, ALIGN_LEFT | ALIGN_MIDDLE);
        betonKnob(k.cx, k.cy, k.r + 4, toNorm(k.id, vals[k.id]));
        ink(k.cx, k.cy + 62, k.name, 20, 3);
    }

    // ------------------------------------------------------------ main draw
    void initResources()
    {
        if (resInit) return;
        resInit = true;
#ifndef TL_MOCK
        // created lazily inside the first paint, when the GL context is guaranteed to be current
        fontMonoId = createFontFromMemory("mono-bold", res_mono_bold, res_mono_bold_size, false);
        fontLogoId = createFontFromMemory("serif-bold", res_serif_bold, res_serif_bold_size, false);
        concrete = createImageFromMemory(res_concrete_png, res_concrete_png_size, 0);
#endif
    }

    void onNanoDisplay() override
    {
        initResources();
        const float sc = scaleF();
        save();
        scale(sc, sc);
        drawChassis();
        drawLeftSection();
        drawRightSection();
        drawCentre();
        drawTooltip();
        if (nowSec() - lastPressT < 0.4) {   // click feedback ring
            beginPath(); circle(lastPressX, lastPressY, 14); strokeColor(Color(60, 130, 255, 0.8f)); strokeWidth(2.f); stroke();
        }
        restore();
    }

    void drawChassis()
    {
        // black rack body
        beginPath(); rect(0, 0, W, H); fillPaint(linearGradient(0, 0, 0, H, Color(30, 32, 36), Color(10, 11, 13))); fill();
        // concrete face
        const float px = RAIL, py = 8, pw = W - 2 * RAIL, ph = H - 16;
#ifndef TL_MOCK
        if (concrete.isValid()) {
            beginPath(); rect(px, py, pw, ph);
            fillPaint(imagePattern(px, py, pw, ph, 0.f, concrete, 1.f)); fill();
        } else
#endif
        { beginPath(); rect(px, py, pw, ph); fillColor(Color(196, 196, 194)); fill();
#ifdef TL_MOCK
          mockTexture(px, py, pw, ph);
#endif
        }
        // warm tint + edge vignette
        beginPath(); rect(px, py, pw, ph); fillColor(Color(245, 240, 230, 0.16f)); fill();
        beginPath(); rect(px, py, pw, ph); fillPaint(boxGradient(px, py, pw, ph, 4, 60, Color(0, 0, 0, 0.f), Color(0, 0, 0, 0.18f))); fill();
        // bevel
        beginPath(); rect(px, py, pw, 2); fillColor(Color(255, 255, 255, 0.45f)); fill();
        beginPath(); rect(px, py + ph - 2, pw, 2); fillColor(Color(0, 0, 0, 0.35f)); fill();
        beginPath(); rect(px - 3, py, 3, ph); fillColor(Color(0, 0, 0, 0.6f)); fill();
        beginPath(); rect(px + pw, py, 3, ph); fillColor(Color(0, 0, 0, 0.6f)); fill();
        railScrew(RAIL / 2, 62); railScrew(RAIL / 2, H - 62); railScrew(W - RAIL / 2, 62); railScrew(W - RAIL / 2, H - 62);
        monoLabel(W - RAIL / 2, H / 2, "v1.0", 12, Color(120, 124, 130));
    }

    void drawLeftSection()
    {
        const bool on = vals[kBypass] < 0.5f;
        ledBlue(165, 74, on);
        ink(165, 104, "ON", 18, 3);
        pillV(165, 190, on);
        ink(165, 252, "BYPASS", 16, 3);

        betonBig(kKnobs[0], "0", "12", "24");
        betonBig(kKnobs[1], "-12", "-6", "0");
        betonSmall(kKnobs[2], "0", "12");
        betonSmall(kKnobs[3], "5", "1k");

        ink(CLIP_X - 20, 48, "CLIPPER", 21, 3);
        const bool clipOn = vals[kClip] > 0.5f;
        const int shape = (int)(vals[kClipShape] + 0.5f);
        for (int i = 0; i < 4; ++i) {
            const bool lit = (i == 3) ? !clipOn : (clipOn && shape == 2 - i);
            squareButton(CLIP_X, BTN_Y[i], lit);
            ink(CLIP_X - 34, BTN_Y[i] + 1, kClipLabels[i], 16, 1, ALIGN_RIGHT | ALIGN_MIDDLE);
        }
    }

    void drawRightSection()
    {
        ink(MODE_X + 22, 48, "MODE", 21, 3);
        const int mode = (int)(vals[kMode] + 0.5f);
        for (int i = 0; i < 4; ++i) {
            squareButton(MODE_X, BTN_Y[i], mode == i);
            ink(MODE_X + 34, BTN_Y[i] + 1, kModeLabels[i], 16, 1, ALIGN_LEFT | ALIGN_MIDDLE);
        }
        betonSmall(kKnobs[4], "0", "MAX");
        betonSmall(kKnobs[5], "OFF", "MAX");

        ink(TOG_X, 40, "TRUE PEAK", 16, 2);
        ink(TOG_X, 66, "ON", 13, 2);
        pillV(TOG_X, 118, vals[kTruePeak] > 0.5f);
        ink(TOG_X, 170, "OFF", 13, 2);
        ink(TOG_X, 222, "MONO SUB", 16, 2);
        pillH(TOG_X, 260, vals[kMonoSub] > 0.5f);
    }

    // ------------------------------------------------------------ centre: title, meter, logo, presets
    void drawCentre()
    {
        const float cx = (WIN_X0 + WIN_X1) / 2;
        if (overlay != 0) {
            rrect(WIN_X0 + 4, 12, WIN_X1 - WIN_X0 - 8, H - 24, 8); fillColor(Color(0, 0, 0, 0.35f)); fill();
            rrect(WIN_X0 + 6, 10, WIN_X1 - WIN_X0 - 12, H - 24, 8); fillPaint(linearGradient(0, 10, 0, H - 14, Color(30, 32, 36), Color(14, 15, 17))); fill();
            drawOverlay();
        } else {
            ink(cx, 26, "TW-2  \xC2\xB7  TRUE PEAK", 17, 4);
            ink(cx, 48, "LIMITER", 17, 6);
            drawMeter();
            fontLogo(); fontSize(56); textLetterSpacing(2.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(Color(255, 255, 255, 0.4f)); text(cx, 266.5f, "TRAPWIN", nullptr);
            fillColor(Color(22, 22, 24)); text(cx, 265, "TRAPWIN", nullptr); textLetterSpacing(0.f);
            ink(cx, 298, "AUDIO  \xC2\xB7  UNDERGROUND", 13, 6);
            drawPresetBar();
        }
        if (nowSec() - toastTime < 1.6) {
            rrect(WIN_X0 + 40, 150, WIN_X1 - WIN_X0 - 80, 44, 8); fillColor(Color(0, 0, 0, 0.88f)); fill();
            rrect(WIN_X0 + 40, 150, WIN_X1 - WIN_X0 - 80, 44, 8); strokeColor(Color(60, 130, 255)); strokeWidth(1.5f); stroke();
            monoLabel(cx, 172, toastMsg, 17, Color(235, 240, 250));
        }
    }

    void drawLogo(float, float, float) {}

    static float grPos(float d) { return 1.f - std::log10(1.f + d) / std::log10(21.f); }

    void drawMeter()
    {
        grClipDisp = std::fmin(vals[kOutClipGR], grClipDisp + 0.35f); if (grClipDisp > 0.f) grClipDisp = 0.f;
        grLimDisp  = std::fmin(vals[kOutGR],     grLimDisp  + 0.35f); if (grLimDisp > 0.f) grLimDisp = 0.f;
        const float bx = 1026, by = 64, bw = 371, bh = 160;
        // shadow + black frame
        rrect(bx + 3, by + 6, bw, bh, 10); fillColor(Color(0, 0, 0, 0.35f)); fill();
        rrect(bx, by, bw, bh, 10); fillPaint(linearGradient(0, by, 0, by + bh, Color(46, 48, 52), Color(12, 12, 14))); fill();
        const float fx = bx + 12, fy = by + 12, fw = bw - 24, fh = bh - 24;
        rrect(fx, fy, fw, fh, 4);
        fillPaint(linearGradient(0, fy, 0, fy + fh, Color(240, 230, 196), Color(214, 200, 160))); fill();
        rrect(fx, fy, fw, fh, 4);
        fillPaint(radialGradient(fx + fw / 2, fy + fh * 0.8f, 20, fw * 0.6f, Color(255, 248, 215, 0.6f), Color(255, 240, 200, 0.f))); fill();

        save();
        scissor(fx, fy, fw, fh);
        const float cx = fx + fw / 2, cy = fy + fh + 44, R = 122;
        auto pt = [&](float pos, float r, float& px, float& py) {
            const float th = (-46.f + 92.f * pos) * PI / 180.f;
            px = cx + std::sin(th) * r; py = cy - std::cos(th) * r;
        };
        static const float marks[8] = { 20, 10, 7, 5, 3, 2, 1, 0 };
        static const char* const mlab[8] = { "20", "10", "7", "5", "3", "2", "1", "0" };
        for (int i = 0; i < 8; ++i) {
            float ax, ay, bx2, by2, lx, ly;
            const float p = grPos(marks[i]);
            const bool red = marks[i] >= 7;
            pt(p, R, ax, ay); pt(p, R + 14, bx2, by2); pt(p, R + 30, lx, ly);
            strokeColor(red ? Color(190, 30, 30) : Color(40, 36, 30)); strokeWidth(red ? 4.f : 2.4f); lineCap(BUTT);
            beginPath(); moveTo(ax, ay); lineTo(bx2, by2); stroke();
            monoLabel(lx, ly, mlab[i], 16, red ? Color(190, 30, 30) : Color(40, 36, 30));
        }
        for (float d = 0.5f; d < 10.f; d += (d < 5 ? 0.5f : 1.f)) {
            float ax, ay, bx2, by2;
            pt(grPos(d), R, ax, ay); pt(grPos(d), R + 7, bx2, by2);
            strokeColor(Color(40, 36, 30)); strokeWidth(1.2f); beginPath(); moveTo(ax, ay); lineTo(bx2, by2); stroke();
        }
        {
            const float a0 = -46.f * PI / 180.f - PI / 2, a1 = 46.f * PI / 180.f - PI / 2;
            beginPath(); arc(cx, cy, R, a0, a1, CW); strokeColor(Color(40, 36, 30)); strokeWidth(1.4f); stroke();
        }
        monoLabel(fx + fw - 14, fy + fh - 48, "GR", 20, Color(40, 36, 30), ALIGN_RIGHT | ALIGN_MIDDLE);
        monoLabel(fx + 12, fy + 14, "dB", 13, Color(40, 36, 30), ALIGN_LEFT | ALIGN_MIDDLE);

        const float total = -(grClipDisp + grLimDisp);
        const bool hot = total > 6.f;
        beginPath(); circle(fx + fw - 16, fy + 14, 5);
        fillPaint(radialGradient(fx + fw - 18, fy + 12, 1, 6, hot ? Color(255, 200, 190) : Color(160, 90, 80), hot ? Color(230, 20, 20) : Color(90, 30, 25))); fill();

        lcd(fx + 8, fy + fh - 30, 118, 24, "LUFS", vals[kOutLufsI] > -70.f ? vals[kOutLufsI] : -100.f, false);
        lcd(fx + fw - 126, fy + fh - 30, 118, 24, "dBTP", vals[kOutTP], vals[kOutTP] > vals[kCeiling] + 0.05f);

        const float gr = std::fmin(20.f, total);
        float nx, ny; pt(grPos(gr), R + 26, nx, ny);
        float sx, sy; pt(grPos(gr), 10, sx, sy);
        strokeColor(Color(0, 0, 0, 0.18f)); strokeWidth(3.f); lineCap(ROUND);
        beginPath(); moveTo(sx + 3, sy + 3); lineTo(nx + 3, ny + 3); stroke();
        strokeColor(Color(18, 18, 20)); strokeWidth(2.2f);
        beginPath(); moveTo(sx, sy); lineTo(nx, ny); stroke();
        restore();

        beginPath(); moveTo(fx, fy); lineTo(fx + fw * 0.55f, fy); lineTo(fx + fw * 0.35f, fy + fh); lineTo(fx, fy + fh); closePath();
        fillColor(Color(255, 255, 255, 0.10f)); fill();
        rrect(fx, fy, fw, fh, 4); strokePaint(linearGradient(0, fy, 0, fy + 10, Color(0, 0, 0, 0.5f), Color(0, 0, 0, 0.f))); strokeWidth(4); stroke();
    }

    void lcd(float x, float y, float w, float h, const char* unit, float v, bool red)
    {
        rrect(x, y, w, h, 3); fillColor(Color(12, 14, 18)); fill();
        char buf[32];
        if (v > -99.f) std::snprintf(buf, sizeof(buf), "%.1f", v); else std::snprintf(buf, sizeof(buf), "--.-");
        fontMono(); fontSize(17); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        const Color c = red ? Color(255, 70, 70) : Color(90, 160, 255);
        fontBlur(6.f); fillColor(Color(c.red, c.green, c.blue, 0.6f)); text(x + w - 46, y + h / 2 + 1, buf, nullptr);
        fontBlur(0.f); fillColor(c); text(x + w - 46, y + h / 2 + 1, buf, nullptr);
        monoLabel(x + w - 6, y + h / 2 + 1, unit, 11, Color(120, 150, 200), ALIGN_RIGHT | ALIGN_MIDDLE);
    }

    // preset strip under the logo
    Rect pbPrev() const { return { 1035, 314, 28, 28 }; }
    Rect pbName() const { return { 1066, 314, 160, 28 }; }
    Rect pbNext() const { return { 1229, 314, 28, 28 }; }
    Rect pbSave() const { return { 1263, 314, 50, 28 }; }
    Rect pbAB()   const { return { 1316, 314, 34, 28 }; }
    Rect pbMore() const { return { 1353, 314, 46, 28 }; }

    void miniButton(const Rect& r, const char* t, bool on)
    {
        rrect(r.x, r.y + 2, r.w, r.h, 5); fillColor(Color(0, 0, 0, 0.35f)); fill();
        rrect(r.x, r.y, r.w, r.h, 5); fillPaint(linearGradient(0, r.y, 0, r.y + r.h, Color(46, 48, 52), Color(16, 17, 19))); fill();
        if (on) { rrect(r.x + 0.5f, r.y + 0.5f, r.w - 1, r.h - 1, 5); strokeColor(Color(60, 130, 255)); strokeWidth(1.4f); stroke(); }
        monoLabel(r.x + r.w / 2, r.y + r.h / 2 + 1, t, 13, on ? Color(110, 170, 255) : Color(228, 230, 234));
    }

    void drawPresetBar()
    {
        miniButton(pbPrev(), "<", false);
        const Rect n = pbName();
        rrect(n.x, n.y, n.w, n.h, 4); fillColor(Color(12, 14, 18)); fill();
        std::string nm = currentPreset.empty() ? std::string("(unsaved)") : currentPreset;
        if (nm.size() > 16) nm = nm.substr(0, 15) + "..";
        monoLabel(n.x + n.w / 2, n.y + n.h / 2 + 1, nm.c_str(), 14, Color(110, 170, 255));
        miniButton(pbNext(), ">", false);
        miniButton(pbSave(), "SAVE", false);
        miniButton(pbAB(), abSlot ? "B" : "A", abInit);
        miniButton(pbMore(), "MORE", false);
    }

    // ------------------------------------------------------------ overlays in the centre window
    Rect ovClose() const { return { WIN_X1 - 46, 16, 34, 28 }; }
    Rect tabRect(int t) const { return { WIN_X0 + 16 + t * 196.f, 16, 190, 30 }; }
    Rect presetRect(int i) const { return { WIN_X0 + 16 + (i % 2) * 224.f, 58 + (i / 2) * 50.f, 218, 42 }; }
    Rect deleteRect(int k) const { const Rect r = presetRect(k); return { r.x + r.w - 36, r.y + 6, 30, r.h - 12 }; }
    Rect pageRect(int i) const { return { i == 0 ? WIN_X0 + 160 : WIN_X0 + 281, 292, 36, 30 }; }
    Rect nameRect() const { return { WIN_X0 + 24, 96, WIN_X1 - WIN_X0 - 48, 50 }; }
    Rect saveBtnRect() const { return { WIN_X0 + 246, 236, 206, 50 }; }
    Rect cancelBtnRect() const { return { WIN_X0 + 24, 236, 206, 50 }; }
    Rect itemRect(int i) const { return { WIN_X0 + 14 + (i % 4) * 113.f, 52 + (i / 4) * 72.f, 108, 66 }; }

    void drawOverlay()
    {
        const Color white(232, 236, 244), cyan(100, 155, 255), dim(150, 160, 180);
        miniButton(ovClose(), "X", false);
        if (overlay == 2) {
            for (int t = 0; t < 2; ++t) {
                const Rect r = tabRect(t);
                const bool act = presetTab == t;
                rrect(r.x, r.y, r.w, r.h, 6); fillColor(act ? Color(20, 90, 130) : Color(26, 30, 36)); fill();
                rrect(r.x, r.y, r.w, r.h, 6); strokeColor(act ? cyan : Color(70, 80, 90)); strokeWidth(1.2f); stroke();
                label(r.x + r.w / 2, r.y + r.h / 2 + 1, t == 0 ? "FACTORY" : "MY PRESETS", 16, act ? white : dim, ALIGN_CENTER | ALIGN_MIDDLE, 1.5f);
            }
            if (presetTab == 0) {
                for (int i = 0; i < kNumPresets; ++i) {
                    const Rect r = presetRect(i);
                    const bool cur = currentPreset == kPresets[i].name;
                    rrect(r.x, r.y, r.w, r.h, 6); fillColor(cur ? Color(20, 90, 130) : Color(30, 34, 40)); fill();
                    rrect(r.x, r.y, r.w, r.h, 6); strokeColor(Color(70, 110, 130)); strokeWidth(1); stroke();
                    label(r.x + r.w / 2, r.y + r.h / 2 + 1, kPresets[i].name, 16, white);
                }
            } else {
                if (myPresets.empty()) {
                    label(WIN_X0 + 238, 150, "No saved presets yet.", 18, white);
                    label(WIN_X0 + 238, 182, "Set your sound, then press SAVE.", 15, dim);
                }
                const int pages = std::max(1, ((int)myPresets.size() + 7) / 8);
                if (myPage >= pages) myPage = pages - 1;
                for (int k = 0; k < 8; ++k) {
                    const int i = myPage * 8 + k;
                    if (i >= (int)myPresets.size()) break;
                    const Rect r = presetRect(k);
                    const bool cur = currentPreset == myPresets[i];
                    rrect(r.x, r.y, r.w, r.h, 6); fillColor(cur ? Color(20, 90, 130) : Color(30, 34, 40)); fill();
                    rrect(r.x, r.y, r.w, r.h, 6); strokeColor(Color(70, 110, 130)); strokeWidth(1); stroke();
                    std::string nm = myPresets[i]; if (nm.size() > 18) nm = nm.substr(0, 17) + "..";
                    label(r.x + 12, r.y + r.h / 2 + 1, nm.c_str(), 15, white, ALIGN_LEFT | ALIGN_MIDDLE);
                    const Rect d = deleteRect(k);
                    const bool armed = deleteArm == i;
                    rrect(d.x, d.y, d.w, d.h, 4); fillColor(armed ? Color(200, 30, 40) : Color(45, 50, 58)); fill();
                    label(d.x + d.w / 2, d.y + d.h / 2 + 1, armed ? "?" : "X", 14, Color(235, 238, 242));
                }
                if (pages > 1) {
                    char pg[32]; std::snprintf(pg, sizeof(pg), "%d / %d", myPage + 1, pages);
                    label(WIN_X0 + 238, 308, pg, 15, white);
                    miniButton(pageRect(0), "<", false); miniButton(pageRect(1), ">", false);
                }
            }
        } else if (overlay == 3) {
            label(WIN_X0 + 238, 66, "SAVE PRESET", 20, cyan, ALIGN_CENTER | ALIGN_MIDDLE, 3);
            const Rect f = nameRect();
            rrect(f.x, f.y, f.w, f.h, 6); fillColor(Color(8, 16, 24)); fill();
            rrect(f.x, f.y, f.w, f.h, 6); strokeColor(cyan); strokeWidth(1.5f); stroke();
            std::string shown = saveName;
            if (std::fmod(nowSec(), 1.0) < 0.55) shown += "|";
            label(f.x + 14, f.y + f.h / 2 + 1, shown.c_str(), 21, white, ALIGN_LEFT | ALIGN_MIDDLE);
            label(WIN_X0 + 238, 180, "Type a name, then SAVE or Enter.", 15, dim);
            label(WIN_X0 + 238, 204, "Same name = overwrite.", 15, dim);
            const Rect sv = saveBtnRect(), cn = cancelBtnRect();
            rrect(sv.x, sv.y, sv.w, sv.h, 8); fillPaint(linearGradient(0, sv.y, 0, sv.y + sv.h, Color(120, 215, 255), Color(20, 140, 210))); fill();
            label(sv.x + sv.w / 2, sv.y + sv.h / 2 + 1, "SAVE", 20, Color(255, 255, 255), ALIGN_CENTER | ALIGN_MIDDLE, 2);
            miniButton(cn, "CANCEL", false);
        } else if (overlay == 4) {
            for (int i = 0; i < 16; ++i) {
                const Item& it = kItems[i];
                const Rect r = itemRect(i);
                bool on = false; char buf[32] = "";
                if (it.kind == IK_Action) std::snprintf(buf, sizeof(buf), "CLICK");
                else { formatValue(it.id, buf, sizeof(buf)); on = it.kind == IK_Cont || vals[it.id] > 0.5f; }
                rrect(r.x, r.y, r.w, r.h, 6); fillColor(Color(8, 16, 24)); fill();
                rrect(r.x, r.y, r.w, r.h, 6); strokeColor(on && it.kind != IK_Cont ? cyan : Color(55, 75, 90)); strokeWidth(1.2f); stroke();
                label(r.x + r.w / 2, r.y + 20, it.name, 12.5f, dim);
                label(r.x + r.w / 2, r.y + 46, buf, 19, on ? Color(110, 215, 255) : Color(150, 160, 170));
            }
            label(WIN_X0 + 200, 30, "SETTINGS", 17, cyan, ALIGN_CENTER | ALIGN_MIDDLE, 4);
        }
    }

    void drawTooltip()
    {
        if (!dragging || dragKnob < 0) return;
        const KnobDef& k = kKnobs[dragKnob];
        char buf[32]; formatValue(k.id, buf, sizeof(buf));
        const float ty = k.r > 40 ? k.cy : k.cy;
        rrect(k.cx - 62, ty - 18, 124, 36, 7); fillColor(Color(10, 14, 18, 0.92f)); fill();
        rrect(k.cx - 62, ty - 18, 124, 36, 7); strokeColor(Color(90, 200, 255)); strokeWidth(1.4f); stroke();
        label(k.cx, ty + 1, buf, 19, Color(120, 220, 255));
    }

    // ------------------------------------------------------------ interaction
    int knobAt(float x, float y) const
    {
        for (int i = 0; i < kNumKnobs; ++i) {
            const KnobDef& k = kKnobs[i];
            const float dx = x - k.cx, dy = y - k.cy, rr = k.r + 14;
            if (dx * dx + dy * dy <= rr * rr) return i;
        }
        return -1;
    }
    static bool hitCircle(float x, float y, float cx, float cy, float r) { return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r; }

    bool onMouse(const MouseEvent& ev) override
    {
        const float sc = scaleF();
        const float x = ev.pos.getX() / sc, y = ev.pos.getY() / sc;
        if (!ev.press) {
            if (dragging) { editParameter(dragParam, false); dragging = false; dragKnob = -1; }
            repaint();
            return true;
        }
        if (ev.press) { lastPressX = x; lastPressY = y; lastPressT = nowSec(); }
        const double t = nowSec();
        const bool dbl = (t - lastClick < 0.35) && std::fabs(x - lastClickX) < 6 && std::fabs(y - lastClickY) < 6;
        lastClick = t; lastClickX = x; lastClickY = y;

        if (ev.button == 3) {   // right click: default value
            const int k = knobAt(x, y);
            if (k >= 0) { setParamGesture(kKnobs[k].id, kParamInfo[kKnobs[k].id].def); repaint(); return true; }
            return false;
        }
        if (ev.button != 1) return false;

        // centre window
        if (x >= WIN_X0 && x <= WIN_X1) {
            if (overlay != 0) { overlayClick(x, y, dbl); repaint(); return true; }
            if (pbPrev().hit(x, y)) { stepPreset(-1); }
            else if (pbNext().hit(x, y)) { stepPreset(+1); }
            else if (pbName().hit(x, y)) { refreshMyPresets(); deleteArm = -1; overlay = 2; }
            else if (pbSave().hit(x, y)) { openSave(); }
            else if (pbAB().hit(x, y)) { swapAB(); }
            else if (pbMore().hit(x, y)) { overlay = 4; }
            else if (y > 64 && y < 224) { setParamGesture(kResetMeters, 1.f); vals[kResetMeters] = 0; toast("Meters reset"); }
            repaint();
            return true;
        }

        const int k = knobAt(x, y);
        if (k >= 0) {
            const int id = kKnobs[k].id;
            if (dbl) { setParamGesture(id, kParamInfo[id].def); repaint(); return true; }
            dragging = true; dragKnob = k; dragParam = id; dragStartY = y; dragStartX = x; dragStartNorm = toNorm(id, vals[id]);
            editParameter(id, true);
            repaint();
            return true;
        }
        for (int i = 0; i < 4; ++i) {
            if (Rect{ CLIP_X - BTN_W / 2, BTN_Y[i] - BTN_H / 2, BTN_W, BTN_H }.hit(x, y)) {
                if (i == 3) setParamGesture(kClip, 0.f);
                else { setParamGesture(kClip, 1.f); setParamGesture(kClipShape, (float)(2 - i)); }
                repaint(); return true;
            }
            if (Rect{ MODE_X - BTN_W / 2, BTN_Y[i] - BTN_H / 2, BTN_W, BTN_H }.hit(x, y)) {
                setParamGesture(kMode, (float)i); repaint(); return true;
            }
        }
        if (hitCircle(x, y, 165, 74, 22) || Rect{ 140, 150, 50, 82 }.hit(x, y)) {
            setParamGesture(kBypass, vals[kBypass] > 0.5f ? 0.f : 1.f); repaint(); return true;
        }
        if (Rect{ TOG_X - 26, 80, 52, 78 }.hit(x, y)) { setParamGesture(kTruePeak, vals[kTruePeak] > 0.5f ? 0.f : 1.f); repaint(); return true; }
        if (Rect{ TOG_X - 40, 238, 80, 44 }.hit(x, y)) { setParamGesture(kMonoSub, vals[kMonoSub] > 0.5f ? 0.f : 1.f); repaint(); return true; }
        return false;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (!dragging) return false;
        const float x = ev.pos.getX() / scaleF();
        // vertical drag (up = more) plus horizontal drag (right = more)
        const float y = ev.pos.getY() / scaleF() - (x - dragStartX);
        const bool fine = (ev.mod & kModifierShift) != 0;
        setParam(dragParam, fromNorm(dragParam, dragStartNorm + (dragStartY - y) / (fine ? 1400.f : 260.f)));
        repaint();
        return true;
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        const float sc = scaleF();
        const float x = ev.pos.getX() / sc, y = ev.pos.getY() / sc;
        int id = -1;
        const int k = knobAt(x, y);
        if (k >= 0) id = kKnobs[k].id;
        else if (overlay == 4) {
            for (int i = 0; i < 16; ++i) if (itemRect(i).hit(x, y) && kItems[i].kind == IK_Cont) id = kItems[i].id;
        }
        if (id < 0) return false;
        const bool fine = (ev.mod & kModifierShift) != 0;
        setParamGesture(id, fromNorm(id, toNorm(id, vals[id]) + (float)ev.delta.getY() * (fine ? 0.004f : 0.025f)));
        repaint();
        return true;
    }

    void stepPreset(int dir)
    {
        refreshMyPresets();
        const int total = kNumPresets + (int)myPresets.size();
        int cur = -1;
        for (int i = 0; i < kNumPresets; ++i) if (currentPreset == kPresets[i].name) cur = i;
        for (int i = 0; i < (int)myPresets.size(); ++i) if (currentPreset == myPresets[i]) cur = kNumPresets + i;
        const int nx = cur < 0 ? (dir > 0 ? 0 : total - 1) : (cur + dir + total) % total;
        if (nx < kNumPresets) applyPreset(nx); else loadMyPreset(myPresets[nx - kNumPresets]);
    }

    void overlayClick(float x, float y, bool dbl)
    {
        if (ovClose().hit(x, y)) { overlay = 0; return; }
        if (overlay == 2) {
            for (int t = 0; t < 2; ++t) if (tabRect(t).hit(x, y)) { presetTab = t; deleteArm = -1; if (t == 1) refreshMyPresets(); return; }
            if (presetTab == 0) {
                for (int i = 0; i < kNumPresets; ++i) if (presetRect(i).hit(x, y)) { applyPreset(i); overlay = 0; return; }
                return;
            }
            const int pages = std::max(1, ((int)myPresets.size() + 7) / 8);
            if (pages > 1) {
                if (pageRect(0).hit(x, y)) { myPage = (myPage + pages - 1) % pages; deleteArm = -1; return; }
                if (pageRect(1).hit(x, y)) { myPage = (myPage + 1) % pages; deleteArm = -1; return; }
            }
            for (int k = 0; k < 8; ++k) {
                const int i = myPage * 8 + k;
                if (i >= (int)myPresets.size()) break;
                if (deleteRect(k).hit(x, y)) {
                    if (deleteArm == i) { deleteArm = -1; deleteMyPreset(myPresets[i]); } else deleteArm = i;
                    return;
                }
                if (presetRect(k).hit(x, y)) { deleteArm = -1; loadMyPreset(myPresets[i]); overlay = 0; return; }
            }
            deleteArm = -1;
            return;
        }
        if (overlay == 3) {
            if (saveBtnRect().hit(x, y)) { doSave(); return; }
            if (cancelBtnRect().hit(x, y)) { overlay = 0; return; }
            return;
        }
        if (overlay == 4) {
            for (int i = 0; i < 16; ++i) {
                if (!itemRect(i).hit(x, y)) continue;
                const Item& it = kItems[i];
                if (it.kind == IK_Action) { setParamGesture(kResetMeters, 1.f); vals[kResetMeters] = 0; toast("Meters reset"); }
                else if (it.kind == IK_Bool) setParamGesture(it.id, vals[it.id] > 0.5f ? 0.f : 1.f);
                else if (it.kind == IK_Cycle) setParamGesture(it.id, (float)(((int)(vals[it.id] + 0.5f) + 1) % it.n));
                else {
                    if (dbl) { setParamGesture(it.id, kParamInfo[it.id].def); return; }
                    dragging = true; dragKnob = -1; dragParam = it.id; dragStartY = y; dragStartX = x; dragStartNorm = toNorm(it.id, vals[it.id]);
                    editParameter(it.id, true);
                }
                return;
            }
        }
    }

    void applyPreset(int i)
    {
        for (int p = 0; p < kNumInputParams; ++p)
            if (p != kBypass && p != kResetMeters) setParamGesture(p, kParamInfo[p].def);
        const Preset& pr = kPresets[i];
        for (int k = 0; k < pr.n; ++k) setParamGesture((int)pr.vals[k].id, pr.vals[k].v);
        currentPreset = pr.name;
        char msg[64]; std::snprintf(msg, sizeof(msg), "Preset: %s", pr.name); toast(msg);
    }

    void swapAB()
    {
        if (!abInit) { for (int i = 0; i < kNumInputParams; ++i) abStore[0][i] = abStore[1][i] = vals[i]; abInit = true; }
        for (int i = 0; i < kNumInputParams; ++i) abStore[abSlot][i] = vals[i];
        abSlot ^= 1;
        for (int i = 0; i < kNumInputParams; ++i)
            if (i != kBypass && i != kResetMeters && abStore[abSlot][i] != vals[i]) setParamGesture(i, abStore[abSlot][i]);
        toast(abSlot ? "Setting B" : "Setting A");
    }

    // ------------------------------------------------------------ user presets
    static std::string presetDir()
    {
#ifdef _WIN32
        const char* base = std::getenv("APPDATA");
        std::string d = std::string(base ? base : ".") + "\\TrapVST";
        CreateDirectoryA(d.c_str(), nullptr);
        d += "\\Limiter"; CreateDirectoryA(d.c_str(), nullptr);
        d += "\\Presets"; CreateDirectoryA(d.c_str(), nullptr);
        return d + "\\";
#else
        const char* base = std::getenv("HOME");
        std::string d = std::string(base ? base : ".") + "/.trapvst";
        mkdir(d.c_str(), 0755); d += "/limiter"; mkdir(d.c_str(), 0755); d += "/presets"; mkdir(d.c_str(), 0755);
        return d + "/";
#endif
    }
    static const char* presetExt() { return ".tlpreset"; }

    void refreshMyPresets()
    {
        myPresets.clear();
        const std::string dir = presetDir();
        const std::string ext = presetExt();
#ifdef _WIN32
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA((dir + "*" + ext).c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                std::string n = fd.cFileName;
                if (n.size() > ext.size()) myPresets.push_back(n.substr(0, n.size() - ext.size()));
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
#else
        if (DIR* d = opendir(dir.c_str())) {
            while (dirent* e = readdir(d)) {
                std::string n = e->d_name;
                if (n.size() > ext.size() && n.compare(n.size() - ext.size(), ext.size(), ext) == 0)
                    myPresets.push_back(n.substr(0, n.size() - ext.size()));
            }
            closedir(d);
        }
#endif
        // migrate the single preset from v1.0/1.1
        if (myPresets.empty()) {
#ifdef _WIN32
            const char* base = std::getenv("APPDATA");
            std::string old = std::string(base ? base : ".") + "\\TrapLimit\\user_preset.txt";
#else
            const char* base = std::getenv("HOME");
            std::string old = std::string(base ? base : ".") + "/.traplimit/user_preset.txt";
#endif
            if (FILE* f = std::fopen(old.c_str(), "r")) {
                std::string body; char buf[256];
                while (std::fgets(buf, sizeof(buf), f)) body += buf;
                std::fclose(f);
                if (FILE* o = std::fopen((dir + "My Preset 1" + ext).c_str(), "w")) { std::fputs(body.c_str(), o); std::fclose(o); myPresets.push_back("My Preset 1"); }
                std::remove(old.c_str());
            }
        }
        std::sort(myPresets.begin(), myPresets.end(), [](const std::string& a, const std::string& b) {
            std::string x = a, y = b;
            for (auto& c : x) c = (char)std::tolower((unsigned char)c);
            for (auto& c : y) c = (char)std::tolower((unsigned char)c);
            return x < y;
        });
    }

    void openSave()
    {
        refreshMyPresets();
        // default name: current user preset name, or the next free "My Preset N"
        bool isMine = std::find(myPresets.begin(), myPresets.end(), currentPreset) != myPresets.end();
        if (isMine) saveName = currentPreset;
        else {
            for (int n = 1; n < 1000; ++n) {
                char b[32]; std::snprintf(b, sizeof(b), "My Preset %d", n);
                if (std::find(myPresets.begin(), myPresets.end(), std::string(b)) == myPresets.end()) { saveName = b; break; }
            }
        }
        overlay = 3;
        nameFresh = true;
    }

    static std::string cleanName(const std::string& in)
    {
        std::string o;
        for (char c : in) {
            if (std::isalnum((unsigned char)c) || c == ' ' || c == '-' || c == '_' || c == '.' || c == '(' || c == ')' || c == '#') o += c;
        }
        while (!o.empty() && (o.back() == ' ' || o.back() == '.')) o.pop_back();
        while (!o.empty() && o.front() == ' ') o.erase(o.begin());
        return o;
    }

    void doSave()
    {
        const std::string name = cleanName(saveName);
        if (name.empty()) { toast("Type a preset name first"); return; }
        const bool exists = std::find(myPresets.begin(), myPresets.end(), name) != myPresets.end();
        const std::string path = presetDir() + name + presetExt();
        FILE* f = std::fopen(path.c_str(), "w");
        if (!f) { toast("Save failed (folder not writable)"); return; }
        std::fprintf(f, "# TrapVST Limiter preset\n");
        for (int i = 0; i < kNumInputParams; ++i)
            if (i != kBypass && i != kResetMeters) std::fprintf(f, "%s %g\n", kParamInfo[i].symbol, vals[i]);
        std::fclose(f);
        currentPreset = name;
        refreshMyPresets();
        presetTab = 1;
        overlay = 0;
        char msg[96]; std::snprintf(msg, sizeof(msg), exists ? "Updated: %s" : "Saved: %s", name.c_str()); toast(msg);
    }

    void loadMyPreset(const std::string& name)
    {
        FILE* f = std::fopen((presetDir() + name + presetExt()).c_str(), "r");
        if (!f) { toast("Preset not found"); refreshMyPresets(); return; }
        // start from defaults so missing values don't leak from the previous sound
        for (int i = 0; i < kNumInputParams; ++i)
            if (i != kBypass && i != kResetMeters) setParamGesture(i, kParamInfo[i].def);
        char line[256];
        while (std::fgets(line, sizeof(line), f)) {
            char sym[64]; float v;
            if (line[0] == '#' || std::sscanf(line, "%63s %f", sym, &v) != 2) continue;
            for (int i = 0; i < kNumInputParams; ++i)
                if (!std::strcmp(sym, kParamInfo[i].symbol) && i != kBypass && i != kResetMeters) { setParamGesture(i, v); break; }
        }
        std::fclose(f);
        currentPreset = name;
        char msg[96]; std::snprintf(msg, sizeof(msg), "Loaded: %s", name.c_str()); toast(msg);
    }

    void deleteMyPreset(const std::string& name)
    {
        std::remove((presetDir() + name + presetExt()).c_str());
        if (currentPreset == name) currentPreset = "";
        refreshMyPresets();
        char msg[96]; std::snprintf(msg, sizeof(msg), "Deleted: %s", name.c_str()); toast(msg);
    }

    // ------------------------------------------------------------ keyboard (preset name entry)
    bool onKeyboard(const KeyboardEvent& ev) override
    {
        if (overlay != 3 || !ev.press) return false;
        if (ev.key == kKeyBackspace) { nameFresh = false; if (!saveName.empty()) saveName.pop_back(); repaint(); return true; }
        if (ev.key == kKeyEnter) { doSave(); repaint(); return true; }
        if (ev.key == kKeyEscape) { overlay = 0; repaint(); return true; }
        if (ev.key == kKeyDelete) { saveName.clear(); repaint(); return true; }
        return true;   // swallow other keys while typing
    }

    bool onCharacterInput(const CharacterInputEvent& ev) override
    {
        if (overlay != 3) return false;
        const unsigned c = ev.character;
        if (nameFresh && c >= 32 && c < 127) { saveName.clear(); nameFresh = false; }
        if (c >= 32 && c < 127 && saveName.size() < 28) {
            std::string t = saveName + (char)c;
            if (!cleanName(t + "x").empty() && (std::isalnum(c) || std::strchr(" -_.()#", (int)c))) saveName = t;
        }
        repaint();
        return true;
    }


private:
    float vals[kParamCount];
    int fontMonoId = -1, fontLogoId = -1;
#ifndef TL_MOCK
    NanoImage concrete;
#endif
    int overlay = 0;          // 0 none, 2 presets, 3 save, 4 settings
    bool dragging = false;
    int dragKnob = -1, dragParam = 0; float dragStartY = 0, dragStartX = 0, dragStartNorm = 0;
    bool resInit = false;
    float lastPressX = 0, lastPressY = 0; double lastPressT = -10;
    double lastClick = 0, toastTime = -10; float lastClickX = 0, lastClickY = 0;
    char toastMsg[96] = {};
    int abSlot = 0; bool abInit = false; float abStore[2][kNumInputParams] = {};
    std::string currentPreset = "Default";
    std::vector<std::string> myPresets;
    int presetTab = 0, myPage = 0, deleteArm = -1;
    std::string saveName;
    bool nameFresh = false;
    float grClipDisp = 0.f, grLimDisp = 0.f;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrapWinBetonUI)
};

UI* createUI() { return new TrapWinBetonUI(); }

END_NAMESPACE_DISTRHO
