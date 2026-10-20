#include "PluginEditor.h"
#include "BitEraData.h"

namespace bitera
{
using namespace juce;

//==============================================================================
// resources
namespace res
{
const char* find (const char* originalName, int& size)
{
    for (int i = 0; i < BitEraData::namedResourceListSize; ++i)
    {
        const char* n = BitEraData::namedResourceList[i];
        if (String (BitEraData::getNamedResourceOriginalFilename (n)) == originalName)
            return BitEraData::getNamedResource (n, size);
    }
    size = 0;
    return nullptr;
}

Image image (const char* name)
{
    int size = 0;
    if (auto* d = find (name, size))
        return ImageCache::getFromMemory (d, size);
    return {};
}

Typeface::Ptr typeface (const char* name)
{
    int size = 0;
    if (auto* d = find (name, size))
        return Typeface::createSystemTypefaceFor (d, (size_t) size);
    return {};
}
} // namespace res

struct Fonts
{
    Typeface::Ptr elite = res::typeface ("SpecialElite-Regular.ttf");
    Typeface::Ptr marker = res::typeface ("PermanentMarker-Regular.ttf");
    Typeface::Ptr archivo = res::typeface ("ArchivoBlack-Regular.ttf");
    Typeface::Ptr barlow = res::typeface ("BarlowCondensed-Bold.ttf");
    Typeface::Ptr courier = res::typeface ("CourierPrime-Regular.ttf");
    Typeface::Ptr courierBold = res::typeface ("CourierPrime-Bold.ttf");

    static Font from (const Typeface::Ptr& t, float h) { return t != nullptr ? Font (FontOptions (t).withHeight (h)) : Font (FontOptions (h, Font::bold)); }
    static Font sys (const char* name, float h, int style) { return Font (FontOptions (name, h, style)); }

    // display face of each era (titles, logo, preset display)
    Font display (int eraIndex, float h) const
    {
        switch (eraIndex)
        {
            case 0:  return from (elite, h);
            case 1:  return sys ("Trebuchet MS", h, Font::bold);
            case 2:  return from (archivo, h);
            case 3:  return from (marker, h);
            case 4:  return from (barlow, h).withExtraKerningFactor (0.08f);
            case 5:  return from (courierBold, h);
            case 6:  return from (archivo, h).italicised();
            case 7:  return sys ("Trebuchet MS", h, Font::bold);
            case 8:  return sys ("Verdana", h, Font::bold | Font::italic);
            case 9:  return sys ("Arial", h, Font::plain);
            case 10: return from (barlow, h);
            default: return from (barlow, h).italicised();
        }
    }
    // small text (names, labels, values)
    Font body (int eraIndex, float h, bool bold = true) const
    {
        switch (eraIndex)
        {
            case 0: case 2: case 5: case 11: return from (bold ? courierBold : courier, h);
            default: return sys ("Arial", h, bold ? Font::bold : Font::plain);
        }
    }
};

//==============================================================================
// drawing helpers
void drawCover (Graphics& g, const Image& img, Rectangle<float> r, float focusY = 0.3f)
{
    g.setOpacity (1.0f);
    if (! img.isValid())
    {
        g.setColour (Colours::black);
        g.fillRect (r);
        return;
    }
    const float scale = jmax (r.getWidth() / (float) img.getWidth(), r.getHeight() / (float) img.getHeight());
    const float sw = r.getWidth() / scale, sh = r.getHeight() / scale;
    const float sx = ((float) img.getWidth() - sw) * 0.5f;
    const float sy = ((float) img.getHeight() - sh) * focusY;
    g.drawImage (img, (int) r.getX(), (int) r.getY(), (int) r.getWidth(), (int) r.getHeight(),
                 (int) sx, (int) sy, (int) sw, (int) sh);
}

Path chamferRect (Rectangle<float> r, float c)
{
    Path p;
    p.startNewSubPath (r.getX() + c, r.getY());
    p.lineTo (r.getRight(), r.getY());
    p.lineTo (r.getRight(), r.getBottom() - c);
    p.lineTo (r.getRight() - c, r.getBottom());
    p.lineTo (r.getX(), r.getBottom());
    p.lineTo (r.getX(), r.getY() + c);
    p.closeSubPath();
    return p;
}

Path panelShape (const Theme& t, Rectangle<float> r)
{
    if (t.frame == FrameStyle::chamfer)
        return chamferRect (r, 12.0f);
    Path p;
    p.addRoundedRectangle (r, jmin (t.corner, r.getHeight() * 0.5f));
    return p;
}

//==============================================================================
class KnobLookAndFeel : public LookAndFeel_V4
{
public:
    const Theme* theme = nullptr;

    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float startA, float endA, Slider&) override
    {
        if (theme == nullptr) return;
        const auto& t = *theme;
        auto r = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
        const float size = jmin (r.getWidth(), r.getHeight());
        r = r.withSizeKeepingCentre (size, size);
        const auto c = r.getCentre();
        const float radius = size * 0.5f;
        const float angle = startA + pos * (endA - startA);

        // value ring
        Path track, value;
        track.addCentredArc (c.x, c.y, radius - 2, radius - 2, 0, startA, endA, true);
        value.addCentredArc (c.x, c.y, radius - 2, radius - 2, 0, startA, angle, true);
        g.setColour (t.ink.withAlpha (0.15f));
        g.strokePath (track, PathStrokeType (3.0f));
        if (t.knob == KnobStyle::neon)
        {
            g.setColour (t.accent.withAlpha (0.25f));
            g.strokePath (value, PathStrokeType (8.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (t.accent == t.ink ? t.accent2.darker (0.6f) : t.accent);
        if (t.knob == KnobStyle::flat && t.accent == Colour (0xff111111)) g.setColour (Colour (0xffff6a1f));
        g.strokePath (value, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

        auto body = r.reduced (radius * 0.2f);
        auto cap = r.reduced (radius * 0.4f);
        const bool square = t.knob == KnobStyle::pixel;
        const bool rounded = t.knob == KnobStyle::chamfer;

        auto fillShape = [&] (Rectangle<float> rr, const FillType& f)
        {
            g.setFillType (f);
            if (square) g.fillRect (rr);
            else if (rounded) g.fillRoundedRectangle (rr, rr.getWidth() * 0.22f);
            else g.fillEllipse (rr);
        };

        // drop shadow
        g.setColour (Colours::black.withAlpha (square ? 1.0f : 0.35f));
        if (square) g.fillRect (body.translated (4, 4));
        else g.fillEllipse (body.translated (0, 3));

        switch (t.knob)
        {
            case KnobStyle::metal:
            case KnobStyle::brass:
            case KnobStyle::bakelite:
            {
                // knurled skirt
                fillShape (body, FillType (Colour (t.knob == KnobStyle::brass ? 0xff2a1a0c : 0xff1c1d21)));
                g.setColour (Colour (t.knob == KnobStyle::brass ? 0xff5a3a18 : 0xff4a4b52));
                for (int i = 0; i < 36; ++i)
                {
                    const float a = MathConstants<float>::twoPi * (float) i / 36.0f;
                    const float r1 = body.getWidth() * 0.5f, r0 = r1 * 0.78f;
                    g.drawLine (c.x + std::sin (a) * r0, c.y - std::cos (a) * r0, c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, 1.2f);
                }
                ColourGradient cg;
                if (t.knob == KnobStyle::brass) cg = ColourGradient (Colour (0xfff6dc8f), cap.getX() + cap.getWidth() * 0.3f, cap.getY(), Colour (0xff6d4a12), cap.getRight(), cap.getBottom(), true);
                else if (t.knob == KnobStyle::bakelite) cg = ColourGradient (Colour (0xffe8e8e8), cap.getX() + cap.getWidth() * 0.3f, cap.getY(), Colour (0xff3a3a3a), cap.getRight(), cap.getBottom(), true);
                else cg = ColourGradient (Colour (0xfff1f2f5), cap.getX() + cap.getWidth() * 0.3f, cap.getY(), Colour (0xff3a3c42), cap.getRight(), cap.getBottom(), true);
                fillShape (cap, FillType (cg));
                break;
            }
            case KnobStyle::soft:
                fillShape (body, FillType (t.panelEdge.isTransparent() ? t.accent2 : t.accent2.withAlpha (0.35f).overlaidWith (t.panel)));
                fillShape (cap, FillType (ColourGradient (Colours::white, cap.getX(), cap.getY(), t.panel.darker (0.08f), cap.getRight(), cap.getBottom(), false)));
                break;
            case KnobStyle::chamfer:
                fillShape (body, FillType (ColourGradient (Colour (0xff5a6aa8), body.getX(), body.getY(), Colour (0xff1d2a60), body.getRight(), body.getBottom(), false)));
                fillShape (cap, FillType (ColourGradient (Colour (0xffe6ebff), cap.getX(), cap.getY(), Colour (0xff3d4a7a), cap.getRight(), cap.getBottom(), false)));
                break;
            case KnobStyle::pixel:
                fillShape (body, FillType (Colour (0xff141414)));
                fillShape (cap, FillType (t.accent));
                g.setColour (t.accent.brighter (0.5f));
                g.fillRect (cap.withHeight (4.0f));
                g.fillRect (cap.withWidth (4.0f));
                g.setColour (t.accent.darker (0.5f));
                g.fillRect (cap.withTrimmedTop (cap.getHeight() - 4.0f));
                g.fillRect (cap.withTrimmedLeft (cap.getWidth() - 4.0f));
                break;
            case KnobStyle::neon:
                fillShape (body, FillType (Colour (0xff12001f)));
                g.setColour (t.accent2.withAlpha (0.8f));
                g.drawEllipse (body, 1.5f);
                fillShape (cap, FillType (Colour (0xff2a1040)));
                break;
            case KnobStyle::flat:
                fillShape (body, FillType (t.ink.withAlpha (t.bgTop.getPerceivedBrightness() > 0.8f ? 0.08f : 1.0f)));
                fillShape (cap, FillType (t.bgTop.getPerceivedBrightness() > 0.8f ? Colours::white : t.bgTop));
                break;
            case KnobStyle::gloss:
                fillShape (body, FillType (ColourGradient (Colour (0xffc9d6dc), body.getX(), body.getY(), Colour (0xff7f97a3), body.getX(), body.getBottom(), false)));
                fillShape (cap, FillType (ColourGradient (Colours::white, cap.getX() + cap.getWidth() * 0.35f, cap.getY(), Colour (0xff6b8592), cap.getRight(), cap.getBottom(), true)));
                g.setColour (Colours::white.withAlpha (0.6f));
                g.fillEllipse (cap.reduced (cap.getWidth() * 0.25f).withHeight (cap.getHeight() * 0.22f).translated (0, -cap.getHeight() * 0.12f));
                break;
        }

        // pointer
        Path ptr;
        const float pr = cap.getWidth() * 0.5f;
        ptr.addRoundedRectangle (-1.5f, -pr + 2.0f, 3.0f, pr * 0.55f, 1.0f);
        Colour pc = t.ink;
        if (t.knob == KnobStyle::neon) pc = t.accent2;
        else if (t.knob == KnobStyle::pixel) pc = Colour (0xfff8f8f8);
        else if (t.knob == KnobStyle::soft || t.knob == KnobStyle::chamfer) pc = t.accent;
        else if (t.knob == KnobStyle::metal || t.knob == KnobStyle::bakelite) pc = Colour (0xff111111);
        else if (t.knob == KnobStyle::brass) pc = Colour (0xff2a1d10);
        else if (t.knob == KnobStyle::gloss) pc = t.accent;
        g.setColour (pc);
        g.fillPath (ptr, AffineTransform::rotation (angle).translated (c.x, c.y));
    }
};

//==============================================================================
class Content : public Component, private Timer, private AudioProcessorValueTreeState::Listener
{
public:
    explicit Content (BitEraProcessor& p) : proc (p)
    {
        for (int i = 0; i < numEras; ++i)
        {
            covers[i] = res::image (era (i).cover);
            thumbs[i] = covers[i].isValid() ? covers[i].rescaled (120, 180, Graphics::mediumResamplingQuality) : Image();
        }
        crtCover = covers[2].createCopy();
        crtCover.desaturate();

        const char* ids[6] = { "volume", "attack", "decay", "release", "tone", "space" };
        const char* names[6] = { "VOLUME", "ATTACK", "DECAY", "RELEASE", "TONE", "SPACE" };
        for (int i = 0; i < 6; ++i)
        {
            auto& k = knobs[i];
            k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
            k.slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
            k.slider.setLookAndFeel (&knobLnf);
            k.slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
            k.slider.setPopupDisplayEnabled (true, true, this);
            k.name = names[i];
            k.id = ids[i];
            addAndMakeVisible (k.slider);
            k.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.state, ids[i], k.slider);
            k.slider.onValueChange = [this] { repaint (knobPanel.toNearestInt()); };
        }

        proc.state.addParameterListener ("preset", this);
        showIntro = ! proc.introSeen();
        layout();
        updateTheme();
        startTimerHz (20);
    }

    ~Content() override
    {
        proc.state.removeParameterListener ("preset", this);
        for (auto& k : knobs)
            k.slider.setLookAndFeel (nullptr);
    }

    //==========================================================================
    void paint (Graphics& g) override
    {
        const int e = currentEra();
        const auto& E = era (e);
        const auto& t = E.theme;

        paintBackground (g, e, t);
        paintHeader (g, e, t);
        paintCover (g, e, t);
        paintTitle (g, e, t);
        paintRows (g, e, t);
        paintKnobPanel (g, e, t);
        paintStrip (g, e);
        if (showIntro)
            paintIntro (g);
    }

    //==========================================================================
    void mouseMove (const MouseEvent& ev) override
    {
        const auto hit = hitTestArea (ev.position);
        setMouseCursor (hit.kind == Hit::none ? MouseCursor::NormalCursor
                        : (hit.kind == Hit::drag || hit.kind == Hit::rowDrag) ? MouseCursor::DraggingHandCursor
                        : MouseCursor::PointingHandCursor);
    }

    void mouseDown (const MouseEvent& ev) override
    {
        dragStarted = false;
        downHit = hitTestArea (ev.position);
    }

    void mouseDrag (const MouseEvent& ev) override
    {
        if (dragStarted || ev.getDistanceFromDragStart() < 6)
            return;
        if (downHit.kind == Hit::drag)
            startWavDrag (proc.getPreset());
        else if (downHit.kind == Hit::rowDrag || downHit.kind == Hit::row || downHit.kind == Hit::rowPlay)
            startWavDrag (currentEra() * soundsPerEra + downHit.index);
    }

    void mouseUp (const MouseEvent& ev) override
    {
        if (dragStarted || ev.getDistanceFromDragStart() > 5)
            return;
        const auto hit = hitTestArea (ev.position);
        if (hit.kind != downHit.kind || hit.index != downHit.index)
            return;

        switch (hit.kind)
        {
            case Hit::start:
                showIntro = false;
                proc.setIntroSeen();
                updateTheme();
                break;
            case Hit::era:
                proc.stopPreview();
                playing = -1;
                proc.setPreset (hit.index * soundsPerEra);
                break;
            case Hit::prev: stepPreset (-1); break;
            case Hit::next: stepPreset (1); break;
            case Hit::rowPlay:
            case Hit::row:
            {
                const int preset = currentEra() * soundsPerEra + hit.index;
                if (playing == preset && hit.kind == Hit::rowPlay)
                {
                    proc.stopPreview();
                    playing = -1;
                }
                else
                {
                    proc.previewPreset (preset);
                    playing = preset;
                    playEndMs = Time::getMillisecondCounterHiRes() + proc.getSoundLengthSeconds (preset) * 1000.0 + 150.0;
                }
                repaint();
                break;
            }
            case Hit::drag:
            case Hit::logo:
                if (hit.kind == Hit::logo) { showIntro = true; updateTheme(); }
                break;
            case Hit::rowDrag:
            case Hit::none:
                break;
        }
    }

private:
    //==========================================================================
    struct Hit
    {
        enum Kind { none, start, era, prev, next, row, rowPlay, rowDrag, drag, logo } kind = none;
        int index = -1;
    };

    struct Knob
    {
        Slider slider;
        std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attachment;
        String name, id;
    };

    BitEraProcessor& proc;
    Fonts fonts;
    KnobLookAndFeel knobLnf;
    Knob knobs[6];
    Image covers[numEras], thumbs[numEras], crtCover;

    Rectangle<float> logoArea { 20, 8, 230, 50 }, display { 330, 12, 440, 42 }, dragBtn { 870, 13, 210, 40 };
    Rectangle<float> coverArea { 22, 86, 282, 416 }, titleArea { 332, 84, 748, 146 }, knobPanel { 332, 392, 640, 112 };
    Rectangle<float> rows[soundsPerEra], strip { 20, 520, 1060, 168 };
    Rectangle<float> startBtn { 430, 446, 240, 60 };

    bool showIntro = false, dragStarted = false;
    Hit downHit;
    int playing = -1;
    double playEndMs = 0;

    int currentEra() const { return eraOfPreset (proc.getPreset()); }

    void layout()
    {
        for (int i = 0; i < soundsPerEra; ++i)
            rows[i] = { 332.0f + (float) (i / 3) * 326.0f, 232.0f + (float) (i % 3) * 52.0f, 314.0f, 46.0f };   // left: one-shots, right: phrases / loops / FX
        const float kw = (knobPanel.getWidth() - 28.0f) / 6.0f;
        for (int i = 0; i < 6; ++i)
            knobs[i].slider.setBounds (Rectangle<float> (knobPanel.getX() + 14.0f + (float) i * kw, knobPanel.getY() + 24.0f, kw, 58.0f)
                                           .withSizeKeepingCentre (58.0f, 58.0f).toNearestInt());
    }

    void updateTheme()
    {
        knobLnf.theme = &era (currentEra()).theme;
        for (auto& k : knobs)
            k.slider.setVisible (! showIntro);
        repaint();
    }

    void parameterChanged (const String&, float) override
    {
        MessageManager::callAsync ([safe = SafePointer<Content> (this)] { if (safe != nullptr) safe->updateTheme(); });
    }

    void timerCallback() override
    {
        for (auto& k : knobs)
            if (k.slider.isVisible() == showIntro)
                k.slider.setVisible (! showIntro);
        if (playing >= 0 && Time::getMillisecondCounterHiRes() > playEndMs)
        {
            playing = -1;
            repaint();
        }
    }

    void stepPreset (int d)
    {
        const int e = currentEra();
        const int idx = (proc.getPreset() - e * soundsPerEra + d + soundsPerEra) % soundsPerEra;
        proc.setPreset (e * soundsPerEra + idx);
    }

    Hit hitTestArea (Point<float> p) const
    {
        if (showIntro)
            return startBtn.contains (p) ? Hit { Hit::start, 0 } : Hit {};
        if (logoArea.contains (p)) return { Hit::logo, 0 };
        if (display.contains (p)) return { p.x < display.getCentreX() ? Hit::prev : Hit::next, 0 };
        if (dragBtn.contains (p)) return { Hit::drag, 0 };
        for (int i = 0; i < soundsPerEra; ++i)
            if (rows[i].contains (p))
            {
                if (p.x < rows[i].getX() + 52.0f) return { Hit::rowPlay, i };
                if (p.x > rows[i].getRight() - 90.0f) return { Hit::rowDrag, i };
                return { Hit::row, i };
            }
        if (strip.contains (p))
        {
            const float w = (strip.getWidth() - 5.0f * 11.0f) / 12.0f;
            const int i = (int) ((p.x - strip.getX()) / (w + 5.0f));
            if (i >= 0 && i < numEras) return { Hit::era, i };
        }
        return {};
    }

    void startWavDrag (int preset)
    {
        MemoryBlock mb;
        if (! proc.getSoundWav (preset, mb))
            return;
        auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("Bit Era");
        dir.createDirectory();
        const auto& E = era (eraOfPreset (preset));
        auto f = dir.getChildFile (File::createLegalFileName (String ("Bit Era ") + E.year + " " + sound (preset).name + ".wav"));
        if (! f.replaceWithData (mb.getData(), mb.getSize()))
            return;
        dragStarted = true;
        DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }

    //==========================================================================
    void paintBackground (Graphics& g, int e, const Theme& t)
    {
        auto b = getLocalBounds().toFloat();
        g.setGradientFill (ColourGradient (t.bgTop, 0, 0, t.bgBottom, 0, b.getHeight(), false));
        g.fillAll();

        if (t.darkCover)
        {
            g.saveState();
            g.beginTransparencyLayer (0.32f);
            drawCover (g, covers[e], { 260, -40, 900, 780 }, 0.25f);
            g.endTransparencyLayer();
            g.restoreState();
            g.setGradientFill (ColourGradient (t.bgTop, 0, 0, t.bgTop.withAlpha (0.15f), b.getWidth(), 0, false));
            g.fillAll();
            g.setGradientFill (ColourGradient (t.bgBottom.withAlpha (0.0f), 0, 440, t.bgBottom, 0, 700, false));
            g.fillAll();
        }
        if (e == 0)   // projector vignette + film scratches
        {
            g.setGradientFill (ColourGradient (Colours::transparentBlack, 550, 330, Colour (0x9a3c280f), 0, 0, true));
            g.fillAll();
            g.setColour (t.ink.withAlpha (0.1f));
            for (float x : { 140.0f, 474.0f, 811.0f, 1012.0f })
                g.drawVerticalLine ((int) x, 0, b.getHeight());
        }
        if (e == 1)   // watercolor blots
        {
            g.setColour (t.accent.withAlpha (0.16f)); g.fillEllipse (-120, -80, 420, 360);
            g.setColour (t.accent2.withAlpha (0.16f)); g.fillEllipse (760, 380, 460, 380);
            g.setColour (Colour (0x55f0c86e)); g.fillEllipse (650, -120, 360, 300);
        }
        if (t.stars)
        {
            Random rnd (1979);
            for (int i = 0; i < 160; ++i)
            {
                const float x = rnd.nextFloat() * b.getWidth(), y = rnd.nextFloat() * b.getHeight(), s = 0.8f + rnd.nextFloat() * 1.6f;
                g.setColour ((i % 7 == 0 ? t.accent2 : Colours::white).withAlpha (0.25f + rnd.nextFloat() * 0.6f));
                g.fillEllipse (x, y, s, s);
            }
        }
        if (t.grid)
        {
            g.setColour (Colours::black.withAlpha (0.05f));
            for (int x = 0; x < (int) b.getWidth(); x += 8) g.drawVerticalLine (x, 0, b.getHeight());
            for (int y = 0; y < (int) b.getHeight(); y += 8) g.drawHorizontalLine (y, 0, b.getWidth());
        }
        if (e == 6)   // OVA neon floor glow
        {
            g.setGradientFill (ColourGradient (Colours::transparentBlack, 0, 420, t.accent2.withAlpha (0.12f), 0, 700, false));
            g.fillAll();
        }
        if (t.scanlines)
        {
            g.setColour (Colours::black.withAlpha (e == 2 ? 0.07f : 0.16f));
            for (int y = 0; y < (int) b.getHeight(); y += 3) g.drawHorizontalLine (y, 0, b.getWidth());
        }
    }

    void paintHeader (Graphics& g, int e, const Theme& t)
    {
        Rectangle<float> h (0, 0, 1100, 66);
        g.setColour (t.header);
        g.fillRect (h);
        switch (e)
        {
            case 0: g.setColour (t.ink); g.fillRect (0.0f, 62.0f, 1100.0f, 1.5f); g.fillRect (0.0f, 65.0f, 1100.0f, 1.5f); break;
            case 3: g.setColour (Colour (0xffa8431b)); g.fillRect (0.0f, 60.0f, 1100.0f, 3.0f); g.setColour (t.accent); g.fillRect (0.0f, 63.0f, 1100.0f, 3.0f); g.setColour (t.accent2); g.fillRect (0.0f, 66.0f, 1100.0f, 3.0f); break;
            case 4: g.setColour (t.accent2); g.fillRect (0.0f, 63.0f, 1100.0f, 3.0f); g.setColour (t.accent); g.fillRect (0.0f, 66.0f, 1100.0f, 3.0f); break;
            case 5: g.setColour (Colour (0xff141414)); g.fillRect (0.0f, 60.0f, 1100.0f, 6.0f); break;
            case 6: g.setColour (t.accent2.withAlpha (0.25f)); g.fillRect (0.0f, 62.0f, 1100.0f, 8.0f); g.setColour (t.accent2); g.fillRect (0.0f, 64.0f, 1100.0f, 2.0f); break;
            case 8: case 9: g.setColour (t.panelEdge); g.fillRect (0.0f, 65.0f, 1100.0f, 1.0f); break;
            case 11: g.setColour (t.accent.withAlpha (0.3f)); g.fillRect (0.0f, 62.0f, 1100.0f, 8.0f); g.setColour (t.accent); g.fillRect (0.0f, 64.0f, 1100.0f, 2.0f); break;
            default: break;
        }

        // logo
        String logo = (e == 9) ? "bit era" : (e == 0 || e == 1 || e == 3 || e == 7) ? "Bit Era" : "BIT ERA";
        if (e == 0) logo = String (CharPointer_UTF8 ("\xe2\x9d\xa6 ")) + logo;
        g.setFont (fonts.display (e, e == 5 ? 22.0f : 32.0f));
        if (e == 11 || e == 8)
            g.setGradientFill (ColourGradient (Colours::white, 0, 12, e == 8 ? Colour (0xff00708a) : Colour (0xff6c7488), 0, 44, false));
        else if (e == 6)
            g.setColour (t.accent);
        else
            g.setColour (t.headerInk);
        g.drawText (logo, logoArea.withHeight (36), Justification::bottomLeft);
        g.setFont (fonts.body (e, 11.0f));
        g.setColour (t.headerInk.withAlpha (0.85f));
        if (e == 6) g.setColour (t.muted);
        g.drawText (String::fromUTF8 ("BY TRAPWIN  \xc2\xb7  ") + era (e).ep, logoArea.withTrimmedTop (38), Justification::topLeft);

        // preset display
        auto d = display;
        if (t.frame == FrameStyle::chamfer)
        {
            Path hex;
            hex.startNewSubPath (d.getX() + 12, d.getY()); hex.lineTo (d.getRight() - 12, d.getY()); hex.lineTo (d.getRight(), d.getCentreY());
            hex.lineTo (d.getRight() - 12, d.getBottom()); hex.lineTo (d.getX() + 12, d.getBottom()); hex.lineTo (d.getX(), d.getCentreY()); hex.closeSubPath();
            g.setColour (t.lcd); g.fillPath (hex);
        }
        else
        {
            const float rad = t.frame == FrameStyle::pixel || t.frame == FrameStyle::flat ? 0.0f : jmin (t.corner + 4.0f, 21.0f);
            if (t.frame == FrameStyle::pixel) { g.setColour (Colours::black); g.fillRect (d.translated (4, 4)); }
            g.setColour (t.lcd);
            g.fillRoundedRectangle (d, rad);
            if (t.frame == FrameStyle::pixel) { g.setColour (Colour (0xff8a8a8a)); g.drawRect (d, 3.0f); }
            if (t.frame == FrameStyle::neon) { g.setColour (t.accent2); g.drawRoundedRectangle (d, rad, 1.2f); }
            if (t.frame == FrameStyle::film) { g.setColour (t.ink); g.drawRect (d.expanded (4), 1.0f); }
        }
        const int preset = proc.getPreset();
        g.setColour (t.lcdInk);
        g.setFont (fonts.body (e, 15.0f));
        g.drawText ("<", d.withWidth (34), Justification::centred);
        g.drawText (">", d.withTrimmedLeft (d.getWidth() - 34), Justification::centred);
        g.setFont (fonts.body (e, 11.5f));
        g.setColour (t.lcdInk.withAlpha (0.85f));
        g.drawText (String (era (e).year) + String::fromUTF8 ("  \xc2\xb7  ") + sound (preset).category, d.withHeight (18).translated (0, 4), Justification::centred);
        g.setColour (t.lcdInk);
        g.setFont (fonts.display (e, e == 5 ? 13.0f : 19.0f));
        g.drawText (sound (preset).name, d.withTrimmedTop (16).reduced (36, 0), Justification::centred);

        // DRAG WAV
        auto b = dragBtn;
        if (t.frame == FrameStyle::pixel) { g.setColour (Colour (0xff141414)); g.fillRect (b.translated (4, 4)); }
        g.setColour (t.dragBg);
        if (e == 11)
        {
            Path p; p.startNewSubPath (b.getX(), b.getY()); p.lineTo (b.getRight(), b.getY()); p.lineTo (b.getRight(), b.getY() + b.getHeight() * 0.68f);
            p.lineTo (b.getRight() - 18, b.getBottom()); p.lineTo (b.getX(), b.getBottom()); p.closeSubPath();
            g.fillPath (p);
        }
        else
            g.fillRoundedRectangle (b, t.frame == FrameStyle::pixel || t.frame == FrameStyle::flat ? 0.0f : jmin (t.corner, 20.0f));
        if (t.frame == FrameStyle::neon) { g.setColour (t.accent); g.drawRoundedRectangle (b, 4.0f, 1.2f); }
        if (e == 0 || e == 2) { g.setColour (e == 0 ? t.ink : Colour (0xfff2f2ee)); g.drawRoundedRectangle (b, e == 2 ? 6.0f : 0.0f, 2.0f); }
        g.setColour (t.dragInk);
        g.setFont (fonts.display (e, e == 5 ? 12.0f : 19.0f));
        g.drawText (String (CharPointer_UTF8 ("DRAG WAV \xe2\x86\x93")), b, Justification::centred);
    }

    void paintCover (Graphics& g, int e, const Theme& t)
    {
        auto r = coverArea;
        const auto& img = e == 2 ? crtCover : covers[e];
        Path clip;

        switch (t.frame)
        {
            case FrameStyle::film:
            {
                g.setColour (t.ink.withAlpha (0.35f)); g.fillRect (r.translated (6, 8));
                g.setColour (Colour (0xff1a120a)); g.fillRect (r);
                auto in = r.reduced (18, 10);
                drawCover (g, img, in);
                g.setColour (t.bgTop);
                for (float y = r.getY() + 8; y < r.getBottom() - 10; y += 22)
                {
                    g.fillRoundedRectangle (r.getX() + 5, y, 8, 12, 2);
                    g.fillRoundedRectangle (r.getRight() - 13, y, 8, 12, 2);
                }
                break;
            }
            case FrameStyle::photo:
            case FrameStyle::polaroid:
            {
                const bool pol = t.frame == FrameStyle::polaroid;
                DropShadow (t.ink.withAlpha (0.3f), 22, { 0, 10 }).drawForRectangle (g, r.toNearestInt());
                g.setColour (Colour (0xfffffaf0));
                g.fillRoundedRectangle (r, pol ? 2.0f : 20.0f);
                auto in = pol ? r.withTrimmedLeft (12).withTrimmedRight (12).withTrimmedTop (12).withTrimmedBottom (40) : r.reduced (9);
                clip.addRoundedRectangle (in, pol ? 0.0f : 13.0f);
                g.saveState(); g.reduceClipRegion (clip); drawCover (g, img, in); g.restoreState();
                if (pol) { g.setColour (t.muted); g.setFont (fonts.display (e, 16.0f)); g.drawText (String (era (e).year) + String::fromUTF8 (" \xc2\xb7 ") + era (e).shortName, r.withTrimmedTop (r.getHeight() - 38), Justification::centred); }
                break;
            }
            case FrameStyle::crt:
            {
                DropShadow (Colours::black.withAlpha (0.4f), 20, { 0, 10 }).drawForRectangle (g, r.toNearestInt());
                g.setColour (Colour (0xff888888)); g.fillRoundedRectangle (r.expanded (4), 38);
                g.setColour (Colour (0xff1a1a1a)); g.fillRoundedRectangle (r, 34);
                auto in = r.reduced (16);
                clip.addRoundedRectangle (in, 26);
                g.saveState(); g.reduceClipRegion (clip); drawCover (g, img, in);
                g.setGradientFill (ColourGradient (Colours::transparentBlack, in.getCentreX(), in.getCentreY(), Colours::black.withAlpha (0.55f), in.getX(), in.getY(), true));
                g.fillRect (in);
                g.restoreState();
                break;
            }
            case FrameStyle::retro:
            {
                g.setColour (t.accent2); g.fillRoundedRectangle (r.translated (20, 20), 28);
                g.setColour (t.accent); g.fillRoundedRectangle (r.translated (10, 10), 28);
                g.setColour (t.ink); g.fillRoundedRectangle (r, 28);
                auto in = r.reduced (7);
                clip.addRoundedRectangle (in, 22);
                g.saveState(); g.reduceClipRegion (clip); drawCover (g, img, in); g.restoreState();
                break;
            }
            case FrameStyle::chamfer:
            {
                auto outer = chamferRect (r, 24);
                g.setColour (t.accent.withAlpha (0.35f)); g.strokePath (outer, PathStrokeType (10.0f));
                g.saveState(); g.reduceClipRegion (outer); drawCover (g, img, r); g.restoreState();
                g.setColour (t.accent2); g.strokePath (outer, PathStrokeType (3.0f));
                break;
            }
            case FrameStyle::pixel:
                g.setColour (Colour (0xff141414)); g.fillRect (r.translated (8, 8)); g.fillRect (r);
                drawCover (g, img, r.reduced (6));
                break;
            case FrameStyle::neon:
            case FrameStyle::trap:
            {
                if (t.frame == FrameStyle::trap) { g.setColour (Colours::black); g.fillRect (r.translated (8, 8)); }
                for (int i = 6; i > 0; --i) { g.setColour (t.accent.withAlpha (0.06f)); g.drawRect (r.expanded ((float) i * 3), 3.0f); }
                drawCover (g, img, r);
                g.setColour (t.accent); g.drawRect (r, 2.0f);
                break;
            }
            case FrameStyle::gloss:
            case FrameStyle::card:
            {
                DropShadow (t.ink.withAlpha (0.2f), 26, { 0, 14 }).drawForRectangle (g, r.toNearestInt());
                if (t.frame == FrameStyle::gloss) { g.setColour (Colours::white.withAlpha (0.75f)); g.fillRoundedRectangle (r.expanded (6), 20); }
                clip.addRoundedRectangle (r, t.corner + 2);
                g.saveState(); g.reduceClipRegion (clip); drawCover (g, img, r); g.restoreState();
                if (t.frame == FrameStyle::gloss) { g.setColour (t.panelEdge); g.drawRoundedRectangle (r, t.corner + 2, 1.0f); }
                break;
            }
            case FrameStyle::flat:
                g.setColour (t.ink); g.fillRect (r.translated (14, 14));
                drawCover (g, img, r);
                break;
        }

        // era badge
        auto badge = Rectangle<float> (r.getX() + 12, r.getY() + 12, 0, 22);
        const String txt = String (era (e).ep) + String::fromUTF8 ("  \xc2\xb7  ") + era (e).year;
        auto f = fonts.body (e, 12.0f);
        badge.setWidth (GlyphArrangement::getStringWidth (f, txt) + 16);
        if (t.frame == FrameStyle::film) badge.translate (8, 4);
        g.setColour (t.frame == FrameStyle::trap || t.frame == FrameStyle::neon ? Colours::black : t.lcd);
        g.fillRoundedRectangle (badge, t.frame == FrameStyle::pixel || t.frame == FrameStyle::flat ? 0.0f : 4.0f);
        g.setColour (t.frame == FrameStyle::trap ? t.accent : t.frame == FrameStyle::neon ? t.accent2 : t.lcdInk);
        g.setFont (f);
        g.drawText (txt, badge, Justification::centred);
    }

    void paintTitle (Graphics& g, int e, const Theme& t)
    {
        const auto& E = era (e);
        String title (E.title);
        if (t.upperTitle) title = title.toUpperCase();
        const float size = e == 5 ? 34.0f : e == 10 ? 70.0f : e == 4 ? 60.0f : 56.0f;
        g.setFont (fonts.display (e, size));
        auto r = titleArea.withHeight (size * 2.05f);

        // per-era title treatment
        if (e == 3) { g.setColour (t.accent2); g.drawFittedText (title, r.translated (4, 4).toNearestInt(), Justification::topLeft, 2, 1.0f); }
        if (e == 5) { g.setColour (t.accent); g.drawFittedText (title, r.translated (4, 4).toNearestInt(), Justification::topLeft, 2, 1.0f); }
        if (e == 6) { g.setColour (t.accent2); g.drawFittedText (title, r.translated (-3, 0).toNearestInt(), Justification::topLeft, 2, 1.0f);
                      g.setColour (t.accent);  g.drawFittedText (title, r.translated (3, 0).toNearestInt(), Justification::topLeft, 2, 1.0f); }
        if (e == 7) { g.setColour (t.accent); g.drawFittedText (title, r.translated (0, 3).toNearestInt(), Justification::topLeft, 2, 1.0f); }
        if (e == 11 || e == 4) { g.setColour (t.accent.withAlpha (0.35f)); g.drawFittedText (title, r.translated (0, 2).toNearestInt(), Justification::topLeft, 2, 1.0f); }

        g.setColour (e == 7 ? Colour (0xfffffaf0) : t.ink);
        g.drawFittedText (title, r.toNearestInt(), Justification::topLeft, 2, 1.0f);

        auto lineR = Rectangle<float> (titleArea.getX(), r.getBottom() + 4, titleArea.getWidth(), 24);
        auto lf = e == 11 ? fonts.display (e, 22.0f) : fonts.body (e, e == 5 ? 11.0f : 15.0f);
        g.setFont (lf);
        if (e == 2)
        {
            g.setColour (t.panel);
            g.fillRect (lineR.withWidth (GlyphArrangement::getStringWidth (lf, E.line) + 16));
            g.setColour (t.ink);
            g.drawText (E.line, lineR.translated (8, 0), Justification::centredLeft);
        }
        else
        {
            g.setColour (e == 11 ? t.accent2 : e == 10 ? Colours::white : (e == 0 ? t.accent : e == 1 ? t.accent2 : e == 4 ? t.accent2 : e == 6 ? t.accent2 : e == 7 ? t.ink : t.accent));
            if (e == 9) g.setColour (t.muted);
            g.drawText (E.line, lineR, Justification::centredLeft);
        }
    }

    void paintRows (Graphics& g, int e, const Theme& t)
    {
        const int preset = proc.getPreset();
        for (int i = 0; i < soundsPerEra; ++i)
        {
            const int p = e * soundsPerEra + i;
            const bool sel = p == preset, on = p == playing;
            auto r = rows[i];
            auto shape = panelShape (t, r);
            if (t.frame == FrameStyle::retro) { shape.clear(); shape.addRoundedRectangle (r, r.getHeight() * 0.5f); }
            if (t.frame == FrameStyle::pixel && sel) { g.setColour (t.accent); g.fillRect (r.translated (4, 4)); }

            g.setColour (sel && (e == 3) ? t.accent2 : t.panel);
            if (e == 10 && sel) g.setColour (Colours::white);
            g.fillPath (shape);
            g.setColour (sel ? (t.accent == t.ink && e != 10 ? t.ink : t.accent) : t.panelEdge);
            if (e == 10) g.setColour (Colour (0xff111111));
            g.strokePath (shape, PathStrokeType (sel ? (t.frame == FrameStyle::pixel ? 4.0f : 2.0f) : (t.frame == FrameStyle::pixel || e == 2 || e == 3 ? 3.0f : 1.0f)));
            if (e == 10 && sel) { g.setColour (Colour (0xff111111)); g.fillRect (r.withWidth (8)); }
            if (e == 11 && sel) { g.setColour (t.accent); g.fillRect (r.withWidth (3)); }

            // play button
            auto pb = Rectangle<float> (r.getX() + 12, r.getCentreY() - 16, 32, 32);
            const bool sq = t.frame == FrameStyle::pixel || t.frame == FrameStyle::flat;
            g.setColour (on ? (t.accent == t.ink ? t.ink : t.accent) : t.lcd);
            if (e == 10) g.setColour (on ? Colour (0xffff6a1f) : Colour (0xff111111));
            if (sq) g.fillRect (pb); else g.fillEllipse (pb);
            g.setColour (on ? (t.accent == t.ink ? t.panel : t.lcd == t.accent ? t.ink : Colours::white) : t.lcdInk);
            if (e == 10) g.setColour (on ? Colour (0xff111111) : Colour (0xffff6a1f));
            Path icon;
            if (on) icon.addRectangle (pb.withSizeKeepingCentre (10, 10));
            else icon.addTriangle (pb.getCentreX() - 4, pb.getCentreY() - 7, pb.getCentreX() - 4, pb.getCentreY() + 7, pb.getCentreX() + 8, pb.getCentreY());
            g.fillPath (icon);

            // name + category
            auto textR = r.withTrimmedLeft (54).withTrimmedRight (88);
            g.setColour (t.ink);
            if (e == 6 || e == 4 || e == 11) g.setColour (Colours::white);
            g.setFont (fonts.body (e, e == 5 ? 13.0f : 15.0f));
            g.drawText (sound (p).name, textR.withHeight (r.getHeight() * 0.6f), Justification::bottomLeft);
            g.setColour ((e == 6 || e == 4 || e == 11) ? Colours::white.withAlpha (0.8f) : t.ink.withAlpha (0.72f));
            g.setFont (fonts.body (e, 12.0f));
            g.drawText (String (sound (p).category) + String::fromUTF8 ("  \xc2\xb7  ") + String (proc.getSoundLengthSeconds (p), 1) + "s",
                        textR.withTrimmedTop (r.getHeight() * 0.6f), Justification::topLeft);

            // drag handle
            auto chip = Rectangle<float> (r.getRight() - 84, r.getCentreY() - 13, 72, 26);
            g.setColour (t.lcd.withAlpha (0.9f));
            if (sq) g.fillRect (chip); else g.fillRoundedRectangle (chip, jmin (t.corner, 13.0f));
            g.setColour (t.lcdInk);
            for (int k = 0; k < 3; ++k)
                g.fillRect (chip.getX() + 16.0f, chip.getCentreY() - 5.0f + (float) k * 4.0f, 9.0f, 1.6f);
            g.setFont (fonts.body (e, 13.0f));
            g.drawText ("WAV", chip.withTrimmedLeft (22), Justification::centred);
        }
    }

    void paintKnobPanel (Graphics& g, int e, const Theme& t)
    {
        auto r = knobPanel;
        auto shape = panelShape (t, r);
        if (t.frame == FrameStyle::pixel) { g.setColour (Colour (0xff141414)); g.fillRect (r.translated (6, 6)); }
        Colour fill = t.panel;
        if (e == 3) fill = t.ink;
        if (e == 5) fill = Colour (0xff3a3a3a);
        if (e == 10) fill = Colour (0xff111111);
        if (e == 11) { g.setGradientFill (ColourGradient (Colour (0xff232327), 0, r.getY(), Colour (0xff141417), 0, r.getBottom(), false)); }
        else if (e == 8) { g.setGradientFill (ColourGradient (Colours::white, 0, r.getY(), Colour (0xffd6e4ea), 0, r.getBottom(), false)); }
        else g.setColour (fill);
        g.fillPath (shape);
        g.setColour (e == 0 ? t.ink : t.panelEdge);
        g.strokePath (shape, PathStrokeType (e == 0 || e == 2 || e == 5 ? 3.0f : 1.0f));
        if (e == 11)
        {
            g.setColour (Colour (0xff8a8a90));
            for (auto pt : { r.getTopLeft().translated (7, 7), r.getTopRight().translated (-7, 7), r.getBottomLeft().translated (7, -7), r.getBottomRight().translated (-7, -7) })
                g.fillEllipse (Rectangle<float> (6, 6).withCentre (pt));
        }

        const bool darkPanel = fill.getPerceivedBrightness() < 0.4f || e == 11 || e == 4 || e == 6;
        const Colour label = (e == 3 ? t.accent2 : darkPanel ? Colours::white : t.ink);
        for (auto& k : knobs)
        {
            auto kb = k.slider.getBounds().toFloat();
            g.setColour (label);
            g.setFont (fonts.body (e, 13.0f));
            g.drawText (k.name, kb.withY (r.getY() + 4).withHeight (18).expanded (24, 0), Justification::centred);
            auto vb = Rectangle<float> (kb.getCentreX() - 36, kb.getBottom() + 3, 72, 19);
            g.setColour (t.lcd);
            if (t.frame == FrameStyle::pixel || t.frame == FrameStyle::flat) g.fillRect (vb); else g.fillRoundedRectangle (vb, 3.0f);
            g.setColour (t.lcdInk);
            g.setFont (fonts.body (e, 12.5f));
            auto* param = proc.state.getParameter (k.id);
            g.drawText (param != nullptr ? param->getCurrentValueAsText() : String(), vb, Justification::centred);
        }
    }

    void paintStrip (Graphics& g, int e)
    {
        const float w = (strip.getWidth() - 5.0f * 11.0f) / 12.0f;
        for (int i = 0; i < numEras; ++i)
        {
            Rectangle<float> r (strip.getX() + (float) i * (w + 5.0f), strip.getY(), w, strip.getHeight());
            const bool sel = i == e;
            if (sel) r = r.translated (0, -4);
            drawCover (g, thumbs[i], r);
            if (! sel)
            {
                g.setColour (Colours::black.withAlpha (0.45f));
                g.fillRect (r);
            }
            g.setGradientFill (ColourGradient (Colours::transparentBlack, 0, r.getY() + r.getHeight() * 0.35f, Colours::black.withAlpha (0.97f), 0, r.getBottom() - 20.0f, false));
            g.fillRect (r);
            g.setColour (sel ? Colour (0xffffd23a) : Colours::white);
            g.setFont (fonts.from (fonts.barlow, 22.0f));
            g.drawText (era (i).year, r.withTrimmedTop (r.getHeight() - 44).withTrimmedLeft (6), Justification::topLeft);
            g.setColour (Colours::white);
            g.setFont (fonts.sys ("Arial", 12.5f, Font::bold));
            g.drawText (era (i).shortName, r.withTrimmedTop (r.getHeight() - 20).withTrimmedLeft (6), Justification::topLeft);
            g.setColour (sel ? Colour (0xffffd23a) : Colours::black.withAlpha (0.5f));
            g.drawRect (r, sel ? 2.0f : 1.0f);
        }
    }

    void paintIntro (Graphics& g)
    {
        auto b = getLocalBounds().toFloat();
        g.fillAll (Colour (0xff050405));
        const float w = b.getWidth() / (float) numEras;
        for (int i = 0; i < numEras; ++i)
        {
            Rectangle<float> r ((float) i * w, 0, w, b.getHeight());
            drawCover (g, covers[i], r, 0.25f);
            g.setGradientFill (ColourGradient (Colour (0x26050405), 0, 0, Colour (0xf2050405), 0, b.getHeight(), false));
            g.fillRect (r);
            g.setColour (Colours::black);
            g.drawVerticalLine ((int) r.getRight(), 0, b.getHeight());
            g.setColour (Colours::white);
            g.setFont (fonts.from (fonts.barlow, 18.0f));
            g.drawText (era (i).year, r.withTrimmedTop (b.getHeight() - 120).withHeight (24), Justification::centred);
        }
        g.setGradientFill (ColourGradient (Colour (0xeb050405), b.getCentreX(), b.getHeight() * 0.46f, Colour (0x26050405), b.getCentreX() + 560, b.getHeight() * 0.46f, true));
        g.fillAll();
        g.setColour (Colours::black.withAlpha (0.18f));
        for (int y = 0; y < (int) b.getHeight(); y += 3) g.drawHorizontalLine (y, 0, b.getWidth());

        // rainbow of all era colours on top
        const float sw = b.getWidth() / (float) numEras;
        for (int i = 0; i < numEras; ++i)
        {
            g.setColour (era (i).theme.accent == era (i).theme.ink ? era (i).theme.accent2 : era (i).theme.accent);
            g.fillRect ((float) i * sw, 0.0f, sw, 4.0f);
        }

        g.setColour (Colour (0xff9aa3b5));
        g.setFont (Font (FontOptions (11.0f, Font::bold)).withExtraKerningFactor (0.4f));
        g.drawText ("TRAPWIN PRESENTS", 0, 100, (int) b.getWidth(), 20, Justification::centred);

        auto logoF = fonts.from (fonts.barlow, 170.0f).italicised();
        g.setFont (logoF);
        g.setColour (Colour (0xff1c2b5a));
        g.drawText ("BIT ERA", 0, 128, (int) b.getWidth(), 170, Justification::centred);
        g.setGradientFill (ColourGradient (Colours::white, 0, 150, Colour (0xff6c7488), 0, 230, false));
        g.drawText ("BIT ERA", 0, 122, (int) b.getWidth(), 170, Justification::centred);

        g.setColour (Colour (0xffffd23a));
        g.setFont (fonts.from (fonts.barlow, 30.0f));
        g.drawText ("100 YEARS OF ANIME. 50 YEARS OF GAME SOUND. ONE PLUGIN.", 0, 300, (int) b.getWidth(), 36, Justification::centred);
        g.setColour (Colours::white.withAlpha (0.85f));
        g.setFont (fonts.from (fonts.courierBold, 13.0f));
        g.drawText (String (CharPointer_UTF8 ("12 ERAS   \xe2\x9c\x96   36 SOUNDS (TEST BUILD)   \xe2\x9c\x96   1917 \xe2\x86\x92 NOW")), 0, 344, (int) b.getWidth(), 20, Justification::centred);

        Path btn;
        auto s = startBtn;
        btn.startNewSubPath (s.getX(), s.getY()); btn.lineTo (s.getRight(), s.getY()); btn.lineTo (s.getRight(), s.getY() + s.getHeight() * 0.68f);
        btn.lineTo (s.getRight() - 16, s.getBottom()); btn.lineTo (s.getX(), s.getBottom()); btn.closeSubPath();
        g.setColour (Colour (0xffffd23a));
        g.fillPath (btn);
        g.setColour (Colour (0xff05070b));
        g.setFont (fonts.from (fonts.barlow, 30.0f));
        g.drawText (String (CharPointer_UTF8 ("\xe2\x96\xb6 PRESS START")), s, Justification::centred);

        g.setColour (Colour (0xff9aa3b5));
        g.setFont (Font (FontOptions (11.0f)));
        g.drawText ("Pick any year on the timeline. Play a sound, then drag it straight into your DAW.", 0, 516, (int) b.getWidth(), 18, Justification::centred);
        g.setFont (Font (FontOptions (10.0f, Font::bold)));
        g.drawText (String::fromUTF8 ("VST3 \xc2\xb7 AU \xc2\xb7 v") + BE_VERSION_STRING, 24, (int) b.getHeight() - 34, 300, 16, Justification::centredLeft);
        g.drawText ("TRAPWIN.COM", (int) b.getWidth() - 324, (int) b.getHeight() - 34, 300, 16, Justification::centredRight);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Content)
};
} // namespace bitera

//==============================================================================
BitEraEditor::BitEraEditor (BitEraProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    content = std::make_unique<bitera::Content> (p);
    content->setBounds (0, 0, designWidth, designHeight);
    addAndMakeVisible (*content);

    setResizable (true, true);
    setResizeLimits (designWidth / 2, designHeight / 2, designWidth * 2, designHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) designWidth / (double) designHeight);
    setSize (designWidth, designHeight);
}

BitEraEditor::~BitEraEditor() = default;

void BitEraEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colours::black); }

void BitEraEditor::resized()
{
    const float scale = (float) getWidth() / (float) designWidth;
    content->setTransform (juce::AffineTransform::scale (scale));
}
