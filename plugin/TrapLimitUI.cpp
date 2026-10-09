// Trap Limit - phone-style UI (NanoVG)
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

START_NAMESPACE_DISTRHO

namespace {
const float W = 600.f, H = 960.f;

enum KeyType { KT_Select, KT_Toggle, KT_Radio, KT_Cycle, KT_Action };
enum Action { A_None, A_Reset, A_Help, A_Save, A_Load, A_ResetMeters, A_Shift, A_Engage, A_Bypass, A_Presets, A_AB };
enum Icon {
    I_None, I_Ceil, I_Clock, I_Attack, I_Release, I_Auto, I_Clip, I_Drive, I_Sat, I_808, I_Link,
    I_Split, I_Hpf, I_MS, I_Stereo, I_Punch, I_Mono, I_OS, I_Dither, I_TP, I_Trash,
    I_Head, I_Speaker, I_Sub, I_Soft, I_Hard, I_Tape, I_Save, I_Load, I_Enter,
    I_Power, I_Menu, I_AB, I_Delta, I_Handset
};

struct Key {
    float x, y, w, h;
    const char* letter;
    const char* label;
    KeyType type;
    int param;      // param id or -1
    float value;    // radio value
    int action;
    int icon;
};

struct Rect { float x, y, w, h; bool hit(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; } };

double nowSec()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}
} // namespace

class TrapLimitUI : public UI
{
public:
    TrapLimitUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        loadSharedResources();
        for (int i = 0; i < kNumInputParams; ++i) vals[i] = kParamInfo[i].def;
        for (int i = kNumInputParams; i < kParamCount; ++i) vals[i] = -100.f;
        vals[kOutGR] = 0.f; vals[kOutClipGR] = 0.f; vals[kOutLRA] = 0.f; vals[kOutCPU] = 0.f; vals[kOutLatency] = 0.f;
        buildKeys();
        setGeometryConstraints(300, 480, true);
        abInit = false;
    }

protected:
    // ------------------------------------------------------------ host -> UI
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
        if (!std::strcmp(pi.unit, "dB"))      std::snprintf(buf, n, "%+.1f dB", v);
        else if (!std::strcmp(pi.unit, "ms")) { if (v < 10.f) std::snprintf(buf, n, "%.2f ms", v); else std::snprintf(buf, n, "%.0f ms", v); }
        else if (!std::strcmp(pi.unit, "%"))  std::snprintf(buf, n, "%.0f %%", v);
        else std::snprintf(buf, n, "%.0f", v);
    }

    const char* shortName(int id) const
    {
        switch (id) {
        case kGain: return "GAIN"; case kCeiling: return "CEILING"; case kLookahead: return "LOOKAHEAD";
        case kAttack: return "ATTACK"; case kRelease: return "RELEASE"; case kDrive: return "DRIVE";
        case kSat: return "SATURATION"; case k808: return "808 BOOST"; case kLink: return "STEREO LINK";
        case kStereo: return "STEREO WIDTH"; case kPunch: return "PUNCH";
        }
        return kParamInfo[id].name;
    }

    void toast(const char* msg) { std::snprintf(toastMsg, sizeof(toastMsg), "%s", msg); toastTime = nowSec(); }

    // ------------------------------------------------------------ layout
    void buildKeys()
    {
        nKeys = 0;
        const float kx = 30.f, kw = 540.f;
        const float rowY[4] = { 544.f, 630.f, 716.f, 802.f };
        const float rh = 78.f;
        struct Def { const char* l; const char* lab; KeyType t; int p; float v; int a; int ic; };
        const Def r1[10] = {
            {"Q","CEILING",KT_Select,kCeiling,0,0,I_Ceil}, {"W","LOOK-AHEAD",KT_Select,kLookahead,0,0,I_Clock},
            {"E","ATTACK",KT_Select,kAttack,0,0,I_Attack}, {"R","RELEASE",KT_Select,kRelease,0,0,I_Release},
            {"T","AUTO-REL",KT_Toggle,kAutoRel,0,0,I_Auto}, {"Y","CLIP",KT_Toggle,kClip,0,0,I_Clip},
            {"U","DRIVE",KT_Select,kDrive,0,0,I_Drive}, {"I","SAT",KT_Select,kSat,0,0,I_Sat},
            {"O","808",KT_Select,k808,0,0,I_808}, {"P","LINK",KT_Select,kLink,0,0,I_Link} };
        const Def r2[10] = {
            {"A","SUB SPLIT",KT_Toggle,kSubSplit,0,0,I_Split}, {"S","HPF SC",KT_Toggle,kHpfSc,0,0,I_Hpf},
            {"D","M/S",KT_Toggle,kMS,0,0,I_MS}, {"F","STEREO",KT_Select,kStereo,0,0,I_Stereo},
            {"G","PUNCH",KT_Select,kPunch,0,0,I_Punch}, {"H","MONO SUB",KT_Toggle,kMonoSub,0,0,I_Mono},
            {"J","OVERSMP",KT_Cycle,kOversample,0,0,I_OS}, {"K","DITHER",KT_Toggle,kDither,0,0,I_Dither},
            {"L","TP ON",KT_Toggle,kTruePeak,0,0,I_TP}, {"DEL","RESET",KT_Action,-1,0,A_Reset,I_Trash} };
        const Def r3[10] = {
            {"ALT","HELP",KT_Action,-1,0,A_Help,I_None}, {"Z","-14 STREAM",KT_Radio,kStream,1,0,I_Head},
            {"X","-16 HI-FI",KT_Radio,kStream,2,0,I_Speaker}, {"C","-9 CLUB",KT_Radio,kStream,3,0,I_Sub},
            {"V","SOFT",KT_Radio,kClipShape,0,0,I_Soft}, {"B","HARD",KT_Radio,kClipShape,1,0,I_Hard},
            {"N","TAPE",KT_Radio,kClipShape,2,0,I_Tape}, {"M","SAVE",KT_Action,-1,0,A_Save,I_Save},
            {"","LOAD",KT_Action,-1,0,A_Load,I_Load}, {"","MTR RESET",KT_Action,-1,0,A_ResetMeters,I_Enter} };
        const Def* rows[3] = { r1, r2, r3 };
        for (int r = 0; r < 3; ++r) {
            for (int k = 0; k < 10; ++k) {
                const Def& d = rows[r][k];
                Key& key = keys[nKeys++];
                key.x = kx + k * (kw / 10.f) + 2.f; key.y = rowY[r]; key.w = kw / 10.f - 4.f; key.h = rh;
                key.letter = d.l; key.label = d.lab; key.type = d.t; key.param = d.p; key.value = d.v; key.action = d.a; key.icon = d.ic;
            }
        }
        const Def b[5] = {
            {"","SHIFT",KT_Action,-1,0,A_Shift,I_None}, {"","0dB",KT_Toggle,kUnity,0,0,I_None},
            {"","LIMIT",KT_Toggle,kLimit,0,0,I_None}, {"SYM","DELTA",KT_Toggle,kDelta,0,0,I_Delta},
            {"","SHIFT",KT_Action,-1,0,A_Shift,I_None} };
        const float bw[5] = { 90, 80, 200, 80, 90 };
        float x = kx;
        for (int k = 0; k < 5; ++k) {
            Key& key = keys[nKeys++];
            key.x = x + 2.f; key.y = rowY[3]; key.w = bw[k] - 4.f; key.h = rh - 6.f;
            key.letter = b[k].l; key.label = b[k].lab; key.type = b[k].t; key.param = b[k].p; key.value = b[k].v; key.action = b[k].a; key.icon = b[k].ic;
            x += bw[k];
        }
        // navigation row
        const float ny = 462.f, nh = 72.f;
        const Def nav[4] = {
            {"","ENGAGE",KT_Action,-1,0,A_Engage,I_Handset}, {"","PRESETS",KT_Action,-1,0,A_Presets,I_Menu},
            {"","A / B",KT_Action,-1,0,A_AB,I_AB}, {"","BYPASS",KT_Action,-1,0,A_Bypass,I_Handset} };
        const float nx[4] = { 32.f, 138.f, 356.f, 462.f };
        for (int k = 0; k < 4; ++k) {
            Key& key = keys[nKeys++];
            key.x = nx[k]; key.y = ny; key.w = 104.f; key.h = nh;
            key.letter = nav[k].l; key.label = nav[k].lab; key.type = nav[k].t; key.param = nav[k].p; key.value = 0; key.action = nav[k].a; key.icon = nav[k].ic;
        }
        trackpad = { 248.f, 458.f, 104.f, 80.f };
        for (int m = 0; m < 6; ++m) dock[m] = { 44.f + m * 86.f, 382.f, 80.f, 62.f };
        popupRect = { 120.f, 150.f, 250.f, 92.f };
    }

    bool keyActive(const Key& k) const
    {
        switch (k.type) {
        case KT_Select: return selected == k.param;
        case KT_Toggle: return vals[k.param] > 0.5f;
        case KT_Radio:  return std::fabs(vals[k.param] - k.value) < 0.1f;
        case KT_Cycle:  return vals[k.param] > 0.5f;
        default: break;
        }
        if (k.action == A_Shift) return fine;
        if (k.action == A_Help) return overlay == 1;
        if (k.action == A_Presets) return overlay == 2;
        if (k.action == A_Save) return overlay == 3;
        if (k.action == A_Load) return overlay == 2 && presetTab == 1;
        return false;
    }

    // ------------------------------------------------------------ drawing primitives
    void rrect(float x, float y, float w, float h, float r) { beginPath(); roundedRect(x, y, w, h, r); }
    void label(float x, float y, const char* s, float size, Color c, int align = ALIGN_CENTER | ALIGN_MIDDLE)
    {
        fontSize(size); fillColor(c); textAlign(align); text(x, y, s, nullptr);
    }
    void glowText(float x, float y, const char* s, float size, Color c, int align = ALIGN_CENTER | ALIGN_MIDDLE)
    {
        fontSize(size); textAlign(align);
        fontBlur(6.f); fillColor(Color(c.red, c.green, c.blue, 0.55f)); text(x, y, s, nullptr);
        fontBlur(0.f); fillColor(c); text(x, y, s, nullptr);
    }

    void drawIcon(int ic, float cx, float cy, float s, Color c)
    {
        strokeColor(c); fillColor(c); strokeWidth(1.6f); lineCap(ROUND); lineJoin(ROUND);
        const float h = s * 0.5f;
        switch (ic) {
        case I_Ceil:
            beginPath(); moveTo(cx - h, cy - h); lineTo(cx + h, cy - h); stroke();
            beginPath(); moveTo(cx, cy + h); lineTo(cx, cy - h * 0.5f); moveTo(cx - h * 0.4f, cy - h * 0.1f); lineTo(cx, cy - h * 0.5f); lineTo(cx + h * 0.4f, cy - h * 0.1f); stroke(); break;
        case I_Clock:
            beginPath(); circle(cx, cy, h); stroke();
            beginPath(); moveTo(cx, cy - h * 0.6f); lineTo(cx, cy); lineTo(cx + h * 0.45f, cy + h * 0.2f); stroke(); break;
        case I_Attack:
            beginPath(); moveTo(cx - h, cy + h * 0.6f); lineTo(cx - h * 0.2f, cy + h * 0.6f); lineTo(cx, cy - h); lineTo(cx + h * 0.3f, cy + h * 0.6f); lineTo(cx + h, cy + h * 0.6f); stroke(); break;
        case I_Release:
            beginPath(); moveTo(cx - h, cy - h); lineTo(cx - h, cy + h * 0.6f); moveTo(cx - h, cy - h); bezierTo(cx - h * 0.6f, cy + h * 0.5f, cx, cy + h * 0.6f, cx + h, cy + h * 0.6f); stroke(); break;
        case I_Auto:
            beginPath(); moveTo(cx - h, cy - h * 0.8f); bezierTo(cx - h * 0.5f, cy + h * 0.4f, cx, cy + h * 0.6f, cx + h, cy + h * 0.6f); stroke();
            for (int i = 0; i < 3; ++i) { beginPath(); circle(cx - h * 0.2f + i * h * 0.45f, cy - h * 0.6f, 1.4f); fill(); } break;
        case I_Clip:
            beginPath(); moveTo(cx - h, cy + h * 0.2f); lineTo(cx - h * 0.6f, cy - h * 0.6f); lineTo(cx + h * 0.1f, cy - h * 0.6f); lineTo(cx + h * 0.4f, cy + h * 0.7f); lineTo(cx + h, cy - h * 0.1f); stroke(); break;
        case I_Drive:
            beginPath(); arc(cx, cy + h * 0.3f, h, 3.4f, 6.02f, CW); stroke();
            beginPath(); moveTo(cx, cy + h * 0.3f); lineTo(cx + h * 0.55f, cy - h * 0.4f); stroke(); break;
        case I_Sat:
            for (int i = -1; i <= 1; ++i) { beginPath(); moveTo(cx + i * h * 0.6f, cy - h); bezierTo(cx + i * h * 0.6f + h * 0.4f, cy - h * 0.4f, cx + i * h * 0.6f - h * 0.4f, cy + h * 0.2f, cx + i * h * 0.6f, cy + h); stroke(); } break;
        case I_808: case I_Sub:
            beginPath(); circle(cx, cy, h); stroke(); beginPath(); circle(cx, cy, h * 0.55f); stroke(); beginPath(); circle(cx, cy, h * 0.15f); fill(); break;
        case I_Link:
            beginPath(); roundedRect(cx - h, cy - h * 0.35f, h * 1.2f, h * 0.7f, h * 0.35f); stroke();
            beginPath(); roundedRect(cx - h * 0.2f, cy - h * 0.35f, h * 1.2f, h * 0.7f, h * 0.35f); stroke(); break;
        case I_Split:
            beginPath(); moveTo(cx - h, cy - h * 0.7f); bezierTo(cx, cy - h * 0.7f, cx, cy + h * 0.7f, cx + h, cy + h * 0.7f);
            moveTo(cx - h, cy + h * 0.7f); bezierTo(cx, cy + h * 0.7f, cx, cy - h * 0.7f, cx + h, cy - h * 0.7f); stroke(); break;
        case I_Hpf:
            beginPath(); moveTo(cx - h, cy + h); bezierTo(cx - h * 0.3f, cy + h * 0.6f, cx - h * 0.2f, cy - h * 0.6f, cx + h, cy - h * 0.6f); stroke(); break;
        case I_MS:
            beginPath(); circle(cx - h * 0.4f, cy, h * 0.6f); stroke(); beginPath(); circle(cx + h * 0.4f, cy, h * 0.6f); stroke(); break;
        case I_Stereo:
            beginPath(); circle(cx, cy, h * 0.75f); stroke(); beginPath(); moveTo(cx, cy - h * 0.75f); lineTo(cx, cy + h * 0.75f); stroke(); break;
        case I_Punch:
            for (int i = 0; i < 8; ++i) { float a = i * 0.785f; beginPath(); moveTo(cx + std::cos(a) * h * 0.3f, cy + std::sin(a) * h * 0.3f); lineTo(cx + std::cos(a) * h, cy + std::sin(a) * h); stroke(); }
            beginPath(); circle(cx, cy, h * 0.22f); fill(); break;
        case I_Mono:
            beginPath(); circle(cx, cy, h * 0.75f); stroke(); beginPath(); circle(cx, cy, h * 0.3f); fill(); break;
        case I_Dither:
            for (int i = 0; i < 9; ++i) { beginPath(); circle(cx + ((i * 37) % 7 - 3) * h * 0.3f, cy + ((i * 53) % 7 - 3) * h * 0.3f, 1.2f); fill(); } break;
        case I_Trash:
            beginPath(); rect(cx - h * 0.6f, cy - h * 0.5f, h * 1.2f, h * 1.4f); stroke();
            beginPath(); moveTo(cx - h * 0.85f, cy - h * 0.65f); lineTo(cx + h * 0.85f, cy - h * 0.65f); stroke(); break;
        case I_Head:
            beginPath(); arc(cx, cy + h * 0.1f, h * 0.8f, 3.14159f, 0.f, CW); stroke();
            beginPath(); roundedRect(cx - h * 0.95f, cy, h * 0.4f, h * 0.8f, 2); fill();
            beginPath(); roundedRect(cx + h * 0.55f, cy, h * 0.4f, h * 0.8f, 2); fill(); break;
        case I_Speaker:
            beginPath(); moveTo(cx - h, cy - h * 0.3f); lineTo(cx - h * 0.5f, cy - h * 0.3f); lineTo(cx, cy - h * 0.8f); lineTo(cx, cy + h * 0.8f); lineTo(cx - h * 0.5f, cy + h * 0.3f); lineTo(cx - h, cy + h * 0.3f); closePath(); stroke();
            beginPath(); arc(cx + h * 0.1f, cy, h * 0.5f, -0.8f, 0.8f, CW); stroke(); beginPath(); arc(cx + h * 0.1f, cy, h * 0.9f, -0.8f, 0.8f, CW); stroke(); break;
        case I_Soft:
            beginPath(); moveTo(cx - h, cy + h * 0.7f); bezierTo(cx, cy + h * 0.5f, cx - h * 0.2f, cy - h * 0.6f, cx + h, cy - h * 0.6f); stroke(); break;
        case I_Hard:
            beginPath(); moveTo(cx - h, cy + h * 0.7f); lineTo(cx + h * 0.1f, cy - h * 0.6f); lineTo(cx + h, cy - h * 0.6f); stroke(); break;
        case I_Tape:
            beginPath(); roundedRect(cx - h, cy - h * 0.65f, h * 2, h * 1.3f, 2); stroke();
            beginPath(); circle(cx - h * 0.45f, cy - h * 0.1f, h * 0.25f); stroke(); beginPath(); circle(cx + h * 0.45f, cy - h * 0.1f, h * 0.25f); stroke(); break;
        case I_Save:
            beginPath(); moveTo(cx, cy - h); lineTo(cx, cy + h * 0.3f); moveTo(cx - h * 0.45f, cy - h * 0.15f); lineTo(cx, cy + h * 0.3f); lineTo(cx + h * 0.45f, cy - h * 0.15f); stroke();
            beginPath(); moveTo(cx - h, cy + h * 0.4f); lineTo(cx - h, cy + h); lineTo(cx + h, cy + h); lineTo(cx + h, cy + h * 0.4f); stroke(); break;
        case I_Load:
            beginPath(); moveTo(cx - h, cy - h * 0.6f); lineTo(cx - h * 0.2f, cy - h * 0.6f); lineTo(cx, cy - h * 0.35f); lineTo(cx + h, cy - h * 0.35f); lineTo(cx + h, cy + h * 0.7f); lineTo(cx - h, cy + h * 0.7f); closePath(); stroke(); break;
        case I_Enter:
            beginPath(); moveTo(cx + h * 0.8f, cy - h * 0.8f); lineTo(cx + h * 0.8f, cy + h * 0.2f); lineTo(cx - h * 0.8f, cy + h * 0.2f); stroke();
            beginPath(); moveTo(cx - h * 0.4f, cy - h * 0.2f); lineTo(cx - h * 0.8f, cy + h * 0.2f); lineTo(cx - h * 0.4f, cy + h * 0.6f); stroke(); break;
        case I_Power:
            beginPath(); arc(cx, cy, h * 0.8f, -1.0f, 4.14f, CW); stroke(); beginPath(); moveTo(cx, cy - h); lineTo(cx, cy - h * 0.1f); stroke(); break;
        case I_Menu:
            for (int i = -1; i <= 1; ++i) { beginPath(); moveTo(cx - h, cy + i * h * 0.55f); lineTo(cx + h, cy + i * h * 0.55f); stroke(); } break;
        case I_AB:
            beginPath(); moveTo(cx - h, cy - h * 0.35f); lineTo(cx + h, cy - h * 0.35f); moveTo(cx + h * 0.55f, cy - h * 0.75f); lineTo(cx + h, cy - h * 0.35f); lineTo(cx + h * 0.55f, cy + h * 0.05f); stroke();
            beginPath(); moveTo(cx + h, cy + h * 0.45f); lineTo(cx - h, cy + h * 0.45f); moveTo(cx - h * 0.55f, cy + h * 0.05f); lineTo(cx - h, cy + h * 0.45f); lineTo(cx - h * 0.55f, cy + h * 0.85f); stroke(); break;
        case I_Delta:
            beginPath(); moveTo(cx, cy - h * 0.8f); lineTo(cx + h * 0.9f, cy + h * 0.7f); lineTo(cx - h * 0.9f, cy + h * 0.7f); closePath(); stroke(); break;
        case I_Handset:
            strokeWidth(4.f);
            beginPath(); moveTo(cx - h * 1.1f, cy - h * 0.1f); bezierTo(cx - h * 0.6f, cy + h * 0.6f, cx + h * 0.6f, cy + h * 0.6f, cx + h * 1.1f, cy - h * 0.1f); stroke();
            strokeWidth(1.6f); break;
        default: break;
        }
    }

    // ------------------------------------------------------------ main draw
    void onNanoDisplay() override
    {
        const float sc = scaleF();
        save();
        scale(sc, sc);

        // background
        beginPath(); rect(0, 0, W, H); fillColor(Color(28, 29, 32)); fill();

        drawBody();
        drawScreen();
        drawNav();
        for (int i = 0; i < nKeys; ++i) if (keys[i].y > 530.f) drawKey(keys[i]);
        drawFrets();

        // chin text
        fontBlur(0);
        label(300.f, 916.f, "TrapVST", 20.f, Color(150, 154, 160));

        restore();
    }

    void drawBody()
    {
        // outer shadow
        beginPath(); roundedRect(4, 10, W - 8, H - 12, 74); fillPaint(boxGradient(4, 14, W - 8, H - 12, 74, 18, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        // chrome rim
        rrect(8, 6, W - 16, H - 14, 72);
        fillPaint(linearGradient(0, 0, W, 0, Color(90, 92, 98), Color(235, 238, 242)));
        fill();
        rrect(8, 6, W - 16, H - 14, 72);
        fillPaint(linearGradient(0, 0, 0, H, Color(250, 250, 252, 0.6f), Color(40, 42, 46, 0.6f)));
        fill();
        // black body
        rrect(15, 12, W - 30, H - 26, 66);
        fillPaint(linearGradient(0, 12, 0, H, Color(30, 31, 36), Color(6, 6, 8)));
        fill();
        // gloss
        rrect(20, 16, W - 40, 300, 62);
        fillPaint(linearGradient(0, 16, 0, 300, Color(255, 255, 255, 0.10f), Color(255, 255, 255, 0.f)));
        fill();
        // earpiece
        rrect(245, 26, 110, 14, 7); fillColor(Color(70, 72, 78)); fill();
        rrect(247, 28, 106, 10, 5); fillColor(Color(18, 18, 20)); fill();
        for (int i = 0; i < 24; ++i) { beginPath(); circle(252.f + i * 4.2f, 33.f, 0.9f); fillColor(Color(90, 92, 98)); fill(); }
        // sensor dots
        beginPath(); circle(520, 32, 5); fillColor(Color(25, 25, 30)); fill();
        beginPath(); circle(538, 32, 5); fillColor(Color(25, 25, 30)); fill();

        // title with limiter icon
        strokeColor(Color(200, 204, 210)); strokeWidth(2.f); lineJoin(ROUND);
        beginPath(); moveTo(202, 66); lineTo(220, 66);
        lineTo(224, 56); lineTo(228, 76); lineTo(232, 52); lineTo(236, 52); lineTo(240, 80); lineTo(244, 60); lineTo(248, 66); lineTo(262, 66); stroke();
        beginPath(); moveTo(222, 52); lineTo(262, 52); strokeColor(Color(0, 220, 255, 0.8f)); stroke();
        fontSize(34); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(Color(0, 0, 0, 0.6f)); text(273, 68, "LIMITER", nullptr);
        fillPaint(linearGradient(0, 50, 0, 84, Color(250, 250, 252), Color(140, 144, 150)));
        fillColor(Color(222, 226, 232)); text(272, 66, "LIMITER", nullptr);
    }

    void drawScreen()
    {
        // bezel
        rrect(28, 92, 544, 362, 10); fillColor(Color(12, 12, 14)); fill();
        rrect(28, 92, 544, 362, 10); strokeColor(Color(60, 62, 68)); strokeWidth(1.5f); stroke();
        // LCD
        const float sx = 36, sy = 98, sw = 528, sh = 350;
        rrect(sx, sy, sw, sh, 6);
        fillPaint(linearGradient(0, sy, 0, sy + sh, Color(8, 22, 38), Color(3, 8, 16)));
        fill();
        const Color cyan(0, 220, 255), cyanDim(0, 150, 190), white(225, 235, 245), grey(120, 140, 160), red(255, 50, 60);
        char buf[64];

        // status bar
        for (int i = 0; i < 4; ++i) { beginPath(); rect(46 + i * 6, 118 - (i + 1) * 4, 4, (i + 1) * 4); fillColor(cyan); fill(); }
        std::snprintf(buf, sizeof(buf), "CPU %.0f%%", vals[kOutCPU]); label(76, 110, buf, 13, white, ALIGN_LEFT | ALIGN_MIDDLE);
        std::snprintf(buf, sizeof(buf), "%.22s  |  %s", currentPreset.empty() ? "(unsaved)" : currentPreset.c_str(), abSlot ? "B" : "A");
        label(300, 110, buf, 13, white);
        const float tp = vals[kOutTP];
        const float headroom = tp > -99.f ? vals[kCeiling] - tp : std::fabs(vals[kCeiling]);
        std::snprintf(buf, sizeof(buf), "HEADROOM %.1f dB", headroom < 0 ? 0.f : headroom); label(512, 110, buf, 13, white, ALIGN_RIGHT | ALIGN_MIDDLE);
        rrect(518, 103, 34, 14, 2); strokeColor(cyan); strokeWidth(1.5f); stroke();
        beginPath(); rect(552, 107, 3, 6); fillColor(cyan); fill();
        const float bfill = std::fmax(0.05f, std::fmin(1.f, (headroom < 0 ? 0 : headroom) / 3.f));
        beginPath(); rect(520, 105, 30 * bfill, 10); fillColor(headroom < 0.05f ? red : cyan); fill();
        beginPath(); moveTo(sx, 122); lineTo(sx + sw, 122); strokeColor(Color(0, 120, 160, 0.4f)); strokeWidth(1); stroke();

        // waveform panel
        const float wx = 72, wy = 128, ww = 356, wh = 138;
        rrect(44, wy, 386, wh, 4); fillColor(Color(0, 10, 20, 0.6f)); fill();
        rrect(44, wy, 386, wh, 4); strokeColor(Color(0, 120, 160, 0.4f)); strokeWidth(1); stroke();
        const char* dbl[5] = { "0", "-6", "-12", "-18", "-24" };
        for (int i = 0; i < 5; ++i) {
            float yy = wy + 10 + i * (wh - 20) / 4.f;
            label(64, yy, dbl[i], 11, grey, ALIGN_RIGHT | ALIGN_MIDDLE);
            beginPath(); moveTo(wx, yy); lineTo(wx + ww, yy); strokeColor(Color(0, 120, 160, 0.15f)); stroke();
        }
        drawWave(wx, wy + 4, ww, wh - 8);

        // big readouts
        const float lufsI = vals[kOutLufsI];
        label(140, 284, "INTEGRATED", 13, cyanDim);
        if (lufsI > -70.f) std::snprintf(buf, sizeof(buf), "%.1f", lufsI); else std::snprintf(buf, sizeof(buf), "--.-");
        glowText(190, 312, buf, 40, cyan, ALIGN_RIGHT | ALIGN_MIDDLE);
        label(194, 318, "LUFS", 14, cyan, ALIGN_LEFT | ALIGN_MIDDLE);
        beginPath(); moveTo(238, 278); lineTo(238, 330); strokeColor(Color(0, 120, 160, 0.4f)); stroke();
        label(334, 284, "TRUE PEAK", 13, cyanDim);
        if (tp > -99.f) std::snprintf(buf, sizeof(buf), "%.1f", tp); else std::snprintf(buf, sizeof(buf), "--.-");
        glowText(374, 312, buf, 40, tp > vals[kCeiling] + 0.05f ? red : cyan, ALIGN_RIGHT | ALIGN_MIDDLE);
        label(378, 318, "dBTP", 14, cyan, ALIGN_LEFT | ALIGN_MIDDLE);

        // small readouts
        rrect(44, 336, 386, 40, 4); strokeColor(Color(0, 120, 160, 0.4f)); stroke();
        const float colx[3] = { 108, 237, 366 };
        const char* nm[3] = { "SHORT TERM", "MOMENTARY", "LRA" };
        for (int i = 0; i < 3; ++i) {
            label(colx[i], 346, nm[i], 11, cyanDim);
            float v = i == 0 ? vals[kOutLufsS] : i == 1 ? vals[kOutLufsM] : vals[kOutLRA];
            if (i == 2) std::snprintf(buf, sizeof(buf), "%.1f LU", v);
            else if (v > -70.f) std::snprintf(buf, sizeof(buf), "%.1f LUFS", v);
            else std::snprintf(buf, sizeof(buf), "-- LUFS");
            label(colx[i], 364, buf, 15, white);
            if (i < 2) { beginPath(); moveTo(colx[i] + 64, 340); lineTo(colx[i] + 64, 372); strokeColor(Color(0, 120, 160, 0.3f)); stroke(); }
        }

        drawMeters();

        // dock
        const char* dn[6] = { "TRANSPARENT", "PUNCHY", "LOUD", "DIRTY", "808", "SETTINGS" };
        const int mode = (int)(vals[kMode] + 0.5f);
        for (int m = 0; m < 6; ++m) {
            const Rect& r = dock[m];
            const bool act = (m < 4 && m == mode) || (m == 4 && selected == k808) || (m == 5 && overlay == 1);
            rrect(r.x, r.y, r.w, r.h, 8);
            fillPaint(linearGradient(0, r.y, 0, r.y + r.h, Color(20, 40, 70), Color(8, 16, 30))); fill();
            rrect(r.x, r.y, r.w, r.h, 8); strokeColor(act ? cyan : Color(60, 70, 120)); strokeWidth(act ? 2.f : 1.f); stroke();
            if (act) { rrect(r.x - 2, r.y - 2, r.w + 4, r.h + 4, 10); strokeColor(Color(0, 220, 255, 0.25f)); strokeWidth(3); stroke(); }
            const Color ic = act ? cyan : Color(170, 160, 255);
            const float cx = r.x + r.w / 2, cy = r.y + 24;
            switch (m) {
            case 0: drawIcon(I_Soft, cx, cy, 22, ic); break;
            case 1: drawIcon(I_Attack, cx, cy, 22, ic); break;
            case 2: for (int b = 0; b < 5; ++b) { beginPath(); rect(cx - 11 + b * 5, cy + 10 - (4 + (b % 3) * 5 + b), 3, 4 + (b % 3) * 5 + b); fillColor(ic); fill(); } break;
            case 3: drawIcon(I_Clip, cx, cy, 22, ic); break;
            case 4: drawIcon(I_808, cx, cy, 22, ic); break;
            case 5: drawIcon(I_Clock, cx, cy, 20, ic); break;
            }
            label(cx, r.y + r.h - 12, dn[m], m == 0 ? 10.5f : 11.5f, act ? cyan : Color(200, 190, 255));
        }

        drawPopup();
        drawOverlay();

        // toast
        if (nowSec() - toastTime < 1.6) {
            rrect(170, 220, 260, 40, 8); fillColor(Color(0, 0, 0, 0.85f)); fill();
            rrect(170, 220, 260, 40, 8); strokeColor(cyan); stroke();
            label(300, 240, toastMsg, 15, white);
        }

        // bypass dim
        if (vals[kBypass] > 0.5f) {
            rrect(sx, sy, sw, sh, 6); fillColor(Color(0, 0, 0, 0.55f)); fill();
            glowText(300, 240, "BYPASSED", 34, Color(255, 60, 60));
        }
        // glass reflection
        beginPath(); moveTo(sx, sy); lineTo(sx + sw * 0.55f, sy); lineTo(sx + sw * 0.35f, sy + sh); lineTo(sx, sy + sh); closePath();
        fillColor(Color(255, 255, 255, 0.025f)); fill();
    }

    void drawWave(float x, float y, float w, float h)
    {
        DisplayHistory* hist = nullptr;
        if (void* ptr = getPluginInstancePointer())
            hist = tlGetHistory(ptr);
        if (!hist) return;
        const int wp = hist->writePos;
        const float mid = y + h * 0.5f;
        const float colW = w / kHistoryLen;
        beginPath();
        for (int i = 0; i < kHistoryLen; ++i) {
            const int idx = (wp + i) % kHistoryLen;
            float a = std::fmin(1.f, hist->peak[idx]);
            float hh = a * h * 0.5f;
            rect(x + i * colW, mid - hh, colW + 0.3f, hh * 2);
        }
        fillPaint(linearGradient(0, y, 0, y + h, Color(0, 230, 255, 0.95f), Color(0, 120, 220, 0.85f)));
        fill();
        // gain reduction curve from top
        beginPath();
        moveTo(x, y);
        for (int i = 0; i < kHistoryLen; ++i) {
            const int idx = (wp + i) % kHistoryLen;
            float gr = std::fmax(-12.f, hist->gr[idx]);
            lineTo(x + i * colW, y + (-gr / 12.f) * h * 0.5f);
        }
        lineTo(x + w, y);
        closePath();
        fillColor(Color(255, 40, 50, 0.25f)); fill();
        beginPath();
        for (int i = 0; i < kHistoryLen; ++i) {
            const int idx = (wp + i) % kHistoryLen;
            float gr = std::fmax(-12.f, hist->gr[idx]);
            if (i == 0) moveTo(x, y + (-gr / 12.f) * h * 0.5f); else lineTo(x + i * colW, y + (-gr / 12.f) * h * 0.5f);
        }
        strokeColor(Color(255, 40, 50)); strokeWidth(1.6f); stroke();
    }

    void drawMeters()
    {
        const float px = 438, py = 128, pw = 118, ph = 248;
        rrect(px, py, pw, ph, 4); fillColor(Color(0, 10, 20, 0.6f)); fill();
        rrect(px, py, pw, ph, 4); strokeColor(Color(0, 120, 160, 0.4f)); strokeWidth(1); stroke();
        const Color grey(140, 160, 180);
        label(px + 20, py + 12, "IN", 11, grey); label(px + 60, py + 12, "GR", 11, Color(255, 120, 90)); label(px + 98, py + 12, "OUT", 11, grey);
        const float my = py + 26, mh = ph - 36;
        const int segs = 30;
        auto meter = [&](float x, float db, bool gr) {
            for (int s = 0; s < segs; ++s) {
                const float sy = my + mh - (s + 1) * (mh / segs);
                bool lit; Color c;
                if (!gr) {
                    const float segDb = -30.f + (s + 1) * 1.f; // 0..-30 dB, 1 dB per segment
                    lit = db >= segDb - 1.f;
                    c = segDb > -1.f ? Color(255, 60, 60) : (segDb > -6.f ? Color(120, 240, 255) : Color(0, 200, 255));
                } else {
                    // total reduction = clipper (orange, on top) + limiter (red, below)
                    const int fromTop = segs - 1 - s;   // top-down
                    const float step = 12.f / segs;               // 0.4 dB per segment
                    const float depth = fromTop * step;
                    lit = -db > depth + 0.1f;                     // nothing lit at 0 dB
                    c = depth < -grClipDisp ? Color(255, 150, 30) : Color(255, 40, 60);
                }
                beginPath(); rect(x, sy + 1, 12, mh / segs - 2);
                fillColor(lit ? c : Color(c.red, c.green, c.blue, 0.10f)); fill();
            }
        };
        const float inDb = std::fmax(vals[kOutInL], vals[kOutInR]);
        const float outDb = std::fmax(vals[kOutOutL], vals[kOutOutR]);
        meter(px + 8, inDb, false);
        // smooth fall-back so short reductions stay visible
        grClipDisp = std::fmin(vals[kOutClipGR], grClipDisp + 0.35f);
        grLimDisp  = std::fmin(vals[kOutGR],     grLimDisp  + 0.35f);
        if (grClipDisp > 0.f) grClipDisp = 0.f;
        if (grLimDisp > 0.f) grLimDisp = 0.f;
        meter(px + 54, grClipDisp + grLimDisp, true);
        meter(px + 92, outDb, false);
        // scales
        const char* s1[6] = { "0", "-6", "-12", "-18", "-24", "-30" };
        for (int i = 0; i < 6; ++i) label(px + 36, my + i * (mh / 5.f), s1[i], 10.5f, grey);
        const char* s2[5] = { "0", "-3", "-6", "-9", "-12" };
        for (int i = 0; i < 5; ++i) label(px + 78, my + i * (mh / 4.f), s2[i], 10.5f, Color(255, 110, 120));
    }

    void drawPopup()
    {
        if (selected < 0) return;
        if (!dragging && nowSec() - lastEdit > 1.8) return;
        const Rect& r = popupRect;
        rrect(r.x, r.y, r.w, r.h, 10); fillColor(Color(10, 30, 50, 0.88f)); fill();
        rrect(r.x, r.y, r.w, r.h, 10); strokeColor(Color(0, 220, 255, 0.8f)); strokeWidth(1.5f); stroke();
        char buf[48]; formatValue(selected, buf, sizeof(buf));
        label(r.x + r.w / 2, r.y + 20, shortName(selected), 15, Color(200, 225, 240));
        glowText(r.x + r.w / 2, r.y + 46, buf, 24, Color(0, 220, 255));
        const float n = toNorm(selected, vals[selected]);
        const float sx = r.x + 20, sw = r.w - 40, sy = r.y + 74;
        rrect(sx, sy - 3, sw, 6, 3); fillColor(Color(40, 60, 80)); fill();
        rrect(sx, sy - 3, sw * n, 6, 3); fillColor(Color(0, 220, 255)); fill();
        beginPath(); circle(sx + sw * n, sy, 8); fillColor(Color(0, 220, 255)); fill();
        beginPath(); circle(sx + sw * n, sy, 4); fillColor(Color(220, 250, 255)); fill();
    }

    void drawOverlay()
    {
        if (overlay == 0) return;
        const float x = 40, y = 126, w = 520, h = 250;
        rrect(x, y, w, h, 8); fillColor(Color(4, 12, 24, 0.96f)); fill();
        rrect(x, y, w, h, 8); strokeColor(Color(0, 220, 255)); strokeWidth(1.5f); stroke();
        const Color white(225, 235, 245), cyan(0, 220, 255);
        if (overlay == 1) {
            label(x + w / 2, y + 16, "HELP  -  click anywhere on screen to close", 13, cyan);
            const char* lines[] = {
                "Blue keys (CEILING, ATTACK...) select a value for the TRACKPAD.",
                "Drag the trackpad up/down or scroll on any key to change values.",
                "Hold SHIFT key (toggle) for fine adjustment. Double-click = default.",
                "Click trackpad without dragging = back to GAIN.",
                "GR meter: orange = clipper reduction, red = limiter reduction.",
                "SUB SPLIT: clips sub-bass (<120 Hz) separately. MONO SUB: mono lows.",
                "HPF SC: limiter ignores sub when detecting. TP ON: true-peak.",
                "-14 / -16 / -9: hear your track at streaming/club loudness.",
                "0dB: loudness-matched listening. DELTA: hear what is removed.",
                "SAVE: name + store your preset. LOAD / PRESETS > MY PRESETS to pick it.",
            };
            for (int i = 0; i < 10; ++i) label(x + 12, y + 42 + i * 20, lines[i], 13.f, white, ALIGN_LEFT | ALIGN_MIDDLE);
        } else if (overlay == 2) {
            // tabs
            for (int t = 0; t < 2; ++t) {
                const Rect r = tabRect(t);
                const bool act = presetTab == t;
                rrect(r.x, r.y, r.w, r.h, 6); fillColor(act ? Color(0, 90, 130) : Color(10, 22, 38)); fill();
                rrect(r.x, r.y, r.w, r.h, 6); strokeColor(act ? cyan : Color(0, 110, 150)); strokeWidth(act ? 1.8f : 1.f); stroke();
                label(r.x + r.w / 2, r.y + r.h / 2, t == 0 ? "FACTORY" : "MY PRESETS", 14, act ? Color(255, 255, 255) : Color(150, 190, 210));
            }
            if (presetTab == 0) {
                for (int i = 0; i < kNumPresets; ++i) {
                    const Rect r = presetRect(i);
                    const bool cur = currentPreset == kPresets[i].name;
                    rrect(r.x, r.y, r.w, r.h, 6); fillColor(cur ? Color(0, 90, 130) : Color(15, 30, 50)); fill();
                    rrect(r.x, r.y, r.w, r.h, 6); strokeColor(Color(0, 160, 200)); strokeWidth(1); stroke();
                    label(r.x + r.w / 2, r.y + r.h / 2, kPresets[i].name, 15, white);
                }
            } else {
                if (myPresets.empty()) {
                    label(x + w / 2, y + 100, "No saved presets yet.", 15, white);
                    label(x + w / 2, y + 126, "Set your sound, then press SAVE (M key).", 13, Color(150, 190, 210));
                }
                const int pages = std::max(1, ((int)myPresets.size() + 5) / 6);
                if (myPage >= pages) myPage = pages - 1;
                for (int k = 0; k < 6; ++k) {
                    const int i = myPage * 6 + k;
                    if (i >= (int)myPresets.size()) break;
                    const Rect r = presetRect(k);
                    const bool cur = currentPreset == myPresets[i];
                    rrect(r.x, r.y, r.w, r.h, 6); fillColor(cur ? Color(0, 90, 130) : Color(15, 30, 50)); fill();
                    rrect(r.x, r.y, r.w, r.h, 6); strokeColor(Color(0, 160, 200)); strokeWidth(1); stroke();
                    label(r.x + 12, r.y + r.h / 2, myPresets[i].c_str(), 14, white, ALIGN_LEFT | ALIGN_MIDDLE);
                    const Rect d = deleteRect(k);
                    const bool armed = deleteArm == i;
                    rrect(d.x, d.y, d.w, d.h, 5); fillColor(armed ? Color(200, 30, 40) : Color(30, 40, 55)); fill();
                    if (armed) label(d.x + d.w / 2, d.y + d.h / 2, "DEL?", 10, Color(255, 255, 255));
                    else drawIcon(I_Trash, d.x + d.w / 2, d.y + d.h / 2, 14, Color(170, 180, 190));
                }
                if (pages > 1) {
                    char pg[32]; std::snprintf(pg, sizeof(pg), "%d / %d", myPage + 1, pages);
                    label(x + w / 2, 356, pg, 13, white);
                    const Rect pl = pageRect(0), pr = pageRect(1);
                    rrect(pl.x, pl.y, pl.w, pl.h, 5); strokeColor(cyan); stroke(); label(pl.x + pl.w / 2, pl.y + pl.h / 2, "<", 16, cyan);
                    rrect(pr.x, pr.y, pr.w, pr.h, 5); strokeColor(cyan); stroke(); label(pr.x + pr.w / 2, pr.y + pr.h / 2, ">", 16, cyan);
                }
            }
        } else if (overlay == 3) {
            label(x + w / 2, y + 22, "SAVE PRESET", 16, cyan);
            const Rect f = nameRect();
            rrect(f.x, f.y, f.w, f.h, 6); fillColor(Color(0, 0, 0, 0.6f)); fill();
            rrect(f.x, f.y, f.w, f.h, 6); strokeColor(cyan); strokeWidth(1.5f); stroke();
            std::string shown = saveName;
            if (std::fmod(nowSec(), 1.0) < 0.55) shown += "|";
            label(f.x + 14, f.y + f.h / 2, shown.c_str(), 18, white, ALIGN_LEFT | ALIGN_MIDDLE);
            label(x + w / 2, f.y + f.h + 20, "Just type a new name, then press SAVE / Enter. Same name = overwrite.", 12, Color(150, 190, 210));
            const Rect sv = saveBtnRect(), cn = cancelBtnRect();
            rrect(sv.x, sv.y, sv.w, sv.h, 8); fillColor(Color(0, 120, 160)); fill();
            rrect(sv.x, sv.y, sv.w, sv.h, 8); strokeColor(cyan); stroke();
            label(sv.x + sv.w / 2, sv.y + sv.h / 2, "SAVE", 17, Color(255, 255, 255));
            rrect(cn.x, cn.y, cn.w, cn.h, 8); fillColor(Color(25, 32, 45)); fill();
            rrect(cn.x, cn.y, cn.w, cn.h, 8); strokeColor(Color(120, 130, 140)); stroke();
            label(cn.x + cn.w / 2, cn.y + cn.h / 2, "CANCEL", 15, Color(200, 205, 210));
        }
    }
    Rect tabRect(int t) const { return { 56.f + t * 248.f, 134.f, 240.f, 30.f }; }
    Rect deleteRect(int k) const { const Rect r = presetRect(k); return { r.x + r.w - 40.f, r.y + 8.f, 32.f, r.h - 16.f }; }
    Rect pageRect(int i) const { return { i == 0 ? 220.f : 344.f, 342.f, 36.f, 28.f }; }
    Rect nameRect() const { return { 70.f, 176.f, 460.f, 46.f }; }
    Rect saveBtnRect() const { return { 310.f, 290.f, 220.f, 48.f }; }
    Rect cancelBtnRect() const { return { 70.f, 290.f, 220.f, 48.f }; }
    Rect presetRect(int i) const { return { 56.f + (i % 2) * 248.f, 172.f + (i / 2) * 56.f, 240.f, 48.f }; }

    void drawNav()
    {
        // nav background plate
        rrect(26, 456, 548, 82, 10); fillPaint(linearGradient(0, 456, 0, 538, Color(28, 28, 32), Color(10, 10, 12))); fill();
        for (int i = 0; i < nKeys; ++i) if (keys[i].y > 455.f && keys[i].y < 470.f) drawNavKey(keys[i]);
        // trackpad
        const Rect& t = trackpad;
        rrect(t.x, t.y, t.w, t.h, 18);
        fillPaint(linearGradient(t.x, t.y, t.x + t.w, t.y + t.h, Color(250, 252, 255), Color(90, 94, 100))); fill();
        rrect(t.x + 5, t.y + 5, t.w - 10, t.h - 10, 14);
        fillPaint(linearGradient(0, t.y, 0, t.y + t.h, Color(30, 34, 40), Color(4, 6, 10))); fill();
        const int id = selected >= 0 ? selected : kGain;
        char buf[48]; formatValue(id, buf, sizeof(buf));
        label(t.x + t.w / 2, t.y + 22, shortName(id), 11.5f, Color(140, 200, 230));
        glowText(t.x + t.w / 2, t.y + 44, buf, 16, Color(0, 220, 255));
        label(t.x + t.w / 2, t.y + 63, fine ? "FINE" : "EDIT", 11, Color(0, 160, 200));
    }

    void drawNavKey(const Key& k)
    {
        rrect(k.x, k.y, k.w, k.h, 12);
        fillPaint(linearGradient(0, k.y, 0, k.y + k.h, Color(46, 47, 52), Color(12, 12, 14))); fill();
        rrect(k.x, k.y, k.w, k.h, 12); strokeColor(Color(80, 82, 88)); strokeWidth(1); stroke();
        const float cx = k.x + k.w / 2;
        Color c(230, 232, 236);
        if (k.action == A_Engage) c = vals[kBypass] < 0.5f ? Color(60, 255, 90) : Color(40, 120, 50);
        if (k.action == A_Bypass) c = vals[kBypass] > 0.5f ? Color(255, 50, 50) : Color(150, 40, 40);
        if (k.action == A_Presets && overlay == 2) c = Color(0, 220, 255);
        if (k.icon == I_Handset && k.action == A_Bypass) {
            // hang-up handset (arc upward)
            strokeColor(c); strokeWidth(5.f); lineCap(ROUND);
            beginPath(); moveTo(cx - 16, k.y + 30); bezierTo(cx - 10, k.y + 16, cx + 10, k.y + 16, cx + 16, k.y + 30); stroke();
            strokeWidth(1.6f);
        } else if (k.icon == I_Handset) {
            strokeColor(c); strokeWidth(5.f); lineCap(ROUND);
            beginPath(); moveTo(cx - 16, k.y + 18); lineTo(cx - 16, k.y + 24); bezierTo(cx - 8, k.y + 30, cx + 8, k.y + 30, cx + 16, k.y + 24); lineTo(cx + 16, k.y + 18); stroke();
            strokeWidth(1.6f);
        } else drawIcon(k.icon, cx, k.y + 24, 22, c);
        if (k.action == A_AB) {
            label(cx - 12, k.y + 54, "A", 15, abSlot == 0 ? Color(0, 220, 255) : Color(150, 150, 155));
            label(cx, k.y + 54, "/", 15, Color(150, 150, 155));
            label(cx + 12, k.y + 54, "B", 15, abSlot == 1 ? Color(0, 220, 255) : Color(150, 150, 155));
        } else label(cx, k.y + 54, k.label, 14, c);
    }

    void drawKey(const Key& k)
    {
        const bool act = keyActive(k);
        const bool strong = act && (k.type == KT_Select);
        const bool pressedNow = (pressedKey >= 0 && &keys[pressedKey] == &k);
        // key body
        rrect(k.x, k.y + 2, k.w, k.h, 9); fillColor(Color(0, 0, 0, 0.7f)); fill();
        rrect(k.x, k.y, k.w, k.h, 9);
        fillPaint(linearGradient(0, k.y, 0, k.y + k.h,
                                 pressedNow ? Color(20, 20, 24) : Color(58, 60, 66),
                                 pressedNow ? Color(40, 42, 46) : Color(10, 10, 12)));
        fill();
        // top gloss
        rrect(k.x + 3, k.y + 2, k.w - 6, k.h * 0.4f, 7);
        fillPaint(linearGradient(0, k.y, 0, k.y + k.h * 0.4f, Color(255, 255, 255, 0.16f), Color(255, 255, 255, 0.f))); fill();
        if (act) {
            rrect(k.x, k.y, k.w, k.h, 9);
            fillPaint(boxGradient(k.x, k.y, k.w, k.h, 9, 14, Color(0, 200, 255, strong ? 0.10f : 0.05f), Color(0, 160, 255, strong ? 0.45f : 0.25f)));
            fill();
            rrect(k.x + 1, k.y + 1, k.w - 2, k.h - 2, 8); strokeColor(Color(0, 220, 255, strong ? 1.f : 0.7f)); strokeWidth(strong ? 2.2f : 1.5f); stroke();
        } else {
            rrect(k.x + 0.5f, k.y + 0.5f, k.w - 1, k.h - 1, 9); strokeColor(Color(90, 92, 98, 0.6f)); strokeWidth(1); stroke();
        }
        const Color lc = act ? Color(0, 230, 255) : Color(235, 237, 240);
        const float cx = k.x + k.w / 2;
        // letter
        if (k.letter && k.letter[0]) label(k.x + 6, k.y + 11, k.letter, 11, Color(210, 212, 216), ALIGN_LEFT | ALIGN_MIDDLE);
        const bool bottom = k.y > 800.f;
        if (bottom) {
            if (k.icon != I_None) { drawIcon(k.icon, cx, k.y + 24, 16, lc); label(cx, k.y + 52, k.label, 15, lc); }
            else label(cx, k.y + k.h / 2, k.label, k.param == kLimit ? 22.f : 16.f, lc);
            if (k.param == kLimit) {
                rrect(cx - 60, k.y + k.h - 12, 120, 3, 1.5f);
                fillColor(vals[kLimit] > 0.5f ? Color(0, 220, 255) : Color(60, 70, 80)); fill();
            }
            return;
        }
        if (k.icon == I_OS || k.icon == I_TP) {
            const char* t = k.icon == I_TP ? "TP" : kOversampleNames[(int)(vals[kOversample] + 0.5f)];
            label(cx, k.y + 32, t, 18, lc);
        } else if (k.icon != I_None) {
            drawIcon(k.icon, cx, k.y + 32, 18, lc);
        } else if (!std::strcmp(k.label, "HELP")) {
            label(cx, k.y + 32, "?", 20, lc);
        }
        // label (split two words)
        const char* sp = std::strchr(k.label, ' ');
        if (!sp && k.label[0]) sp = std::strchr(k.label + 1, '-');
        const size_t ln = std::strlen(k.label);
        if (sp && ln > 6) {
            char a[16] = {}; std::snprintf(a, sizeof(a), "%.*s", (int)(sp - k.label), k.label);
            label(cx, k.y + 55, a, 11, lc); label(cx, k.y + 68, sp + 1, 11, lc);
        } else label(cx, k.y + 62, k.label, ln >= 7 ? 10.5f : 11.5f, lc);
    }

    void drawFrets()
    {
        const float ys[4] = { 540.f, 626.f, 712.f, 798.f };
        for (int i = 0; i < 4; ++i) {
            beginPath(); rect(26, ys[i], 548, 3);
            fillPaint(linearGradient(26, 0, 574, 0, Color(120, 122, 128), Color(240, 242, 246)));
            fill();
        }
    }

    // ------------------------------------------------------------ interaction
    int keyAt(float x, float y) const
    {
        for (int i = 0; i < nKeys; ++i) {
            const Key& k = keys[i];
            if (x >= k.x && x < k.x + k.w && y >= k.y && y < k.y + k.h) return i;
        }
        return -1;
    }

    bool onMouse(const MouseEvent& ev) override
    {
        const float sc = scaleF();
        const float x = ev.pos.getX() / sc, y = ev.pos.getY() / sc;
        if (ev.button != 1) {
            // right-click on a value key: reset to default
            if (ev.press && ev.button == 3) {
                int ki = keyAt(x, y);
                if (ki >= 0 && keys[ki].type == KT_Select) { setParamGesture(keys[ki].param, kParamInfo[keys[ki].param].def); lastEdit = nowSec(); return true; }
            }
            return false;
        }
        if (!ev.press) {
            if (dragging) {
                editParameter(dragParam, false);
                if (!dragMoved && dragFromTrackpad) { selected = kGain; lastEdit = nowSec(); }
                dragging = false;
            }
            pressedKey = -1;
            repaint();
            return true;
        }

        const double t = nowSec();
        const bool dbl = (t - lastClick < 0.35) && std::fabs(x - lastClickX) < 6 && std::fabs(y - lastClickY) < 6;
        lastClick = t; lastClickX = x; lastClickY = y;

        // overlay on screen
        if (overlay != 0 && x > 36 && x < 564 && y > 98 && y < 448) {
            overlayClick(x, y);
            repaint(); return true;
        }

        // popup slider
        if (selected >= 0 && (t - lastEdit < 1.8) && popupRect.hit(x, y)) {
            startDrag(selected, x, y, false);
            dragHoriz = true;
            const float n = (x - (popupRect.x + 20)) / (popupRect.w - 40);
            setParam(selected, fromNorm(selected, n));
            dragMoved = true;
            return true;
        }

        // trackpad
        if (trackpad.hit(x, y)) {
            const int id = selected >= 0 ? selected : kGain;
            if (dbl) { setParamGesture(id, kParamInfo[id].def); lastEdit = t; return true; }
            startDrag(id, x, y, true);
            return true;
        }

        // dock tiles
        for (int m = 0; m < 6; ++m) if (dock[m].hit(x, y)) {
            if (m < 4) setParamGesture(kMode, (float)m);
            else if (m == 4) { selected = k808; lastEdit = t; }
            else overlay = overlay == 1 ? 0 : 1;
            repaint(); return true;
        }

        const int ki = keyAt(x, y);
        if (ki < 0) return false;
        pressedKey = ki;
        Key& k = keys[ki];
        switch (k.type) {
        case KT_Select:
            if (dbl && selected == k.param) { setParamGesture(k.param, kParamInfo[k.param].def); }
            selected = k.param; lastEdit = t;
            startDrag(k.param, x, y, false);  // allow drag directly on the key
            break;
        case KT_Toggle: setParamGesture(k.param, vals[k.param] > 0.5f ? 0.f : 1.f); break;
        case KT_Radio:
            if (k.param == kStream && std::fabs(vals[kStream] - k.value) < 0.1f) setParamGesture(kStream, 0.f);
            else setParamGesture(k.param, k.value);
            if (k.param == kClipShape && vals[kClip] < 0.5f) setParamGesture(kClip, 1.f);
            break;
        case KT_Cycle: setParamGesture(k.param, (float)(((int)(vals[k.param] + 0.5f) + 1) % 3)); break;
        case KT_Action: doAction(k.action); break;
        }
        repaint();
        return true;
    }

    void startDrag(int id, float x, float y, bool fromTrackpad)
    {
        dragging = true; dragParam = id; dragStartY = y; dragStartX = x; dragMoved = false; dragHoriz = false;
        dragFromTrackpad = fromTrackpad;
        dragStartNorm = toNorm(id, vals[id]);
        editParameter(id, true);
        lastEdit = nowSec();
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (!dragging) return false;
        const float sc = scaleF();
        const float x = ev.pos.getX() / sc, y = ev.pos.getY() / sc;
        if (std::fabs(y - dragStartY) > 2 || std::fabs(x - dragStartX) > 2) dragMoved = true;
        if (!dragMoved) return true;
        float n;
        if (dragHoriz) n = (x - (popupRect.x + 20)) / (popupRect.w - 40);
        else {
            const bool f = fine || (ev.mod & kModifierShift);
            n = dragStartNorm + (dragStartY - y) / (f ? 1200.f : 220.f);
        }
        setParam(dragParam, fromNorm(dragParam, n));
        lastEdit = nowSec();
        repaint();
        return true;
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        const float sc = scaleF();
        const float x = ev.pos.getX() / sc, y = ev.pos.getY() / sc;
        int id = -1;
        if (trackpad.hit(x, y) || popupRect.hit(x, y)) id = selected >= 0 ? selected : kGain;
        else { int ki = keyAt(x, y); if (ki >= 0 && keys[ki].type == KT_Select) { id = keys[ki].param; selected = id; } }
        if (id < 0) return false;
        const bool f = fine || (ev.mod & kModifierShift);
        const float n = toNorm(id, vals[id]) + (float)ev.delta.getY() * (f ? 0.004f : 0.025f);
        setParamGesture(id, fromNorm(id, n));
        lastEdit = nowSec();
        repaint();
        return true;
    }

    // ------------------------------------------------------------ actions
    void doAction(int a)
    {
        switch (a) {
        case A_Reset:
            for (int i = 0; i < kNumInputParams; ++i)
                if (i != kBypass && i != kResetMeters) setParamGesture(i, kParamInfo[i].def);
            selected = kGain; toast("All settings reset");
            break;
        case A_Help: overlay = overlay == 1 ? 0 : 1; break;
        case A_Presets: if (overlay == 2) overlay = 0; else { refreshMyPresets(); overlay = 2; deleteArm = -1; } break;
        case A_Save: openSave(); break;
        case A_Load: refreshMyPresets(); presetTab = 1; deleteArm = -1; overlay = 2; break;
        case A_ResetMeters: setParamGesture(kResetMeters, 1.f); vals[kResetMeters] = 0; toast("Meters reset"); break;
        case A_Shift: fine = !fine; break;
        case A_Engage: setParamGesture(kBypass, 0.f); break;
        case A_Bypass: setParamGesture(kBypass, 1.f); break;
        case A_AB: swapAB(); break;
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

    void overlayClick(float x, float y)
    {
        const bool inBox = x >= 40 && x <= 560 && y >= 126 && y <= 376;
        if (overlay == 1 || !inBox) { overlay = 0; return; }
        if (overlay == 2) {
            for (int t = 0; t < 2; ++t) if (tabRect(t).hit(x, y)) { presetTab = t; deleteArm = -1; if (t == 1) refreshMyPresets(); return; }
            if (presetTab == 0) {
                for (int i = 0; i < kNumPresets; ++i) if (presetRect(i).hit(x, y)) { applyPreset(i); overlay = 0; return; }
                return;
            }
            const int pages = std::max(1, ((int)myPresets.size() + 5) / 6);
            if (pages > 1) {
                if (pageRect(0).hit(x, y)) { myPage = (myPage + pages - 1) % pages; deleteArm = -1; return; }
                if (pageRect(1).hit(x, y)) { myPage = (myPage + 1) % pages; deleteArm = -1; return; }
            }
            for (int k = 0; k < 6; ++k) {
                const int i = myPage * 6 + k;
                if (i >= (int)myPresets.size()) break;
                if (deleteRect(k).hit(x, y)) {
                    if (deleteArm == i) { deleteArm = -1; deleteMyPreset(myPresets[i]); }
                    else deleteArm = i;
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
        }
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
    Key keys[48]; int nKeys = 0;
    Rect trackpad {}, dock[6] {}, popupRect {};
    int selected = kGain;
    int overlay = 0;          // 0 none, 1 help, 2 presets, 3 save
    int pressedKey = -1;
    bool fine = false;
    bool dragging = false, dragMoved = false, dragHoriz = false, dragFromTrackpad = false;
    int dragParam = 0; float dragStartY = 0, dragStartX = 0, dragStartNorm = 0;
    double lastEdit = -10, lastClick = 0, toastTime = -10; float lastClickX = 0, lastClickY = 0;
    char toastMsg[96] = {};
    int abSlot = 0; bool abInit = false; float abStore[2][kNumInputParams] = {};
    std::string currentPreset = "Default";
    std::vector<std::string> myPresets;
    int presetTab = 0, myPage = 0, deleteArm = -1;
    std::string saveName;
    bool nameFresh = false;
    float grClipDisp = 0.f, grLimDisp = 0.f;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrapLimitUI)
};

UI* createUI() { return new TrapLimitUI(); }

END_NAMESPACE_DISTRHO
