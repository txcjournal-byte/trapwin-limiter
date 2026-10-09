// TRAPWIN TW-2 True Peak Limiter - rack-style UI (NanoVG)
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
# include "res/LogoPng.h"
# include "res/PoppinsBold.h"
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
    { kSat,    1712.f,  92.f, 24.f, "SAT"     },
    { k808,    1712.f, 238.f, 24.f, "808 BOOST" },
};
const int kNumKnobs = 6;
const float BTN_W = 56.f, BTN_H = 44.f;
const float BTN_Y[4] = { 100.f, 152.f, 204.f, 256.f };
const float CLIP_X = 905.f, MODE_X = 1585.f;
const char* const kClipLabels[4] = { "TAPE", "HARD", "SOFT", "OFF" };
const char* const kModeLabels[4] = { "TRANS", "PUNCH", "LOUD", "DIRTY" };
const float TOG_X = 1822.f;

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

class TrapWinUI : public UI
{
public:
    TrapWinUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        loadSharedResources();
#ifndef TL_MOCK
        fontBold = createFontFromMemory("poppins-bold", res_poppins_bold, res_poppins_bold_size, false);
        logo = createImageFromMemory(res_logo_png, res_logo_png_size, IMAGE_GENERATE_MIPMAPS);
#endif
        for (int i = 0; i < kNumInputParams; ++i) vals[i] = kParamInfo[i].def;
        for (int i = kNumInputParams; i < kParamCount; ++i) vals[i] = -100.f;
        vals[kOutGR] = 0.f; vals[kOutClipGR] = 0.f; vals[kOutLRA] = 0.f; vals[kOutCPU] = 0.f; vals[kOutLatency] = 0.f;
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH * 7 / 10, DISTRHO_UI_DEFAULT_HEIGHT * 7 / 10, true);
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
    void font() {
#ifndef TL_MOCK
        if (fontBold >= 0) fontFaceId(fontBold);
#else
        fontFace("poppins-bold");
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

    void screw(float cx, float cy)
    {
        beginPath(); circle(cx, cy, 19); fillColor(Color(40, 42, 46)); fill();
        beginPath(); circle(cx, cy, 15);
        fillPaint(radialGradient(cx - 5, cy - 5, 2, 18, Color(250, 251, 252), Color(110, 114, 120))); fill();
        strokeColor(Color(20, 22, 25)); strokeWidth(3.2f); lineCap(ROUND);
        beginPath(); moveTo(cx - 8, cy); lineTo(cx + 8, cy); moveTo(cx, cy - 8); lineTo(cx, cy + 8); stroke();
    }

    void lamp(float cx, float cy, bool on)
    {
        beginPath(); circle(cx, cy, 19); fillPaint(linearGradient(0, cy - 19, 0, cy + 19, Color(120, 124, 130), Color(240, 242, 245))); fill();
        if (on) {
            beginPath(); circle(cx, cy, 34); fillPaint(radialGradient(cx, cy, 8, 34, Color(90, 200, 255, 0.55f), Color(90, 200, 255, 0.f))); fill();
            beginPath(); circle(cx, cy, 14); fillPaint(radialGradient(cx - 4, cy - 4, 1, 15, Color(235, 250, 255), Color(30, 140, 230))); fill();
        } else {
            beginPath(); circle(cx, cy, 14); fillPaint(radialGradient(cx - 4, cy - 4, 1, 15, Color(110, 130, 150), Color(20, 35, 55))); fill();
        }
        beginPath(); ellipse(cx - 4, cy - 6, 5, 3); fillColor(Color(255, 255, 255, 0.7f)); fill();
    }

    void toggleSwitch(float cx, float cy, bool up)
    {
        // hex nut
        beginPath();
        for (int i = 0; i < 6; ++i) {
            const float a = PI / 6 + i * PI / 3;
            const float px = cx + std::cos(a) * 21, py = cy + std::sin(a) * 21;
            if (i == 0) moveTo(px, py); else lineTo(px, py);
        }
        closePath();
        fillPaint(linearGradient(cx - 20, cy - 20, cx + 20, cy + 20, Color(235, 237, 240), Color(70, 73, 78))); fill();
        strokeColor(Color(40, 42, 46)); strokeWidth(1.2f); stroke();
        beginPath(); circle(cx, cy, 11); fillColor(Color(25, 26, 29)); fill();
        // lever
        const float dir = up ? -1.f : 1.f;
        const float tipY = cy + dir * 35;
        strokeColor(Color(60, 62, 66)); strokeWidth(9.f); lineCap(ROUND);
        beginPath(); moveTo(cx, cy); lineTo(cx, tipY); stroke();
        strokePaint(linearGradient(cx - 5, 0, cx + 5, 0, Color(250, 251, 252), Color(130, 134, 140))); strokeWidth(7.f);
        beginPath(); moveTo(cx, cy); lineTo(cx, tipY); stroke();
        beginPath(); circle(cx, tipY, 7); fillPaint(radialGradient(cx - 2, tipY - 2, 1, 8, Color(255, 255, 255), Color(140, 144, 150))); fill();
    }

    void pushButton(float cx, float cy, bool on)
    {
        const float x = cx - BTN_W / 2, y = cy - BTN_H / 2;
        rrect(x - 3, y - 3, BTN_W + 6, BTN_H + 6, 7); fillPaint(linearGradient(0, y - 3, 0, y + BTN_H + 3, Color(150, 154, 160), Color(250, 251, 252))); fill();
        rrect(x, y, BTN_W, BTN_H, 5);
        if (on) {
            fillPaint(linearGradient(0, y, 0, y + BTN_H, Color(170, 232, 255), Color(20, 150, 225))); fill();
            rrect(x - 8, y - 8, BTN_W + 16, BTN_H + 16, 12);
            fillPaint(boxGradient(x, y, BTN_W, BTN_H, 6, 14, Color(60, 190, 255, 0.45f), Color(60, 190, 255, 0.f))); fill();
            rrect(x + 14, cy - 3, BTN_W - 28, 6, 3); fillColor(Color(240, 252, 255)); fill();
        } else {
            fillPaint(linearGradient(0, y, 0, y + BTN_H, Color(58, 60, 66), Color(14, 15, 17))); fill();
            rrect(x + 3, y + 2, BTN_W - 6, BTN_H * 0.45f, 4); fillColor(Color(255, 255, 255, 0.07f)); fill();
        }
    }

    // knob value angle: 7 o'clock .. 5 o'clock (270 deg sweep)
    static float knobAngle(float n) { return (135.f + 270.f * n) * PI / 180.f; }

    void knobBody(float cx, float cy, float r, float n)
    {
        // shadow
        beginPath(); circle(cx + 3, cy + 6, r + 12); fillPaint(radialGradient(cx + 3, cy + 6, r, r + 14, Color(0, 0, 0, 0.35f), Color(0, 0, 0, 0.f))); fill();
        // knurled chrome skirt
        beginPath(); circle(cx, cy, r + 9);
        fillPaint(linearGradient(cx - r, cy - r, cx + r, cy + r, Color(245, 246, 248), Color(80, 83, 88))); fill();
        const int teeth = r > 40 ? 90 : 40;
        strokeWidth(r > 40 ? 1.4f : 1.f);
        for (int i = 0; i < teeth; ++i) {
            const float a = i * 2 * PI / teeth;
            beginPath(); moveTo(cx + std::cos(a) * (r + 2), cy + std::sin(a) * (r + 2)); lineTo(cx + std::cos(a) * (r + 8.5f), cy + std::sin(a) * (r + 8.5f));
            strokeColor(i % 2 ? Color(30, 32, 36, 0.55f) : Color(255, 255, 255, 0.35f)); stroke();
        }
        // cap
        beginPath(); circle(cx, cy, r + 1); fillColor(Color(10, 10, 12)); fill();
        beginPath(); circle(cx, cy, r);
        fillPaint(radialGradient(cx - r * 0.35f, cy - r * 0.45f, r * 0.1f, r * 1.1f, Color(78, 80, 86), Color(8, 8, 10))); fill();
        // specular arc
        beginPath(); arc(cx, cy, r * 0.82f, PI * 1.08f, PI * 1.55f, CW);
        strokeColor(Color(255, 255, 255, 0.22f)); strokeWidth(r * 0.1f); lineCap(ROUND); stroke();
        // pointer
        const float a = knobAngle(n);
        strokeColor(Color(245, 247, 250)); strokeWidth(r > 40 ? 6.f : 3.5f); lineCap(ROUND);
        beginPath(); moveTo(cx + std::cos(a) * r * 0.3f, cy + std::sin(a) * r * 0.3f); lineTo(cx + std::cos(a) * r * 0.86f, cy + std::sin(a) * r * 0.86f); stroke();
    }

    void scaleTicks(float cx, float cy, float rIn, float rOut, int n, bool big)
    {
        strokeColor(Color(28, 31, 36)); lineCap(BUTT);
        for (int i = 0; i < n; ++i) {
            const float a = knobAngle(i / (float)(n - 1));
            strokeWidth(big ? 3.f : 2.f);
            beginPath(); moveTo(cx + std::cos(a) * rIn, cy + std::sin(a) * rIn); lineTo(cx + std::cos(a) * rOut, cy + std::sin(a) * rOut); stroke();
            if (i < n - 1) {
                const float am = knobAngle((i + 0.5f) / (float)(n - 1));
                beginPath(); circle(cx + std::cos(am) * (rIn + (rOut - rIn) * 0.5f), cy + std::sin(am) * (rIn + (rOut - rIn) * 0.5f), big ? 1.6f : 1.2f);
                fillColor(Color(28, 31, 36)); fill();
            }
        }
    }

    void bigKnob(const KnobDef& k, const char* const* labels, int nl)
    {
        scaleTicks(k.cx, k.cy, k.r + 16, k.r + 27, nl, true);
        for (int i = 0; i < nl; ++i) {
            const float a = knobAngle(i / (float)(nl - 1));
            engraved(k.cx + std::cos(a) * (k.r + 54), k.cy + std::sin(a) * (k.r + 50), labels[i], 20);
        }
        knobBody(k.cx, k.cy, k.r, toNorm(k.id, vals[k.id]));
        engraved(k.cx, 296, k.name, 25, 5);
    }

    void smallKnob(const KnobDef& k, const char* lo, const char* mid, const char* hi)
    {
        scaleTicks(k.cx, k.cy, k.r + 13, k.r + 20, 7, false);
        engraved(k.cx + std::cos(knobAngle(0)) * (k.r + 30), k.cy + std::sin(knobAngle(0)) * (k.r + 30) + 6, lo, 16, 0, ALIGN_RIGHT | ALIGN_MIDDLE);
        engraved(k.cx + std::cos(knobAngle(1)) * (k.r + 30), k.cy + std::sin(knobAngle(1)) * (k.r + 30) + 6, hi, 16, 0, ALIGN_LEFT | ALIGN_MIDDLE);
        if (mid) engraved(k.cx, k.cy - k.r - 34, mid, 16);
        knobBody(k.cx, k.cy, k.r, toNorm(k.id, vals[k.id]));
        engraved(k.cx, k.cy + 70, k.name, 19, 4);
    }

    // ------------------------------------------------------------ main draw
    void onNanoDisplay() override
    {
        const float sc = scaleF();
        save();
        scale(sc, sc);

        drawChassis();
        drawLeftSection();
        drawRightSection();
        drawCentre();
        drawTooltip();

        restore();
    }

    void drawChassis()
    {
        // ears
        brushed(0, 0, 100, H, Color(205, 208, 212), Color(160, 164, 170), 11);
        brushed(EAR_R, 0, 100, H, Color(205, 208, 212), Color(160, 164, 170), 12);
        // main panel: upper light band + lower slightly darker band (cool tone for the limiter)
        brushed(100, 0, EAR_R - 100, 174, Color(226, 229, 233), Color(210, 214, 219), 3);
        brushed(100, 174, EAR_R - 100, H - 174, Color(196, 200, 206), Color(184, 188, 195), 5);
        beginPath(); rect(100, 173, EAR_R - 100, 1.5f); fillColor(Color(255, 255, 255, 0.6f)); fill();
        // seams between ears and panel
        for (float sx : { 100.f, EAR_R }) {
            beginPath(); rect(sx - 2, 0, 4, H); fillPaint(linearGradient(sx - 2, 0, sx + 2, 0, Color(90, 94, 100), Color(250, 251, 252))); fill();
        }
        // top highlight / bottom shadow
        beginPath(); rect(0, 0, W, 2); fillColor(Color(255, 255, 255, 0.8f)); fill();
        beginPath(); rect(0, H - 6, W, 6); fillPaint(linearGradient(0, H - 6, 0, H, Color(60, 63, 68), Color(20, 21, 24))); fill();
        screw(50, 64); screw(50, 282); screw(EAR_R + 50, 64); screw(EAR_R + 50, 282);
    }

    void drawLeftSection()
    {
        const bool on = vals[kBypass] < 0.5f;
        lamp(165, 80, on);
        engraved(165, 122, "ON", 17, 2);
        engraved(165, 190, "IN", 15, 1);
        toggleSwitch(165, 238, on);
        engraved(165, 300, "BYPASS", 15, 2);

        static const char* const inL[9] = { "0", "3", "6", "9", "12", "15", "18", "21", "24" };
        static const char* const ceL[7] = { "-12", "-10", "-8", "-6", "-4", "-2", "0" };
        bigKnob(kKnobs[0], inL, 9);
        bigKnob(kKnobs[1], ceL, 7);
        smallKnob(kKnobs[2], "0", "6", "12");
        smallKnob(kKnobs[3], "5", nullptr, "1k");

        engraved(CLIP_X - 22, 46, "CLIPPER", 19, 4);
        const bool clipOn = vals[kClip] > 0.5f;
        const int shape = (int)(vals[kClipShape] + 0.5f);
        for (int i = 0; i < 4; ++i) {
            const bool lit = (i == 3) ? !clipOn : (clipOn && shape == 2 - i);
            pushButton(CLIP_X, BTN_Y[i], lit);
            engraved(CLIP_X - 38, BTN_Y[i] + 1, kClipLabels[i], 16, 1, ALIGN_RIGHT | ALIGN_MIDDLE);
        }
    }

    void drawRightSection()
    {
        engraved(MODE_X - 22, 46, "MODE", 19, 4);
        const int mode = (int)(vals[kMode] + 0.5f);
        for (int i = 0; i < 4; ++i) {
            pushButton(MODE_X, BTN_Y[i], mode == i);
            engraved(MODE_X - 38, BTN_Y[i] + 1, kModeLabels[i], 16, 1, ALIGN_RIGHT | ALIGN_MIDDLE);
        }
        smallKnob(kKnobs[4], "0", "50", "MAX");
        smallKnob(kKnobs[5], "OFF", nullptr, "MAX");

        engraved(TOG_X, 40, "TRUE PEAK", 15, 2);
        engraved(TOG_X, 70, "ON", 14, 1);
        toggleSwitch(TOG_X, 118, vals[kTruePeak] > 0.5f);
        engraved(TOG_X, 166, "OFF", 14, 1);
        engraved(TOG_X, 200, "ON", 14, 1);
        toggleSwitch(TOG_X, 248, vals[kMonoSub] > 0.5f);
        engraved(TOG_X, 296, "OFF", 14, 1);
        engraved(TOG_X, 326, "MONO SUB", 15, 2);
    }

    // ------------------------------------------------------------ centre: meter, presets, logo
    void drawCentre()
    {
        const float x0 = WIN_X0, x1 = WIN_X1, w = x1 - x0;
        beginPath(); rect(x0 - 4, 0, w + 8, H); fillColor(Color(12, 13, 15)); fill();
        beginPath(); rect(x0, 0, w, H); fillPaint(linearGradient(0, 0, 0, H, Color(44, 47, 53), Color(18, 19, 22))); fill();
        // subtle brushed texture
        for (int i = 0; i < 70; ++i) { beginPath(); rect(x0, 4 + i * 5.f, w, 1); fillColor(Color(255, 255, 255, (i % 3) ? 0.012f : 0.025f)); fill(); }
        label(x0 + w / 2, 22, "TW-2   TRUE PEAK LIMITER", 17, Color(236, 240, 245), ALIGN_CENTER | ALIGN_MIDDLE, 3.5f);

        if (overlay == 0) {
            drawMeter();
            drawPresetBar();
            drawLogo(x0 + w / 2, 296, 100);
        } else {
            drawOverlay();
        }
        // toast
        if (nowSec() - toastTime < 1.6) {
            rrect(x0 + 40, 150, w - 80, 44, 8); fillColor(Color(0, 0, 0, 0.88f)); fill();
            rrect(x0 + 40, 150, w - 80, 44, 8); strokeColor(Color(90, 200, 255)); strokeWidth(1.5f); stroke();
            label(x0 + w / 2, 172, toastMsg, 18, Color(235, 245, 250));
        }
    }

    void drawLogo(float cx, float cy, float h)
    {
        const float aspect = 900.f / 328.f;
        const float w = h * aspect;
#ifndef TL_MOCK
        if (logo.isValid()) {
            beginPath(); rect(cx - w / 2, cy - h / 2, w, h);
            fillPaint(imagePattern(cx - w / 2, cy - h / 2, w, h, 0.f, logo, 1.f));
            fill();
        }
#else
        mockImage(cx - w / 2, cy - h / 2, w, h);
#endif
    }

    // GR meter scale position (0 dB right, 20 dB left)
    static float grPos(float d) { return 1.f - std::log10(1.f + d) / std::log10(21.f); }

    void drawMeter()
    {
        grClipDisp = std::fmin(vals[kOutClipGR], grClipDisp + 0.35f); if (grClipDisp > 0.f) grClipDisp = 0.f;
        grLimDisp  = std::fmin(vals[kOutGR],     grLimDisp  + 0.35f); if (grLimDisp > 0.f) grLimDisp = 0.f;
        const float bx = 1003, by = 40, bw = 417, bh = 166;
        // chrome bezel
        rrect(bx - 4, by - 4, bw + 8, bh + 8, 20); fillColor(Color(8, 8, 10)); fill();
        rrect(bx, by, bw, bh, 17); fillPaint(linearGradient(0, by, 0, by + bh, Color(250, 251, 252), Color(120, 124, 130))); fill();
        rrect(bx + 7, by + 7, bw - 14, bh - 14, 11); fillColor(Color(14, 15, 17)); fill();
        const float fx = bx + 14, fy = by + 14, fw = bw - 28, fh = bh - 28;
        rrect(fx, fy, fw, fh, 6);
        fillPaint(linearGradient(0, fy, 0, fy + fh, Color(240, 247, 250), Color(206, 222, 230))); fill();
        rrect(fx, fy, fw, fh, 6);
        fillPaint(radialGradient(fx + fw / 2, fy + fh * 0.75f, 20, fw * 0.6f, Color(255, 255, 255, 0.55f), Color(150, 210, 240, 0.f))); fill();

        save();
        scissor(fx, fy, fw, fh);
        const float cx = fx + fw / 2, cy = fy + fh + 38, R = 128;
        auto pt = [&](float pos, float r, float& px, float& py) {
            const float th = (-46.f + 92.f * pos) * PI / 180.f;
            px = cx + std::sin(th) * r; py = cy - std::cos(th) * r;
        };
        // red zone (heavy reduction 7..20 dB)
        {
            const float a0 = (-46.f + 92.f * grPos(20)) * PI / 180.f - PI / 2, a1 = (-46.f + 92.f * grPos(7)) * PI / 180.f - PI / 2;
            beginPath(); arc(cx, cy, R + 4, a0, a1, CW); strokeColor(Color(205, 30, 35)); strokeWidth(9.f); lineCap(BUTT); stroke();
        }
        // base arc
        {
            const float a0 = (-46.f) * PI / 180.f - PI / 2, a1 = (46.f) * PI / 180.f - PI / 2;
            beginPath(); arc(cx, cy, R, a0, a1, CW); strokeColor(Color(30, 32, 36)); strokeWidth(1.6f); stroke();
        }
        // ticks + numbers
        static const float marks[8] = { 20, 10, 7, 5, 3, 2, 1, 0 };
        static const char* const mlab[8] = { "20", "10", "7", "5", "3", "2", "1", "0" };
        for (int i = 0; i < 8; ++i) {
            float ax, ay, bx2, by2, lx, ly;
            const float p = grPos(marks[i]);
            pt(p, R, ax, ay); pt(p, R + 13, bx2, by2); pt(p, R + 30, lx, ly);
            strokeColor(Color(30, 32, 36)); strokeWidth(2.4f);
            beginPath(); moveTo(ax, ay); lineTo(bx2, by2); stroke();
            label(lx, ly, mlab[i], 17, marks[i] >= 7 ? Color(200, 30, 35) : Color(30, 32, 36));
        }
        for (float d = 0.5f; d < 10.f; d += (d < 5 ? 0.5f : 1.f)) {
            float ax, ay, bx2, by2;
            pt(grPos(d), R, ax, ay); pt(grPos(d), R + 7, bx2, by2);
            strokeWidth(1.2f); beginPath(); moveTo(ax, ay); lineTo(bx2, by2); stroke();
        }
        label(fx + 14, fy + 16, "dB", 15, Color(30, 32, 36), ALIGN_LEFT | ALIGN_MIDDLE);
        label(fx + fw - 16, fy + fh - 54, "GR", 22, Color(30, 32, 36), ALIGN_RIGHT | ALIGN_MIDDLE, 1);

        // peak LED (lights on heavy reduction)
        const float total = -(grClipDisp + grLimDisp);
        const bool hot = total > 6.f;
        beginPath(); circle(fx + fw - 20, fy + 16, 6);
        fillPaint(radialGradient(fx + fw - 22, fy + 14, 1, 7, hot ? Color(255, 200, 190) : Color(170, 90, 90), hot ? Color(230, 20, 20) : Color(90, 30, 30))); fill();
        if (hot) { beginPath(); circle(fx + fw - 20, fy + 16, 14); fillPaint(radialGradient(fx + fw - 20, fy + 16, 3, 14, Color(255, 40, 40, 0.5f), Color(255, 40, 40, 0.f))); fill(); }

        // LCD readouts
        lcd(fx + 10, fy + fh - 34, 128, 28, "LUFS", vals[kOutLufsI] > -70.f ? vals[kOutLufsI] : -100.f, false);
        lcd(fx + fw - 138, fy + fh - 34, 128, 28, "dBTP", vals[kOutTP], vals[kOutTP] > vals[kCeiling] + 0.05f);

        // needle
        const float gr = std::fmin(20.f, total);
        float nx, ny; pt(grPos(gr), R + 26, nx, ny);
        float sx, sy; pt(grPos(gr), 8, sx, sy);
        strokeColor(Color(0, 0, 0, 0.18f)); strokeWidth(3.f); lineCap(ROUND);
        beginPath(); moveTo(sx + 3, sy + 3); lineTo(nx + 3, ny + 3); stroke();
        strokeColor(Color(20, 20, 22)); strokeWidth(2.4f);
        beginPath(); moveTo(sx, sy); lineTo(nx, ny); stroke();
        // pivot dome
        beginPath(); ellipse(cx, fy + fh + 4, 46, 34); fillPaint(linearGradient(0, fy + fh - 30, 0, fy + fh, Color(50, 52, 56), Color(12, 12, 14))); fill();
        restore();

        // glass
        beginPath(); moveTo(fx, fy); lineTo(fx + fw * 0.6f, fy); lineTo(fx + fw * 0.38f, fy + fh); lineTo(fx, fy + fh); closePath();
        fillColor(Color(255, 255, 255, 0.10f)); fill();
        // inner shadow
        rrect(fx, fy, fw, fh, 6); strokePaint(linearGradient(0, fy, 0, fy + 10, Color(0, 0, 0, 0.45f), Color(0, 0, 0, 0.f))); strokeWidth(4); stroke();
    }

    void lcd(float x, float y, float w, float h, const char* unit, float v, bool red)
    {
        rrect(x, y, w, h, 4); fillColor(Color(8, 16, 24)); fill();
        rrect(x, y, w, h, 4); strokeColor(Color(0, 0, 0, 0.6f)); strokeWidth(1.5f); stroke();
        char buf[32];
        if (v > -99.f) std::snprintf(buf, sizeof(buf), "%.1f", v); else std::snprintf(buf, sizeof(buf), "--.-");
        glowLabel(x + w - 52, y + h / 2 + 1, buf, 19, red ? Color(255, 70, 70) : Color(80, 215, 255), ALIGN_RIGHT | ALIGN_MIDDLE);
        label(x + w - 8, y + h / 2 + 2, unit, 12, Color(90, 170, 210), ALIGN_RIGHT | ALIGN_MIDDLE);
    }

    // preset bar under the meter
    Rect pbPrev() const { return { 1003, 214, 30, 30 }; }
    Rect pbName() const { return { 1037, 214, 198, 30 }; }
    Rect pbNext() const { return { 1239, 214, 30, 30 }; }
    Rect pbSave() const { return { 1277, 214, 50, 30 }; }
    Rect pbAB()   const { return { 1331, 214, 40, 30 }; }
    Rect pbMore() const { return { 1375, 214, 45, 30 }; }

    void miniButton(const Rect& r, const char* t, bool on)
    {
        rrect(r.x, r.y, r.w, r.h, 5); fillPaint(linearGradient(0, r.y, 0, r.y + r.h, Color(62, 65, 72), Color(26, 27, 31))); fill();
        rrect(r.x + 0.5f, r.y + 0.5f, r.w - 1, r.h - 1, 5); strokeColor(on ? Color(90, 200, 255) : Color(90, 94, 100)); strokeWidth(1.2f); stroke();
        label(r.x + r.w / 2, r.y + r.h / 2 + 1, t, 14, on ? Color(110, 215, 255) : Color(220, 224, 230));
    }

    void drawPresetBar()
    {
        miniButton(pbPrev(), "<", false);
        const Rect n = pbName();
        rrect(n.x, n.y, n.w, n.h, 5); fillColor(Color(8, 16, 24)); fill();
        rrect(n.x, n.y, n.w, n.h, 5); strokeColor(Color(60, 90, 110)); strokeWidth(1.2f); stroke();
        std::string nm = currentPreset.empty() ? std::string("(unsaved)") : currentPreset;
        if (nm.size() > 20) nm = nm.substr(0, 19) + "..";
        label(n.x + n.w / 2, n.y + n.h / 2 + 1, nm.c_str(), 15, Color(110, 215, 255));
        miniButton(pbNext(), ">", false);
        miniButton(pbSave(), "SAVE", false);
        miniButton(pbAB(), abSlot ? "B" : "A", abInit);
        miniButton(pbMore(), "MORE", false);
    }

    // ------------------------------------------------------------ overlays in the centre window
    Rect ovClose() const { return { WIN_X1 - 44, 6, 36, 30 }; }
    Rect tabRect(int t) const { return { WIN_X0 + 14 + t * 226.f, 44, 220, 34 }; }
    Rect presetRect(int i) const { return { WIN_X0 + 14 + (i % 2) * 226.f, 88 + (i / 2) * 50.f, 220, 42 }; }
    Rect deleteRect(int k) const { const Rect r = presetRect(k); return { r.x + r.w - 36, r.y + 6, 30, r.h - 12 }; }
    Rect pageRect(int i) const { return { i == 0 ? WIN_X0 + 160 : WIN_X0 + 281, 292, 36, 30 }; }
    Rect nameRect() const { return { WIN_X0 + 24, 96, WIN_X1 - WIN_X0 - 48, 50 }; }
    Rect saveBtnRect() const { return { WIN_X0 + 246, 236, 206, 50 }; }
    Rect cancelBtnRect() const { return { WIN_X0 + 24, 236, 206, 50 }; }
    Rect itemRect(int i) const { return { WIN_X0 + 10 + (i % 4) * 115.f, 44 + (i / 4) * 74.f, 110, 68 }; }

    void drawOverlay()
    {
        const Color white(232, 238, 244), cyan(90, 200, 255), dim(140, 170, 190);
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
            label(WIN_X0 + 238, 70, "SAVE PRESET", 20, cyan, ALIGN_CENTER | ALIGN_MIDDLE, 3);
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
            label(WIN_X0 + 238, 346, "drag / scroll values  -  click switches  -  double-click = default", 12.5f, dim);
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
            else if (y > 150 && y < 206) { setParamGesture(kResetMeters, 1.f); vals[kResetMeters] = 0; toast("Meters reset"); }
            repaint();
            return true;
        }

        const int k = knobAt(x, y);
        if (k >= 0) {
            const int id = kKnobs[k].id;
            if (dbl) { setParamGesture(id, kParamInfo[id].def); repaint(); return true; }
            dragging = true; dragKnob = k; dragParam = id; dragStartY = y; dragStartNorm = toNorm(id, vals[id]);
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
        if (hitCircle(x, y, 165, 80, 24) || Rect{ 140, 195, 50, 90 }.hit(x, y)) {
            setParamGesture(kBypass, vals[kBypass] > 0.5f ? 0.f : 1.f); repaint(); return true;
        }
        if (Rect{ TOG_X - 26, 72, 52, 92 }.hit(x, y)) { setParamGesture(kTruePeak, vals[kTruePeak] > 0.5f ? 0.f : 1.f); repaint(); return true; }
        if (Rect{ TOG_X - 26, 202, 52, 92 }.hit(x, y)) { setParamGesture(kMonoSub, vals[kMonoSub] > 0.5f ? 0.f : 1.f); repaint(); return true; }
        return false;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (!dragging) return false;
        const float y = ev.pos.getY() / scaleF();
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
                    dragging = true; dragKnob = -1; dragParam = it.id; dragStartY = y; dragStartNorm = toNorm(it.id, vals[it.id]);
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
    int fontBold = -1;
#ifndef TL_MOCK
    NanoImage logo;
#endif
    int overlay = 0;          // 0 none, 2 presets, 3 save, 4 settings
    bool dragging = false;
    int dragKnob = -1, dragParam = 0; float dragStartY = 0, dragStartNorm = 0;
    double lastClick = 0, toastTime = -10; float lastClickX = 0, lastClickY = 0;
    char toastMsg[96] = {};
    int abSlot = 0; bool abInit = false; float abStore[2][kNumInputParams] = {};
    std::string currentPreset = "Default";
    std::vector<std::string> myPresets;
    int presetTab = 0, myPage = 0, deleteArm = -1;
    std::string saveName;
    bool nameFresh = false;
    float grClipDisp = 0.f, grLimDisp = 0.f;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrapWinUI)
};

UI* createUI() { return new TrapWinUI(); }

END_NAMESPACE_DISTRHO
