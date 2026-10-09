// Minimal mock of DPF UI + NanoVG that records drawing as HTML5 canvas JS (for preview screenshots)
#pragma once
#include <string>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstdlib>

#define START_NAMESPACE_DISTRHO namespace DISTRHO {
#define END_NAMESPACE_DISTRHO }
#define USE_NAMESPACE_DISTRHO using namespace DISTRHO;
#define DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(x)
#define DISTRHO_UI_DEFAULT_WIDTH 600
#define DISTRHO_UI_DEFAULT_HEIGHT 960
typedef unsigned int uint;

namespace DISTRHO {

enum { kModifierShift = 1, kKeyBackspace = 8, kKeyEnter = 13, kKeyEscape = 27, kKeyDelete = 127 };

struct Color {
    float red, green, blue, alpha;
    Color() : red(0), green(0), blue(0), alpha(1) {}
    Color(int r, int g, int b, float a = 1.f) : red(r / 255.f), green(g / 255.f), blue(b / 255.f), alpha(a) {}
    Color(float r, float g, float b, float a = 1.f) : red(r), green(g), blue(b), alpha(a) {}
    std::string css() const {
        char b[64]; std::snprintf(b, sizeof(b), "'rgba(%d,%d,%d,%.3f)'", (int)(red * 255), (int)(green * 255), (int)(blue * 255), alpha);
        return b;
    }
};

template <typename T> struct Point { T x, y; T getX() const { return x; } T getY() const { return y; } };
struct MouseEvent { uint button; bool press; Point<double> pos; uint mod; };
struct MotionEvent { Point<double> pos; uint mod; };
struct KeyboardEvent { bool press; uint key; uint mod; };
struct CharacterInputEvent { uint character; uint mod; };
struct ScrollEvent { Point<double> pos; Point<double> delta; uint mod; };

class UI {
public:
    enum Align { ALIGN_LEFT = 1, ALIGN_CENTER = 2, ALIGN_RIGHT = 4, ALIGN_TOP = 8, ALIGN_MIDDLE = 16, ALIGN_BOTTOM = 32, ALIGN_BASELINE = 64 };
    enum LineCap { BUTT, ROUND, SQUARE, BEVEL, MITER };
    enum Winding { CCW = 1, CW = 2 };
    struct Paint { std::string js; };

    std::string js;
    void* pluginPtr = nullptr;
    int w, h;
    UI(int width, int height) : w(width), h(height) {}
    virtual ~UI() {}
    bool loadSharedResources() { return true; }
    void setGeometryConstraints(uint, uint, bool) {}
    double getScaleFactor() const { return 1.0; }
    void setSize(uint, uint) {}
    void repaint() {}
    uint getWidth() const { return w; }
    uint getHeight() const { return h; }
    void setParameterValue(uint32_t, float) {}
    void editParameter(uint32_t, bool) {}
    void* getPluginInstancePointer() const { return pluginPtr; }

    virtual void parameterChanged(uint32_t, float) {}
    virtual void programLoaded(uint32_t) {}
    virtual void uiIdle() {}
    virtual void onNanoDisplay() = 0;
    virtual bool onMouse(const MouseEvent&) { return false; }
    virtual bool onMotion(const MotionEvent&) { return false; }
    virtual bool onScroll(const ScrollEvent&) { return false; }
    virtual bool onKeyboard(const KeyboardEvent&) { return false; }
    virtual bool onCharacterInput(const CharacterInputEvent&) { return false; }

    // ---- NanoVG subset ----
    char b[512];
    float fsize = 12; int align = ALIGN_LEFT | ALIGN_BASELINE; float blur = 0; float spacing = 0; std::string family = "DejaVu Sans";
    void fontFace(const char* f) { family = f; }
    void fontFaceId(int) {}
    void textLetterSpacing(float s) { spacing = s; }
    void scissor(float x, float y, float ww, float hh) { std::snprintf(b, sizeof(b), "c.beginPath();c.rect(%g,%g,%g,%g);c.clip();", x, y, ww, hh); emit(b); }
    void mockTexture(float x, float y, float ww, float hh) { std::snprintf(b, sizeof(b), "c.drawImage(TEX,%g,%g,%g,%g);", x, y, ww, hh); emit(b); }
    void mockImage(float x, float y, float ww, float hh) { std::snprintf(b, sizeof(b), "c.drawImage(LOGO,%g,%g,%g,%g);", x, y, ww, hh); emit(b); }
    void emit(const char* s) { js += s; js += "\n"; }
    void save() { emit("c.save();"); }
    void restore() { emit("c.restore();"); }
    void scale(float x, float y) { std::snprintf(b, sizeof(b), "c.scale(%g,%g);", x, y); emit(b); }
    void translate(float x, float y) { std::snprintf(b, sizeof(b), "c.translate(%g,%g);", x, y); emit(b); }
    void beginPath() { emit("c.beginPath();"); }
    void closePath() { emit("c.closePath();"); }
    void moveTo(float x, float y) { std::snprintf(b, sizeof(b), "c.moveTo(%g,%g);", x, y); emit(b); }
    void lineTo(float x, float y) { std::snprintf(b, sizeof(b), "c.lineTo(%g,%g);", x, y); emit(b); }
    void bezierTo(float a, float bb, float cc, float d, float e, float f) { std::snprintf(b, sizeof(b), "c.bezierCurveTo(%g,%g,%g,%g,%g,%g);", a, bb, cc, d, e, f); emit(b); }
    void quadTo(float a, float bb, float cc, float d) { std::snprintf(b, sizeof(b), "c.quadraticCurveTo(%g,%g,%g,%g);", a, bb, cc, d); emit(b); }
    void rect(float x, float y, float ww, float hh) { std::snprintf(b, sizeof(b), "c.rect(%g,%g,%g,%g);", x, y, ww, hh); emit(b); }
    void roundedRect(float x, float y, float ww, float hh, float r) { std::snprintf(b, sizeof(b), "c.roundRect(%g,%g,%g,%g,%g);", x, y, ww, hh, std::fmax(0.f, std::fmin(r, std::fmin(ww, hh) / 2))); emit(b); }
    void circle(float x, float y, float r) { std::snprintf(b, sizeof(b), "c.moveTo(%g,%g);c.arc(%g,%g,%g,0,6.2832);", x + r, y, x, y, r); emit(b); }
    void ellipse(float x, float y, float rx, float ry) { std::snprintf(b, sizeof(b), "c.ellipse(%g,%g,%g,%g,0,0,6.2832);", x, y, rx, ry); emit(b); }
    void arc(float x, float y, float r, float a0, float a1, Winding d) { std::snprintf(b, sizeof(b), "c.arc(%g,%g,%g,%g,%g,%s);", x, y, r, a0, a1, d == CCW ? "true" : "false"); emit(b); }
    void fillColor(const Color& col) { js += "c.fillStyle=" + col.css() + ";\n"; }
    void strokeColor(const Color& col) { js += "c.strokeStyle=" + col.css() + ";\n"; }
    void fillPaint(const Paint& p) { js += "c.fillStyle=" + p.js + ";\n"; }
    void strokePaint(const Paint& p) { js += "c.strokeStyle=" + p.js + ";\n"; }
    void strokeWidth(float wd) { std::snprintf(b, sizeof(b), "c.lineWidth=%g;", wd); emit(b); }
    void lineCap(LineCap) { emit("c.lineCap='round';"); }
    void lineJoin(LineCap) { emit("c.lineJoin='round';"); }
    void globalAlpha(float a) { std::snprintf(b, sizeof(b), "c.globalAlpha=%g;", a); emit(b); }
    void fill() { emit("c.fill();"); }
    void stroke() { emit("c.stroke();"); }
    Paint linearGradient(float sx, float sy, float ex, float ey, const Color& i, const Color& o) {
        std::snprintf(b, sizeof(b), "lg(%g,%g,%g,%g,%s,%s)", sx, sy, ex, ey, i.css().c_str(), o.css().c_str()); return { b };
    }
    Paint radialGradient(float cx, float cy, float inr, float outr, const Color& i, const Color& o) {
        std::snprintf(b, sizeof(b), "rg(%g,%g,%g,%g,%s,%s)", cx, cy, inr, outr, i.css().c_str(), o.css().c_str()); return { b };
    }
    Paint boxGradient(float x, float y, float ww, float hh, float r, float f, const Color& i, const Color& o) {
        std::snprintf(b, sizeof(b), "bg(%g,%g,%g,%g,%g,%g,%s,%s)", x, y, ww, hh, r, f, i.css().c_str(), o.css().c_str()); return { b };
    }
    void fontSize(float s) { fsize = s; }
    void fontBlur(float bl) { blur = bl; }
    void textAlign(int a) { align = a; }
    float text(float x, float y, const char* s, const char*) {
        std::string esc; for (const char* p = s; *p; ++p) { if (*p == '\'' || *p == '\\') esc += '\\'; esc += *p; }
        const char* ha = (align & ALIGN_CENTER) ? "center" : (align & ALIGN_RIGHT) ? "right" : "left";
        const char* va = (align & ALIGN_MIDDLE) ? "middle" : (align & ALIGN_TOP) ? "top" : (align & ALIGN_BOTTOM) ? "bottom" : "alphabetic";
        std::snprintf(b, sizeof(b), "tx(%g,%g,'%s',%g,'%s','%s',%g,'%s',%g);", x, y, esc.c_str(), fsize, ha, va, blur, family.c_str(), spacing); emit(b);
        return 0;
    }
};

} // namespace DISTRHO
