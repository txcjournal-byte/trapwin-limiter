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

    // vertical pill switch (black body, white knob)
    void pillV(float cx, float cy, bool up)
    {
        const float w = 40, h = 70;
        rrect(cx - w / 2 - 2, cy - h / 2 - 2, w + 4, h + 4, w / 2 + 2); fillColor(Color(255, 255, 255, 0.35f)); fill();
        rrect(cx - w / 2, cy - h / 2, w, h, w / 2);
        fillPaint(linearGradient(0, cy - h / 2, 0, cy + h / 2, Color(16, 17, 19), Color(44, 46, 50))); fill();
        const float ky = up ? cy - h / 2 + w / 2 : cy + h / 2 - w / 2;
        beginPath(); circle(cx, ky + 2, 15); fillColor(Color(0, 0, 0, 0.5f)); fill();
        beginPath(); circle(cx, ky, 14);
        fillPaint(radialGradient(cx - 4, ky - 5, 2, 16, Color(255, 255, 255), Color(190, 192, 196))); fill();
    }

    // horizontal pill switch (blue when on)
    void pillH(float cx, float cy, bool on)
    {
        const float w = 70, h = 36;
        rrect(cx - w / 2, cy - h / 2, w, h, h / 2);
        fillPaint(on ? linearGradient(0, cy - h / 2, 0, cy + h / 2, Color(40, 70, 140), Color(20, 40, 90))
                     : linearGradient(0, cy - h / 2, 0, cy + h / 2, Color(16, 17, 19), Color(44, 46, 50)));
        fill();
        const float kx = on ? cx + w / 2 - h / 2 : cx - w / 2 + h / 2;
        if (on) { beginPath(); circle(kx, cy, 22); fillPaint(radialGradient(kx, cy, 4, 22, Color(60, 140, 255, 0.6f), Color(60, 140, 255, 0.f))); fill(); }
        beginPath(); circle(kx, cy, 13);
        fillPaint(radialGradient(kx - 3, cy - 4, 1, 15, on ? Color(170, 210, 255) : Color(255, 255, 255), on ? Color(30, 110, 255) : Color(185, 188, 192))); fill();
    }

    void squareButton(float cx, float cy, bool on)
    {
        const float s = 46.f, x = cx - s / 2, y = cy - s / 2;
        rrect(x - 1, y + 2, s + 2, s + 1, 5); fillColor(Color(0, 0, 0, 0.35f)); fill();
        if (on) {
            rrect(x - 10, y - 10, s + 20, s + 20, 12);
            fillPaint(boxGradient(x, y, s, s, 5, 14, Color(50, 120, 255, 0.55f), Color(50, 120, 255, 0.f))); fill();
            rrect(x, y, s, s, 5); fillPaint(linearGradient(0, y, 0, y + s, Color(90, 160, 255), Color(30, 95, 245))); fill();
            rrect(x + 3, y + 2, s - 6, s * 0.4f, 4); fillColor(Color(255, 255, 255, 0.18f)); fill();
        } else {
            rrect(x, y, s, s, 5); fillPaint(linearGradient(0, y, 0, y + s, Color(40, 42, 46), Color(14, 15, 17))); fill();
            rrect(x + 3, y + 2, s - 6, s * 0.4f, 4); fillColor(Color(255, 255, 255, 0.06f)); fill();
        }
    }

    static float knobAngleB(float n) { return (135.f + 270.f * n) * PI / 180.f; }

    void betonKnob(float cx, float cy, float r, float n)
    {
        // shadow on concrete
        beginPath(); circle(cx + 4, cy + 7, r + 6); fillPaint(radialGradient(cx + 4, cy + 7, r * 0.7f, r + 10, Color(0, 0, 0, 0.45f), Color(0, 0, 0, 0.f))); fill();
        // ribbed black skirt
        beginPath(); circle(cx, cy, r); fillPaint(linearGradient(0, cy - r, 0, cy + r, Color(48, 50, 54), Color(10, 10, 12))); fill();
        const int ribs = r > 40 ? 64 : 32;
        strokeWidth(r > 40 ? 2.f : 1.4f);
        for (int i = 0; i < ribs; ++i) {
            const float a = i * 2 * PI / ribs;
            beginPath(); moveTo(cx + std::cos(a) * r * 0.80f, cy + std::sin(a) * r * 0.80f); lineTo(cx + std::cos(a) * (r - 1.5f), cy + std::sin(a) * (r - 1.5f));
            strokeColor(Color(110, 112, 118, 0.55f)); stroke();
        }
        // brushed metal cap
        const float rc = r * 0.74f;
        beginPath(); circle(cx, cy + 1.5f, rc + 1.5f); fillColor(Color(0, 0, 0, 0.6f)); fill();
        beginPath(); circle(cx, cy, rc);
        fillPaint(linearGradient(cx - rc, cy - rc, cx + rc, cy + rc, Color(214, 208, 192), Color(118, 112, 100))); fill();
        for (int i = 1; i < 9; ++i) {
            beginPath(); circle(cx, cy, rc * i / 9.f);
            strokeColor(i % 2 ? Color(255, 255, 255, 0.09f) : Color(0, 0, 0, 0.07f)); strokeWidth(1.f); stroke();
        }
        // conical highlight
        beginPath(); moveTo(cx, cy); arc(cx, cy, rc, PI * 1.15f, PI * 1.45f, CW); closePath();
        fillColor(Color(255, 255, 255, 0.22f)); fill();
        beginPath(); moveTo(cx, cy); arc(cx, cy, rc, PI * 0.15f, PI * 0.45f, CW); closePath();
        fillColor(Color(255, 255, 255, 0.12f)); fill();
        // pointer
        const float a = knobAngleB(n);
        strokeColor(Color(20, 20, 22)); strokeWidth(r > 40 ? 4.f : 2.6f); lineCap(ROUND);
        beginPath(); moveTo(cx + std::cos(a) * rc * 0.15f, cy + std::sin(a) * rc * 0.15f); lineTo(cx + std::cos(a) * rc * 0.95f, cy + std::sin(a) * rc * 0.95f); stroke();
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
        beginPath(); rect(px, py, pw, ph); fillColor(Color(150, 140, 120, 0.07f)); fill();
        beginPath(); rect(px, py, pw, ph); fillPaint(boxGradient(px, py, pw, ph, 4, 60, Color(0, 0, 0, 0.f), Color(0, 0, 0, 0.18f))); fill();
        // bevel
        beginPath(); rect(px, py, pw, 2); fillColor(Color(255, 255, 255, 0.45f)); fill();
        beginPath(); rect(px, py + ph - 2, pw, 2); fillColor(Color(0, 0, 0, 0.35f)); fill();
        beginPath(); rect(px - 3, py, 3, ph); fillColor(Color(0, 0, 0, 0.6f)); fill();
        beginPath(); rect(px + pw, py, 3, ph); fillColor(Color(0, 0, 0, 0.6f)); fill();
        railScrew(RAIL / 2, 62); railScrew(RAIL / 2, H - 62); railScrew(W - RAIL / 2, 62); railScrew(W - RAIL / 2, H - 62);
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
            label(cx, 264 + 1.5f, "TRAPWIN", 54, Color(255, 255, 255, 0.35f));
            fontLogo();
            fontSize(54); textLetterSpacing(2.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(Color(24, 24, 26)); text(cx, 264, "TRAPWIN", nullptr); textLetterSpacing(0.f);
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

