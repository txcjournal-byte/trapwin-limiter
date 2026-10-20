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

    Font getComboBoxFont (ComboBox& box) override { return Font (FontOptions (jmin (13.5f, (float) box.getHeight() * 0.55f), Font::bold)); }
    Font getPopupMenuFont() override { return Font (FontOptions (14.0f)); }
    void positionComboBoxText (ComboBox& box, Label& label) override
    {
        label.setBounds (4, 1, box.getWidth() - 20, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }
    void drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box) override
    {
        auto r = Rectangle<float> (0, 0, (float) w, (float) h);
        g.setColour (box.findColour (ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (box.findColour (ComboBox::outlineColourId));
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
        Path arrow;
        const float ax = (float) w - 11.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
        g.setColour (box.findColour (ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

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
// panel colours shared by the knob panel and the engine pages
Colour panelFill (int e, const Theme& t)
{
    if (e == 3) return t.ink;
    if (e == 5) return Colour (0xff3a3a3a);
    if (e == 10 || e == 11) return Colour (0xff17171a);
    if (e == 8) return Colours::white;
    return t.panel;
}

bool panelIsDark (int e, const Theme& t) { return panelFill (e, t).getPerceivedBrightness() < 0.4f || e == 11 || e == 4 || e == 6; }

Colour panelLabel (int e, const Theme& t) { return e == 3 ? t.accent2 : panelIsDark (e, t) ? Colours::white : t.ink; }

//==============================================================================
// LAYERS page: 4 layers (source, wave, 24 knobs) + the master voice settings
class LayerPage : public Component, private Timer
{
public:
    LayerPage (BitEraProcessor& p, const Fonts& f, KnobLookAndFeel& lnf) : proc (p), fonts (f), knobLnf (lnf)
    {
        static const char* ids[24] = { "level", "pan", "coarse", "fine", "char", "vel", "keylo", "keyhi",
                                       "attack", "decay", "sustain", "release", "cutoff", "reso", "fenv", "ktrack",
                                       "fattack", "fdecay", "fsustain", "frelease", "penv", "pdecay", "vibdepth", "vibrate" };
        static const char* names[24] = { "LEVEL", "PAN", "COARSE", "FINE", "CHARACTER", "VELOCITY", "KEY LOW", "KEY HIGH",
                                         "ATTACK", "DECAY", "SUSTAIN", "RELEASE", "CUTOFF", "RESONANCE", "FILTER ENV", "KEY TRACK",
                                         "F. ATTACK", "F. DECAY", "F. SUSTAIN", "F. RELEASE", "PITCH ENV", "PITCH DECAY", "VIBRATO", "VIB RATE" };
        for (int i = 0; i < 24; ++i)
        {
            cells[i].id = ids[i]; cells[i].name = names[i];
            setupKnob (cells[i].slider);
            addAndMakeVisible (cells[i].slider);
        }
        static const char* mIds[3] = { "glide", "bend", "volume" };
        static const char* mNames[3] = { "GLIDE", "BEND", "VOLUME" };
        for (int i = 0; i < 3; ++i)
        {
            master[i].id = mIds[i]; master[i].name = mNames[i];
            setupKnob (master[i].slider);
            addAndMakeVisible (master[i].slider);
            if (i > 0) master[i].attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.state, mIds[i], master[i].slider);
        }
        for (int s = 0; s < chips::numSources; ++s) source.addItem (chips::sourceName (s), s + 1);
        for (int f = 0; f < numFilterTypes; ++f) filter.addItem (filterName (f), f + 1);
        mode.addItemList ({ "POLY", "MONO", "LEGATO" }, 1);
        for (auto* c : { &source, &filter, &mode }) { addAndMakeVisible (*c); c->setJustificationType (Justification::centredLeft); }
        source.onChange = [this] { clampWave(); repaint(); };
        attachLayer (0);
        startTimerHz (15);
    }

    ~LayerPage() override
    {
        for (auto& c : cells) c.slider.setLookAndFeel (nullptr);
        for (auto& c : master) c.slider.setLookAndFeel (nullptr);
    }

    void setTheme (const Theme* th, int eraIndex)
    {
        theme = th; e = eraIndex;
        const auto& t = *theme;
        const Colour bg = panelFill (e, t), ink = panelLabel (e, t);
        for (auto* c : { &source, &filter, &mode })
        {
            c->setColour (ComboBox::backgroundColourId, t.lcd);
            c->setColour (ComboBox::textColourId, t.lcdInk);
            c->setColour (ComboBox::outlineColourId, t.lcdInk.withAlpha (0.4f));
            c->setColour (ComboBox::arrowColourId, t.lcdInk);
        }
        knobLnf.setColour (PopupMenu::backgroundColourId, t.lcd);
        knobLnf.setColour (PopupMenu::textColourId, t.lcdInk);
        knobLnf.setColour (PopupMenu::highlightedBackgroundColourId, t.accent);
        knobLnf.setColour (PopupMenu::highlightedTextColourId, t.accent.getPerceivedBrightness() > 0.6f ? Colours::black : Colours::white);
        for (auto* c : { &source, &filter, &mode }) c->setLookAndFeel (&knobLnf);
        ignoreUnused (bg, ink);
        repaint();
    }

    void resized() override
    {
        auto b = getLocalBounds().toFloat();
        top = b.removeFromTop (44);
        bottom = b.removeFromBottom (54);
        grid = b.reduced (0, 4);
        // layer tabs, then source / wave / filter
        for (int i = 0; i < numLayers; ++i) layerBtn[i] = { top.getX() + (float) i * 47.0f, top.getY() + 4, 43, 36 };
        source.setBounds (Rectangle<float> (layerBtn[3].getRight() + 10, top.getY() + 7, 176, 30).toNearestInt());
        waveArea = { (float) source.getRight() + 8, top.getY() + 7, 222, 30 };
        filter.setBounds (Rectangle<float> (waveArea.getRight() + 8, top.getY() + 7, top.getRight() - waveArea.getRight() - 8, 30).toNearestInt());

        const float cw = grid.getWidth() / 8.0f, ch = grid.getHeight() / 3.0f;
        for (int i = 0; i < 24; ++i)
        {
            auto cell = Rectangle<float> (grid.getX() + (float) (i % 8) * cw, grid.getY() + (float) (i / 8) * ch, cw, ch);
            cells[i].area = cell;
            cells[i].slider.setBounds (cell.withTrimmedTop (20).withHeight (54).withSizeKeepingCentre (52, 52).toNearestInt());
        }
        // master row: MASTER label, mode combo, 3 knobs with their value next to them
        mode.setBounds (Rectangle<float> (bottom.getX() + 92, bottom.getY() + 14, 130, 30).toNearestInt());
        for (int i = 0; i < 3; ++i)
        {
            auto a = Rectangle<float> (bottom.getX() + 240 + (float) i * 170, bottom.getY() + 6, 160, 46);
            master[i].area = a;
            master[i].slider.setBounds (a.withWidth (46).reduced (2).toNearestInt());
        }
    }

    void paint (Graphics& g) override
    {
        if (theme == nullptr) return;
        const auto& t = *theme;
        const Colour fill = panelFill (e, t), ink = panelLabel (e, t);

        // layer tabs
        for (int i = 0; i < numLayers; ++i)
        {
            const bool sel = i == layer, on = isOn (i);
            auto r = layerBtn[i];
            g.setColour (sel ? t.accent : fill);
            g.fillRoundedRectangle (r, jmin (t.corner, 6.0f));
            g.setColour (sel ? t.accent : t.panelEdge);
            g.drawRoundedRectangle (r, jmin (t.corner, 6.0f), 1.5f);
            const Colour txt = sel ? (t.accent.getPerceivedBrightness() > 0.6f ? Colours::black : Colours::white) : ink;
            g.setColour (txt);
            g.setFont (fonts.body (e, 15.0f));
            g.drawText ("L" + String (i + 1), r.withTrimmedLeft (19), Justification::centredLeft);
            auto dot = Rectangle<float> (r.getX() + 7, r.getCentreY() - 5, 10, 10);
            g.setColour (on ? Colour (0xff3ddc84) : txt.withAlpha (0.35f));
            g.fillEllipse (dot);
        }

        // wave selector
        g.setColour (t.lcd);
        g.fillRoundedRectangle (waveArea, 4.0f);
        g.setColour (t.lcdInk);
        g.setFont (fonts.body (e, 15.0f));
        g.drawText ("<", waveArea.withWidth (28), Justification::centred);
        g.drawText (">", waveArea.withTrimmedLeft (waveArea.getWidth() - 28), Justification::centred);
        g.drawText (chips::waveName (sourceIndex(), waveIndex()), waveArea.reduced (30, 0), Justification::centred);

        // knob grid on the panel
        auto panel = grid.expanded (0, 2);
        g.setColour (fill);
        if (e == 8) g.setGradientFill (ColourGradient (Colours::white, 0, panel.getY(), Colour (0xffd6e4ea), 0, panel.getBottom(), false));
        g.fillPath (panelShape (t, panel));
        g.setColour (e == 0 ? t.ink : t.panelEdge);
        g.strokePath (panelShape (t, panel), PathStrokeType (e == 0 || e == 2 || e == 5 ? 2.0f : 1.0f));
        // section dividers: osc | env | filter | mod
        g.setColour (ink.withAlpha (0.18f));
        g.drawVerticalLine ((int) (grid.getX() + grid.getWidth() * 0.5f), panel.getY() + 8, panel.getBottom() - 8);
        for (int r = 1; r < 3; ++r) g.drawHorizontalLine ((int) (grid.getY() + grid.getHeight() * r / 3.0f), panel.getX() + 8, panel.getRight() - 8);

        const bool layerOn = isOn (layer);
        for (auto& c : cells)
            paintCell (g, c, ink, layerOn ? 1.0f : 0.45f);

        // master row
        g.setColour (fill);
        g.fillPath (panelShape (t, bottom.withTrimmedTop (4)));
        g.setColour (t.panelEdge);
        g.strokePath (panelShape (t, bottom.withTrimmedTop (4)), PathStrokeType (1.0f));
        g.setColour (ink);
        g.setFont (fonts.body (e, 14.0f));
        g.drawText ("L" + String (layer + 1) + " VOICE", bottom.withTrimmedLeft (14).withTrimmedTop (4), Justification::centredLeft);
        for (auto& m : master)
        {
            g.setColour (ink);
            g.setFont (fonts.body (e, 12.0f));
            auto txt = m.area.withTrimmedLeft (50);
            g.drawText (m.name, txt.withHeight (txt.getHeight() * 0.5f), Justification::bottomLeft);
            g.setFont (fonts.body (e, 13.0f, false));
            auto* param = proc.state.getParameter (&m == &master[0] ? BitEraProcessor::layerId (layer, m.id) : String (m.id));
            g.drawText (param != nullptr ? param->getCurrentValueAsText() : String(), txt.withTrimmedTop (txt.getHeight() * 0.5f), Justification::topLeft);
        }
    }

    void mouseUp (const MouseEvent& ev) override
    {
        const auto p = ev.position;
        for (int i = 0; i < numLayers; ++i)
            if (layerBtn[i].contains (p))
            {
                if (p.x < layerBtn[i].getX() + 20 || i == layer) setOn (i, ! isOn (i));   // the dot (or a second click) switches it
                attachLayer (i);
                return;
            }
        if (waveArea.contains (p))
        {
            if (p.x < waveArea.getX() + 30) stepWave (-1);
            else if (p.x > waveArea.getRight() - 30) stepWave (1);
            else showWaveMenu();
        }
    }

    int currentLayer() const { return layer; }

private:
    struct Cell
    {
        Slider slider;
        std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attachment;
        const char* id = "";
        const char* name = "";
        Rectangle<float> area;
    };

    void setupKnob (Slider& s)
    {
        s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        s.setLookAndFeel (&knobLnf);
        s.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        s.setPopupDisplayEnabled (true, true, this);
        s.onValueChange = [this] { repaint(); };
    }

    void paintCell (Graphics& g, const Cell& c, Colour ink, float alpha)
    {
        const auto& t = *theme;
        g.setColour (ink.withMultipliedAlpha (alpha));
        g.setFont (fonts.body (e, 12.0f));
        g.drawText (c.name, c.area.withHeight (20).translated (0, 2), Justification::centred);
        auto vb = Rectangle<float> (c.area.getCentreX() - 40, c.area.getY() + 76, 80, 19);
        g.setColour (t.lcd.withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (vb, 3.0f);
        g.setColour (t.lcdInk.withMultipliedAlpha (alpha));
        g.setFont (fonts.body (e, 12.0f));
        auto* param = proc.state.getParameter (BitEraProcessor::layerId (layer, c.id));
        g.drawText (param != nullptr ? param->getCurrentValueAsText() : String(), vb, Justification::centred);
    }

    void attachLayer (int l)
    {
        layer = jlimit (0, numLayers - 1, l);
        for (auto& c : cells)
        {
            c.attachment.reset();
            c.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.state, BitEraProcessor::layerId (layer, c.id), c.slider);
        }
        sourceAttachment.reset(); filterAttachment.reset(); modeAttachment.reset();
        master[0].attachment.reset();
        master[0].attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.state, BitEraProcessor::layerId (layer, "glide"), master[0].slider);
        modeAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.state, BitEraProcessor::layerId (layer, "mode"), mode);
        sourceAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.state, BitEraProcessor::layerId (layer, "src"), source);
        filterAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.state, BitEraProcessor::layerId (layer, "filter"), filter);
        repaint();
    }

    RangedAudioParameter* param (const char* n) const { return proc.state.getParameter (BitEraProcessor::layerId (layer, n)); }
    bool isOn (int l) const { auto* p = proc.state.getParameter (BitEraProcessor::layerId (l, "on")); return p != nullptr && p->getValue() > 0.5f; }
    void setOn (int l, bool on) { if (auto* p = proc.state.getParameter (BitEraProcessor::layerId (l, "on"))) p->setValueNotifyingHost (on ? 1.0f : 0.0f); repaint(); }
    int sourceIndex() const { auto* p = param ("src"); return p != nullptr ? roundToInt (p->convertFrom0to1 (p->getValue())) : 0; }
    int waveIndex() const { auto* p = param ("wave"); return p != nullptr ? roundToInt (p->convertFrom0to1 (p->getValue())) : 0; }
    void setWave (int w)
    {
        if (auto* p = param ("wave"))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) jlimit (0, chips::numWaves (sourceIndex()) - 1, w)));
        repaint();
    }
    void stepWave (int d) { const int n = chips::numWaves (sourceIndex()); setWave ((waveIndex() + d + n) % n); }
    void clampWave() { if (waveIndex() >= chips::numWaves (sourceIndex())) setWave (0); }

    void showWaveMenu()
    {
        const int src = sourceIndex(), n = chips::numWaves (src);
        PopupMenu menu;
        menu.setLookAndFeel (&knobLnf);
        if (chips::isSampler (src))
        {
            for (int ei = 0; ei < numEras; ++ei)
            {
                PopupMenu sub;
                for (int k = 0; k < soundsPerEra; ++k)
                {
                    const int w = ei * soundsPerEra + k;
                    if (w < n) sub.addItem (w + 1, chips::waveName (src, w), true, w == waveIndex());
                }
                menu.addSubMenu (String (era (ei).year) + "  " + era (ei).shortName, sub);
            }
        }
        else
            for (int w = 0; w < n; ++w) menu.addItem (w + 1, chips::waveName (src, w), true, w == waveIndex());
        menu.showMenuAsync (PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (waveArea.toNearestInt())),
                            [safe = SafePointer<LayerPage> (this)] (int r) { if (safe != nullptr && r > 0) safe->setWave (r - 1); });
    }

    void timerCallback() override { repaint(); }

    BitEraProcessor& proc;
    const Fonts& fonts;
    KnobLookAndFeel& knobLnf;
    const Theme* theme = nullptr;
    int e = 0, layer = 0;
    Cell cells[24], master[3];
    ComboBox source, filter, mode;
    std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment, filterAttachment, modeAttachment;
    Rectangle<float> top, grid, bottom, waveArea, layerBtn[numLayers];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LayerPage)
};

//==============================================================================
// shared bits of the engine pages (MOD, FX, ARP, MACRO)
class Page : public Component, private Timer
{
public:
    Page (BitEraProcessor& p, const Fonts& f, KnobLookAndFeel& lnf) : proc (p), fonts (f), knobLnf (lnf) { startTimerHz (15); }
    ~Page() override = default;

    void setTheme (const Theme* th, int eraIndex)
    {
        theme = th; e = eraIndex;
        for (auto* c : allCombos) styleCombo (*c);
        repaint();
    }

protected:
    struct Knob
    {
        ~Knob() { attachment.reset(); slider.setLookAndFeel (nullptr); }
        Slider slider;
        std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attachment;
        String id, label;
        Rectangle<float> area;   // label above, value below
    };

    struct Combo
    {
        ~Combo() { attachment.reset(); box.setLookAndFeel (nullptr); }
        ComboBox box;
        std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
        String id;
    };

    Knob& addKnob (Knob& k, const String& label)
    {
        k.label = label;
        k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        k.slider.setLookAndFeel (&knobLnf);
        k.slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        k.slider.setPopupDisplayEnabled (true, true, this);
        k.slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (k.slider);
        allKnobs.push_back (&k);
        return k;
    }

    void attach (Knob& k, const String& id)
    {
        k.attachment.reset();
        k.id = id;
        if (proc.state.getParameter (id) != nullptr)
            k.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.state, id, k.slider);
    }

    void addCombo (Combo& c)
    {
        c.box.setJustificationType (Justification::centredLeft);
        addAndMakeVisible (c.box);
        allCombos.push_back (&c.box);
        c.box.onChange = [this] { repaint(); };
    }

    void attach (Combo& c, const String& id)
    {
        c.attachment.reset();
        c.id = id;
        c.box.clear (dontSendNotification);
        if (auto* p = dynamic_cast<AudioParameterChoice*> (proc.state.getParameter (id)))
        {
            c.box.addItemList (p->choices, 1);
            c.attachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.state, id, c.box);
        }
    }

    void styleCombo (ComboBox& c)
    {
        if (theme == nullptr) return;
        const auto& t = *theme;
        c.setColour (ComboBox::backgroundColourId, t.lcd);
        c.setColour (ComboBox::textColourId, t.lcdInk);
        c.setColour (ComboBox::outlineColourId, t.lcdInk.withAlpha (0.4f));
        c.setColour (ComboBox::arrowColourId, t.lcdInk);
        c.setLookAndFeel (&knobLnf);
    }

    float value (const String& id) const
    {
        auto* p = proc.state.getParameter (id);
        return p != nullptr ? p->convertFrom0to1 (p->getValue()) : 0.0f;
    }
    void setValue (const String& id, float v)
    {
        if (auto* p = proc.state.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
    }

    Colour ink() const { return panelLabel (e, *theme); }

    void paintPanel (Graphics& g, Rectangle<float> r) const
    {
        const auto& t = *theme;
        auto shape = panelShape (t, r);
        g.setColour (panelFill (e, t));
        if (e == 8) g.setGradientFill (ColourGradient (Colours::white, 0, r.getY(), Colour (0xffd6e4ea), 0, r.getBottom(), false));
        g.fillPath (shape);
        g.setColour (e == 0 ? t.ink : t.panelEdge);
        g.strokePath (shape, PathStrokeType (e == 0 || e == 2 || e == 5 ? 2.0f : 1.0f));
    }

    void paintHeading (Graphics& g, const String& text, Rectangle<float> r) const
    {
        g.setColour (theme->accent == theme->ink ? ink() : theme->accent);
        if (panelIsDark (e, *theme) && theme->accent.getPerceivedBrightness() < 0.25f) g.setColour (ink());
        g.setFont (fonts.body (e, 13.0f));
        g.drawText (text, r, Justification::centredLeft);
    }

    void paintKnob (Graphics& g, const Knob& k, const String& valueText, float alpha = 1.0f) const
    {
        const auto& t = *theme;
        g.setColour (ink().withMultipliedAlpha (alpha));
        g.setFont (fonts.body (e, 11.5f));
        g.drawText (k.label, k.area.withHeight (16), Justification::centred);
        auto vb = Rectangle<float> (k.area.getCentreX() - 38, k.area.getBottom() - 18, 76, 17);
        g.setColour (t.lcd.withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (vb, 3.0f);
        g.setColour (t.lcdInk.withMultipliedAlpha (alpha));
        g.setFont (fonts.body (e, 11.5f));
        g.drawText (valueText, vb, Justification::centred);
    }

    String paramText (const String& id) const
    {
        auto* p = proc.state.getParameter (id);
        return p != nullptr ? p->getCurrentValueAsText() : String();
    }

    // knob inside an area: label 16 px on top, value 18 px at the bottom
    static void placeKnob (Knob& k, Rectangle<float> area)
    {
        k.area = area;
        const float size = jmin (area.getWidth() - 8.0f, area.getHeight() - 38.0f);
        k.slider.setBounds (area.withTrimmedTop (17).withTrimmedBottom (20).withSizeKeepingCentre (size, size).toNearestInt());
    }

    void paintTabs (Graphics& g, Rectangle<float>* tabs, const StringArray& names, int selected) const
    {
        const auto& t = *theme;
        for (int i = 0; i < names.size(); ++i)
        {
            const bool sel = i == selected;
            g.setColour (sel ? t.accent : panelFill (e, t));
            g.fillRoundedRectangle (tabs[i], jmin (t.corner, 6.0f));
            g.setColour (sel ? t.accent : t.panelEdge);
            g.drawRoundedRectangle (tabs[i], jmin (t.corner, 6.0f), 1.5f);
            g.setColour (sel ? (t.accent.getPerceivedBrightness() > 0.6f ? Colours::black : Colours::white) : ink());
            g.setFont (fonts.body (e, 14.0f));
            g.drawText (names[i], tabs[i], Justification::centred);
        }
    }

    void timerCallback() override { if (isShowing()) repaint(); }

    BitEraProcessor& proc;
    const Fonts& fonts;
    KnobLookAndFeel& knobLnf;
    const Theme* theme = nullptr;
    int e = 0;
    std::vector<Knob*> allKnobs;
    std::vector<ComboBox*> allCombos;
};

//==============================================================================
// MOD page: 3 LFOs, 2 mod envelopes and the 16-slot matrix
class ModPage : public Page
{
public:
    ModPage (BitEraProcessor& p, const Fonts& f, KnobLookAndFeel& lnf) : Page (p, f, lnf)
    {
        for (int k = 0; k < numLfos; ++k)
        {
            const String n = "lfo" + String (k + 1) + "_";
            addCombo (lfoShape[k]); attach (lfoShape[k], n + "shape");
            addCombo (lfoSync[k]); attach (lfoSync[k], n + "sync");
            addCombo (lfoRetrig[k]); attach (lfoRetrig[k], n + "retrig");
            addCombo (lfoUni[k]); attach (lfoUni[k], n + "uni");
            addKnob (lfoRate[k], "RATE"); attach (lfoRate[k], n + "rate");
            addKnob (lfoFade[k], "FADE IN"); attach (lfoFade[k], n + "fade");
        }
        static const char* envNames[4] = { "A", "D", "S", "R" };
        static const char* envIds[4] = { "a", "d", "s", "r" };
        for (int k = 0; k < numModEnvs; ++k)
            for (int q = 0; q < 4; ++q) { addKnob (env[k][q], envNames[q]); attach (env[k][q], "menv" + String (k + 1) + "_" + envIds[q]); }
        for (int s = 0; s < numModSlots; ++s)
        {
            const String n = "mod" + String (s + 1) + "_";
            addCombo (slotSrc[s]); attach (slotSrc[s], n + "src");
            addCombo (slotLayer[s]); attach (slotLayer[s], n + "layer");
            addCombo (slotDst[s]); attach (slotDst[s], n + "dst");
            slotAmt[s].setSliderStyle (Slider::LinearBar);
            slotAmt[s].setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
            slotAmt[s].setPopupDisplayEnabled (true, true, this);
            addAndMakeVisible (slotAmt[s]);
            slotAmtAtt[s] = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.state, n + "amt", slotAmt[s]);
        }
    }

    void resized() override
    {
        auto b = getLocalBounds().toFloat();
        auto top = b.removeFromTop (156);
        const float pw = (top.getWidth() - 4 * 6.0f) / 5.0f;
        for (int i = 0; i < 5; ++i) srcPanel[i] = { top.getX() + (float) i * (pw + 6.0f), top.getY(), pw, top.getHeight() };
        for (int k = 0; k < numLfos; ++k)
        {
            auto r = srcPanel[k].reduced (6).withTrimmedTop (20);
            auto row1 = r.removeFromTop (22);
            lfoShape[k].box.setBounds (row1.removeFromLeft (row1.getWidth() * 0.58f).reduced (1).toNearestInt());
            lfoSync[k].box.setBounds (row1.reduced (1).toNearestInt());
            auto row2 = r.removeFromBottom (22);
            lfoRetrig[k].box.setBounds (row2.removeFromLeft (row2.getWidth() * 0.5f).reduced (1).toNearestInt());
            lfoUni[k].box.setBounds (row2.reduced (1).toNearestInt());
            placeKnob (lfoRate[k], r.removeFromLeft (r.getWidth() * 0.5f));
            placeKnob (lfoFade[k], r);
        }
        for (int k = 0; k < numModEnvs; ++k)
        {
            auto r = srcPanel[3 + k].reduced (4).withTrimmedTop (22);
            const float w = r.getWidth() / 2.0f, h = r.getHeight() / 2.0f;
            for (int q = 0; q < 4; ++q) placeKnob (env[k][q], { r.getX() + (q % 2) * w, r.getY() + (q / 2) * h, w, h });
        }
        auto m = b.withTrimmedTop (8);
        matrix = m;
        auto inner = m.reduced (8).withTrimmedTop (18);
        const float colW = (inner.getWidth() - 12.0f) / 2.0f, rowH = inner.getHeight() / 8.0f;
        for (int s = 0; s < numModSlots; ++s)
        {
            auto r = Rectangle<float> (inner.getX() + (s / 8) * (colW + 12.0f), inner.getY() + (s % 8) * rowH, colW, rowH).reduced (0, 1.5f);
            slotRow[s] = r;
            r.removeFromLeft (20);
            slotSrc[s].box.setBounds (r.removeFromLeft (98).reduced (1, 0).toNearestInt());
            slotLayer[s].box.setBounds (r.removeFromLeft (50).reduced (1, 0).toNearestInt());
            slotDst[s].box.setBounds (r.removeFromLeft (124).reduced (1, 0).toNearestInt());
            slotAmt[s].setBounds (r.reduced (2, 2).toNearestInt());
        }
    }

    void paint (Graphics& g) override
    {
        if (theme == nullptr) return;
        const auto& t = *theme;
        static const char* titles[5] = { "LFO 1", "LFO 2", "LFO 3", "MOD ENV 1", "MOD ENV 2" };
        for (int i = 0; i < 5; ++i)
        {
            paintPanel (g, srcPanel[i]);
            paintHeading (g, titles[i], srcPanel[i].reduced (8, 4).withHeight (18));
        }
        for (int k = 0; k < numLfos; ++k) { paintKnob (g, lfoRate[k], paramText (lfoRate[k].id), value ("lfo" + String (k + 1) + "_sync") > 0.5f ? 0.4f : 1.0f); paintKnob (g, lfoFade[k], paramText (lfoFade[k].id)); }
        for (auto& en : env) for (auto& q : en) paintKnob (g, q, paramText (q.id));

        paintPanel (g, matrix);
        paintHeading (g, "MOD MATRIX   SOURCE  >  LAYER  >  DESTINATION  =  AMOUNT", matrix.reduced (10, 4).withHeight (18));
        for (int s = 0; s < numModSlots; ++s)
        {
            g.setColour (ink().withAlpha (0.8f));
            g.setFont (fonts.body (e, 11.0f));
            g.drawText (String (s + 1), slotRow[s].withWidth (18), Justification::centredRight);
            slotAmt[s].setColour (Slider::trackColourId, t.accent.withAlpha (0.85f));
            slotAmt[s].setColour (Slider::backgroundColourId, t.lcd);
            const float amt = value ("mod" + String (s + 1) + "_amt");
            g.setColour (t.lcdInk);
            auto ab = slotAmt[s].getBounds().toFloat();
            g.setFont (fonts.body (e, 11.0f));
            ignoreUnused (ab, amt);
        }
    }

    void paintOverChildren (Graphics& g) override
    {
        if (theme == nullptr) return;
        for (int s = 0; s < numModSlots; ++s)
        {
            const float amt = value ("mod" + String (s + 1) + "_amt");
            auto ab = slotAmt[s].getBounds().toFloat();
            g.setColour (theme->accent.getPerceivedBrightness() > 0.6f && std::abs (amt) > 0.4f ? Colours::black : theme->lcdInk);
            g.setFont (fonts.body (e, 11.0f));
            g.drawText ((amt > 0 ? "+" : "") + String (roundToInt (amt * 100.0f)) + "%", ab, Justification::centred);
        }
    }

private:
    Combo lfoShape[numLfos], lfoSync[numLfos], lfoRetrig[numLfos], lfoUni[numLfos];
    Knob lfoRate[numLfos], lfoFade[numLfos], env[numModEnvs][4];
    Combo slotSrc[numModSlots], slotLayer[numModSlots], slotDst[numModSlots];
    Slider slotAmt[numModSlots];
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> slotAmtAtt[numModSlots];
    Rectangle<float> srcPanel[5], matrix, slotRow[numModSlots];
};

//==============================================================================
// FX page: 3 slots per layer (A, B, C) and 4 master slots
class FxPage : public Page
{
public:
    FxPage (BitEraProcessor& p, const Fonts& f, KnobLookAndFeel& lnf) : Page (p, f, lnf)
    {
        for (int s = 0; s < 4; ++s)
        {
            addCombo (type[s]);
            for (int q = 0; q < 4; ++q) addKnob (knob[s][q], q == 3 ? "MIX" : "-");
        }
        select (4);
    }

    void select (int target)   // 0..3 = layer, 4 = master
    {
        sel = jlimit (0, 4, target);
        const int slots = sel == 4 ? masterFxSlots : layerFxSlots;
        for (int s = 0; s < 4; ++s)
        {
            const bool used = s < slots;
            type[s].box.setVisible (used);
            for (auto& k : knob[s]) k.slider.setVisible (used);
            if (! used) continue;
            const int layer = sel == 4 ? -1 : sel;
            attach (type[s], BitEraProcessor::fxId (layer, s, "type"));
            static const char* ids[4] = { "p1", "p2", "p3", "mix" };
            for (int q = 0; q < 4; ++q) attach (knob[s][q], BitEraProcessor::fxId (layer, s, ids[q]));
        }
        resized();
        repaint();
    }

    void resized() override
    {
        auto b = getLocalBounds().toFloat();
        auto top = b.removeFromTop (40);
        for (int i = 0; i < 5; ++i) tabs[i] = { top.getX() + (float) i * 92.0f, top.getY() + 2, 86, 32 };
        const int slots = sel == 4 ? masterFxSlots : layerFxSlots;
        const float rh = b.getHeight() / (float) slots;
        for (int s = 0; s < slots; ++s)
        {
            auto r = Rectangle<float> (b.getX(), b.getY() + s * rh, b.getWidth(), rh).reduced (0, 3);
            rows[s] = r;
            auto inner = r.reduced (10, 6);
            auto left = inner.removeFromLeft (230);
            type[s].box.setBounds (left.withTrimmedTop (22).withHeight (30).toNearestInt());
            const float kw = inner.getWidth() / 4.0f;
            for (int q = 0; q < 4; ++q) placeKnob (knob[s][q], { inner.getX() + q * kw, inner.getY() - 2, kw, inner.getHeight() + 4 });
        }
    }

    void paint (Graphics& g) override
    {
        if (theme == nullptr) return;
        static const StringArray names { "L1", "L2", "L3", "L4", "MASTER" };
        paintTabs (g, tabs, names, sel);
        const int slots = sel == 4 ? masterFxSlots : layerFxSlots;
        const int layer = sel == 4 ? -1 : sel;
        for (int s = 0; s < slots; ++s)
        {
            paintPanel (g, rows[s]);
            const String slotName = sel == 4 ? "MASTER " + String (s + 1) : "FX " + String::charToString ((juce_wchar) ('A' + s));
            paintHeading (g, slotName, rows[s].reduced (12, 6).withHeight (18));
            const int ft = roundToInt (value (BitEraProcessor::fxId (layer, s, "type")));
            const float alpha = ft == fxOff ? 0.35f : 1.0f;
            for (int q = 0; q < 3; ++q)
            {
                knob[s][q].label = ft == fxOff ? String ("-") : String (fxParamName (ft, q));
                paintKnob (g, knob[s][q], fxParamText (ft, q, value (knob[s][q].id)), alpha);
            }
            paintKnob (g, knob[s][3], paramText (knob[s][3].id), alpha);
        }
        if (sel == 4)
        {
            g.setColour (ink().withAlpha (0.7f));
            g.setFont (fonts.body (e, 11.0f));
            g.drawText ("Signal: layers > layer FX > MASTER 1-4 > TONE > SPACE > VOLUME", getLocalBounds().toFloat().withTrimmedLeft (470).withHeight (40), Justification::centredLeft);
        }
    }

    void mouseUp (const MouseEvent& ev) override
    {
        for (int i = 0; i < 5; ++i) if (tabs[i].contains (ev.position)) { select (i); return; }
    }

private:
    int sel = 4;
    Combo type[4];
    Knob knob[4][4];
    Rectangle<float> tabs[5], rows[4];
};

//==============================================================================
// ARP page: per layer arpeggiator / step sequencer with step lanes
class ArpPage : public Page
{
public:
    ArpPage (BitEraProcessor& p, const Fonts& f, KnobLookAndFeel& lnf) : Page (p, f, lnf)
    {
        for (auto* c : { &mode, &dir, &rate, &end }) addCombo (*c);
        addKnob (oct, "OCTAVES"); addKnob (steps, "STEPS"); addKnob (gate, "GATE"); addKnob (swing, "SWING");
        select (0);
    }

    void select (int l)
    {
        layer = jlimit (0, numLayers - 1, l);
        attach (mode, BitEraProcessor::layerId (layer, "arp"));
        attach (dir, BitEraProcessor::layerId (layer, "arpdir"));
        attach (rate, BitEraProcessor::layerId (layer, "arprate"));
        attach (end, BitEraProcessor::layerId (layer, "arpend"));
        attach (oct, BitEraProcessor::layerId (layer, "arpoct"));
        attach (steps, BitEraProcessor::layerId (layer, "arpsteps"));
        attach (gate, BitEraProcessor::layerId (layer, "arpgate"));
        attach (swing, BitEraProcessor::layerId (layer, "arpswing"));
        repaint();
    }

    void resized() override
    {
        auto b = getLocalBounds().toFloat();
        auto top = b.removeFromTop (40);
        for (int i = 0; i < numLayers; ++i) tabs[i] = { top.getX() + (float) i * 58.0f, top.getY() + 2, 52, 32 };
        auto ctl = top.withTrimmedLeft (4 * 58.0f + 8);
        const float cw = ctl.getWidth() / 4.0f;
        mode.box.setBounds (Rectangle<float> (ctl.getX(), ctl.getY() + 4, cw - 6, 28).toNearestInt());
        dir.box.setBounds (Rectangle<float> (ctl.getX() + cw, ctl.getY() + 4, cw - 6, 28).toNearestInt());
        rate.box.setBounds (Rectangle<float> (ctl.getX() + cw * 2, ctl.getY() + 4, cw - 6, 28).toNearestInt());
        end.box.setBounds (Rectangle<float> (ctl.getX() + cw * 3, ctl.getY() + 4, cw - 6, 28).toNearestInt());
        auto knobsRow = b.removeFromTop (72).reduced (0, 3);
        knobPanel = knobsRow;
        const float kw = knobsRow.getWidth() / 4.0f;
        Knob* ks[4] = { &oct, &steps, &gate, &swing };
        for (int i = 0; i < 4; ++i) placeKnob (*ks[i], { knobsRow.getX() + i * kw, knobsRow.getY() + 2, kw, knobsRow.getHeight() - 4 });
        grid = b.withTrimmedTop (4);
        auto lanesArea = grid.reduced (8, 8).withTrimmedLeft (64);
        static const float h[numLanes] = { 16, 44, 34, 44, 26, 20 };
        float y = lanesArea.getY();
        for (int i = 0; i < numLanes; ++i) { lane[i] = { lanesArea.getX(), y, lanesArea.getWidth(), h[i] }; y += h[i] + 5; }
    }

    void paint (Graphics& g) override
    {
        if (theme == nullptr) return;
        const auto& t = *theme;
        static const StringArray names { "L1", "L2", "L3", "L4" };
        paintTabs (g, tabs, names, layer);
        paintPanel (g, knobPanel);
        paintKnob (g, oct, paramText (oct.id)); paintKnob (g, steps, paramText (steps.id));
        paintKnob (g, gate, paramText (gate.id)); paintKnob (g, swing, paramText (swing.id));
        paintPanel (g, grid);

        const bool seq = roundToInt (value (BitEraProcessor::layerId (layer, "arp"))) == amSeq;
        const bool off = roundToInt (value (BitEraProcessor::layerId (layer, "arp"))) == amOff;
        const int n = roundToInt (value (BitEraProcessor::layerId (layer, "arpsteps")));
        const int cur = proc.currentStep (layer);
        static const char* laneNames[numLanes] = { "ON", "VELOCITY", "GATE", "NOTE", "PAN", "WAVE" };
        const float sw = lane[0].getWidth() / (float) maxSteps;
        for (int li = 0; li < numLanes; ++li)
        {
            auto r = lane[li];
            g.setColour (ink().withAlpha (li == 3 && ! seq ? 0.35f : 0.9f));
            g.setFont (fonts.body (e, 11.0f));
            g.drawText (laneNames[li], r.withX (r.getX() - 64).withWidth (60), Justification::centredRight);
            for (int s = 0; s < maxSteps; ++s)
            {
                auto c = Rectangle<float> (r.getX() + s * sw, r.getY(), sw, r.getHeight()).reduced (1.0f, 0);
                const bool active = s < n;
                const float a = (active ? 1.0f : 0.25f) * (off ? 0.5f : 1.0f);
                g.setColour (t.lcd.withMultipliedAlpha (a));
                g.fillRect (c);
                if (s == cur && active) { g.setColour (t.accent.withAlpha (0.35f)); g.fillRect (c); }
                g.setColour (t.accent.withMultipliedAlpha (a));
                const float v = lanes (li, s);
                switch (li)
                {
                    case 0: if (v > 0.5f) g.fillRect (c.reduced (2)); break;
                    case 1: case 2: g.fillRect (c.withTrimmedTop (c.getHeight() * (1.0f - v))); break;
                    case 3: case 4:
                    {
                        const float norm = li == 3 ? v / 24.0f : v;   // -1..1
                        auto mid = c.getCentreY();
                        if (norm >= 0) g.fillRect (c.withTop (mid - norm * c.getHeight() * 0.5f).withBottom (mid));
                        else g.fillRect (c.withTop (mid).withBottom (mid - norm * c.getHeight() * 0.5f));
                        g.setColour (t.lcdInk.withAlpha (0.3f * a)); g.drawHorizontalLine ((int) mid, c.getX(), c.getRight());
                        if (li == 3 && std::abs (v) > 0.5f && sw > 18) { g.setColour (t.lcdInk.withMultipliedAlpha (a)); g.setFont (fonts.body (e, 9.0f)); g.drawText (String (roundToInt (v)), c, Justification::centredTop); }
                        break;
                    }
                    default:
                    {
                        g.setColour (t.lcdInk.withMultipliedAlpha (a));
                        g.setFont (fonts.body (e, 9.5f));
                        g.drawText (v < 0 ? String ("-") : String (roundToInt (v) + 1), c, Justification::centred);
                        break;
                    }
                }
            }
        }
        g.setColour (ink().withAlpha (0.7f));
        g.setFont (fonts.body (e, 10.5f));
        g.drawText (seq ? "SEQ: every held key plays the step pattern, NOTE = semitones from the key"
                        : "ARP: held keys in the chosen direction. Drag in the lanes to draw. WAVE: click to cycle the layer's waves (- = layer wave)",
                    grid.reduced (10, 4).withTop (grid.getBottom() - 20), Justification::centredLeft);
    }

    void mouseDown (const MouseEvent& ev) override { edit (ev.position, true); }
    void mouseDrag (const MouseEvent& ev) override { edit (ev.position, false); }
    void mouseUp (const MouseEvent& ev) override
    {
        for (int i = 0; i < numLayers; ++i) if (tabs[i].contains (ev.position)) { select (i); return; }
    }

private:
    static constexpr int numLanes = 6;
    float lanes (int li, int s) const
    {
        switch (li)
        {
            case 0: return proc.getStep (layer, "on", s);
            case 1: return proc.getStep (layer, "vel", s);
            case 2: return proc.getStep (layer, "gate", s);
            case 3: return proc.getStep (layer, "note", s);
            case 4: return proc.getStep (layer, "pan", s);
            default: return proc.getStep (layer, "wave", s);
        }
    }

    void edit (Point<float> p, bool down)
    {
        for (int li = 0; li < numLanes; ++li)
        {
            auto r = lane[li];
            if (! r.expanded (0, 3).contains (p) && ! (dragLane == li && ! down)) continue;
            if (down) dragLane = li;
            if (li != dragLane) continue;
            const int s = jlimit (0, maxSteps - 1, (int) ((p.x - r.getX()) / (r.getWidth() / maxSteps)));
            const float y = jlimit (0.0f, 1.0f, 1.0f - (p.y - r.getY()) / r.getHeight());
            switch (li)
            {
                case 0: if (down) { drawOn = proc.getStep (layer, "on", s) < 0.5f; } proc.setStep (layer, "on", s, drawOn ? 1.0f : 0.0f); break;
                case 1: proc.setStep (layer, "vel", s, jmax (0.05f, y)); break;
                case 2: proc.setStep (layer, "gate", s, jmax (0.05f, y)); break;
                case 3: proc.setStep (layer, "note", s, (float) roundToInt ((y * 2.0f - 1.0f) * 24.0f)); break;
                case 4: proc.setStep (layer, "pan", s, y * 2.0f - 1.0f); break;
                default:
                    if (down)
                    {
                        const int src = roundToInt (value (BitEraProcessor::layerId (layer, "src")));
                        const int w = roundToInt (proc.getStep (layer, "wave", s)) + 1;
                        proc.setStep (layer, "wave", s, w >= chips::numWaves (src) ? -1.0f : (float) w);
                    }
                    break;
            }
            repaint();
            return;
        }
    }

    int layer = 0, dragLane = -1;
    bool drawOn = true;
    Combo mode, dir, rate, end;
    Knob oct, steps, gate, swing;
    Rectangle<float> tabs[numLayers], knobPanel, grid, lane[numLanes];
};

//==============================================================================
// MACRO page: 8 macros and 2 XY pads, with what each one is routed to
class MacroPage : public Page
{
public:
    MacroPage (BitEraProcessor& p, const Fonts& f, KnobLookAndFeel& lnf) : Page (p, f, lnf)
    {
        for (int m = 0; m < numMacros; ++m) { addKnob (macro[m], "MACRO " + String (m + 1)); attach (macro[m], "macro" + String (m + 1)); }
    }

    void resized() override
    {
        auto b = getLocalBounds().toFloat();
        auto left = b.removeFromLeft (b.getWidth() * 0.56f);
        knobArea = left;
        auto inner = left.reduced (8);
        const float w = inner.getWidth() / 4.0f, h = inner.getHeight() / 2.0f;
        for (int m = 0; m < numMacros; ++m)
            placeKnob (macro[m], Rectangle<float> (inner.getX() + (m % 4) * w, inner.getY() + (m / 4) * h, w, h * 0.72f));
        auto right = b.withTrimmedLeft (8);
        const float padSize = jmin (right.getWidth() / 2.0f - 6.0f, right.getHeight() - 60.0f);
        for (int k = 0; k < 2; ++k) pad[k] = { right.getX() + k * (padSize + 12.0f), right.getY() + 26, padSize, padSize };
        padPanel = right;
    }

    void paint (Graphics& g) override
    {
        if (theme == nullptr) return;
        const auto& t = *theme;
        paintPanel (g, knobArea);
        for (int m = 0; m < numMacros; ++m)
        {
            paintKnob (g, macro[m], paramText (macro[m].id));
            g.setColour (ink().withAlpha (0.75f));
            g.setFont (fonts.body (e, 10.0f, false));
            g.drawFittedText (routes (msMacro1 + m), macro[m].area.withY (macro[m].area.getBottom() + 2).withHeight (40).toNearestInt(), Justification::centredTop, 3);
        }
        paintPanel (g, padPanel);
        for (int k = 0; k < 2; ++k)
        {
            paintHeading (g, "XY " + String (k + 1), pad[k].withY (padPanel.getY() + 4).withHeight (18));
            g.setColour (t.lcd);
            g.fillRoundedRectangle (pad[k], 6.0f);
            g.setColour (t.lcdInk.withAlpha (0.2f));
            for (int i = 1; i < 4; ++i)
            {
                g.drawVerticalLine ((int) (pad[k].getX() + pad[k].getWidth() * i / 4), pad[k].getY(), pad[k].getBottom());
                g.drawHorizontalLine ((int) (pad[k].getY() + pad[k].getHeight() * i / 4), pad[k].getX(), pad[k].getRight());
            }
            const float x = value ("xy" + String (k + 1) + "x"), y = value ("xy" + String (k + 1) + "y");
            auto c = Point<float> (pad[k].getX() + x * pad[k].getWidth(), pad[k].getBottom() - y * pad[k].getHeight());
            g.setColour (t.accent);
            g.fillEllipse (Rectangle<float> (18, 18).withCentre (c));
            g.setColour (t.lcdInk);
            g.drawEllipse (Rectangle<float> (18, 18).withCentre (c), 1.5f);
            g.setColour (ink().withAlpha (0.75f));
            g.setFont (fonts.body (e, 10.0f, false));
            g.drawFittedText ("X: " + routes (msXY1X + 2 * k) + "\nY: " + routes (msXY1Y + 2 * k),
                              pad[k].withY (pad[k].getBottom() + 4).withHeight (padPanel.getBottom() - pad[k].getBottom() - 6).toNearestInt(), Justification::topLeft, 4);
        }
    }

    void mouseDown (const MouseEvent& ev) override { drag (ev.position); }
    void mouseDrag (const MouseEvent& ev) override { drag (ev.position); }

private:
    String routes (int src) const
    {
        StringArray out;
        for (int s = 0; s < numModSlots; ++s)
        {
            const String n = "mod" + String (s + 1) + "_";
            if (roundToInt (value (n + "src")) != src || std::abs (value (n + "amt")) < 0.001f) continue;
            const int d = roundToInt (value (n + "dst")), l = roundToInt (value (n + "layer"));
            out.add (String (modDestName (d)) + (d < mdVolume ? (l == 0 ? "" : " L" + String (l)) : ""));
        }
        return out.isEmpty() ? String ("not routed (MOD page)") : out.joinIntoString (", ");
    }

    void drag (Point<float> p)
    {
        for (int k = 0; k < 2; ++k)
            if (pad[k].expanded (6).contains (p))
            {
                setValue ("xy" + String (k + 1) + "x", jlimit (0.0f, 1.0f, (p.x - pad[k].getX()) / pad[k].getWidth()));
                setValue ("xy" + String (k + 1) + "y", jlimit (0.0f, 1.0f, (pad[k].getBottom() - p.y) / pad[k].getHeight()));
                repaint();
            }
    }

    Knob macro[numMacros];
    Rectangle<float> knobArea, padPanel, pad[2];
};

//==============================================================================
// favourites, shared by every instance (user settings file)
struct Favourites
{
    static PropertiesFile& file()
    {
        static PropertiesFile f ([] { PropertiesFile::Options o; o.applicationName = "Bit Era"; o.filenameSuffix = ".settings";
                                      o.folderName = "TrapWin/Bit Era"; o.osxLibrarySubFolder = "Application Support"; return o; }());
        return f;
    }
    static bool is (int preset) { return file().getValue ("favourites").contains ("," + String (preset) + ","); }
    static void toggle (int preset)
    {
        String v = file().getValue ("favourites", ",");
        const String key = "," + String (preset) + ",";
        v = v.contains (key) ? v.replace (key, ",") : v + String (preset) + ",";
        file().setValue ("favourites", v);
        file().saveIfNeeded();
    }
};

//==============================================================================
// BROWSE page: search the 7,200 presets by name, era, category, type and favourites
class BrowsePage : public Page, private ListBoxModel
{
public:
    BrowsePage (BitEraProcessor& p, const Fonts& f, KnobLookAndFeel& lnf) : Page (p, f, lnf)
    {
        search.setTextToShowWhenEmpty ("Search 7,200 sounds...", Colours::grey);
        search.onTextChange = [this] { refilter(); };
        addAndMakeVisible (search);
        eraBox.addItem ("ALL ERAS", 1);
        for (int i = 0; i < numEras; ++i) eraBox.addItem (String (era (i).year) + "  " + era (i).shortName, i + 2);
        typeBox.addItem ("ALL TYPES", 1);
        for (int t = 0; t < numPresetTypes; ++t) typeBox.addItem (presetTypeName (t), t + 2);
        for (auto* c : { &eraBox, &typeBox })
        {
            c->setSelectedId (1, dontSendNotification);
            c->onChange = [this] { refilter(); };
            c->setJustificationType (Justification::centredLeft);
            addAndMakeVisible (*c);
            allCombos.push_back (c);
        }
        list.setModel (this);
        list.setRowHeight (24);
        addAndMakeVisible (list);
        refilter();
    }

    ~BrowsePage() override { list.setModel (nullptr); for (auto* c : { &eraBox, &typeBox }) c->setLookAndFeel (nullptr); }

    void showEra (int eraIndex)   // opening the page: the current era, the current preset selected
    {
        eraBox.setSelectedId (eraIndex + 2, sendNotificationSync);
        selectCurrent();
        list.grabKeyboardFocus();
    }

    void followEra (int eraIndex)   // the era changed elsewhere (timeline): an era filter follows it
    {
        if (eraBox.getSelectedId() != 1 && eraBox.getSelectedId() != eraIndex + 2)
        {
            eraBox.setSelectedId (eraIndex + 2, sendNotificationSync);
            selectCurrent();
        }
    }

    void step (int d)   // arrow keys: next / previous sound in the list, played right away
    {
        if (results.empty()) return;
        const int row = jlimit (0, (int) results.size() - 1, list.getSelectedRow() + d);
        list.selectRow (row);
    }

    void selectCurrent()
    {
        const ScopedValueSetter<bool> svs (syncing, true);
        auto it = std::find (results.begin(), results.end(), proc.getPreset());
        if (it != results.end()) list.selectRow ((int) (it - results.begin()));
        else list.deselectAllRows();
    }

    void resized() override
    {
        auto b = getLocalBounds().toFloat();
        auto top = b.removeFromTop (36);
        search.setBounds (top.removeFromLeft (250).reduced (0, 3).toNearestInt());
        top.removeFromLeft (8);
        eraBox.setBounds (top.removeFromLeft (170).reduced (0, 3).toNearestInt());
        top.removeFromLeft (8);
        typeBox.setBounds (top.removeFromLeft (130).reduced (0, 3).toNearestInt());
        top.removeFromLeft (8);
        favBtn = top.removeFromLeft (top.getWidth()).reduced (0, 3);
        b.removeFromTop (6);
        catArea = b.removeFromLeft (150);
        b.removeFromLeft (8);
        list.setBounds (b.withTrimmedBottom (18).toNearestInt());
    }

    void paint (Graphics& g) override
    {
        if (theme == nullptr) return;
        const auto& t = *theme;
        search.setColour (TextEditor::backgroundColourId, t.lcd);
        search.setColour (TextEditor::textColourId, t.lcdInk);
        search.setColour (TextEditor::outlineColourId, t.lcdInk.withAlpha (0.4f));
        search.setColour (TextEditor::focusedOutlineColourId, t.accent);
        search.setColour (CaretComponent::caretColourId, t.lcdInk);
        list.setColour (ListBox::backgroundColourId, panelFill (e, t));
        list.getVerticalScrollBar().setColour (ScrollBar::thumbColourId, t.accent.withAlpha (0.7f));

        g.setColour (onlyFav ? t.accent : panelFill (e, t));
        g.fillRoundedRectangle (favBtn, 4.0f);
        g.setColour (onlyFav ? (t.accent.getPerceivedBrightness() > 0.6f ? Colours::black : Colours::white) : ink());
        g.setFont (fonts.body (e, 13.0f));
        g.drawText (String::fromUTF8 ("\xe2\x98\x85 FAVOURITES"), favBtn, Justification::centred);

        paintPanel (g, catArea);
        const float rh = (catArea.getHeight() - 8.0f) / (float) (numCategories + 1);
        for (int c = -1; c < numCategories; ++c)
        {
            auto r = Rectangle<float> (catArea.getX() + 4, catArea.getY() + 4 + (c + 1) * rh, catArea.getWidth() - 8, rh);
            const bool sel = c == category;
            if (sel) { g.setColour (t.accent); g.fillRoundedRectangle (r.reduced (0, 1), 3.0f); }
            g.setColour (sel ? (t.accent.getPerceivedBrightness() > 0.6f ? Colours::black : Colours::white) : ink());
            g.setFont (fonts.body (e, 12.0f));
            g.drawText (c < 0 ? String ("ALL") : String (categoryName (c)), r.withTrimmedLeft (8), Justification::centredLeft);
            g.setFont (fonts.body (e, 10.5f, false));
            g.drawText (String (countFor (c)), r.withTrimmedRight (8), Justification::centredRight);
        }
        g.setColour (ink().withAlpha (0.8f));
        g.setFont (fonts.body (e, 11.0f));
        g.drawText (String (results.size()) + " sounds   -   click or arrow keys to play, space to stop, star to keep",
                    list.getBounds().toFloat().withY ((float) list.getBottom() + 1).withHeight (16), Justification::centredLeft);
    }

    void mouseUp (const MouseEvent& ev) override
    {
        if (favBtn.contains (ev.position)) { onlyFav = ! onlyFav; refilter(); return; }
        if (catArea.contains (ev.position))
        {
            const float rh = (catArea.getHeight() - 8.0f) / (float) (numCategories + 1);
            category = jlimit (-1, numCategories - 1, (int) ((ev.position.y - catArea.getY() - 4) / rh) - 1);
            refilter();
        }
    }

private:
    int getNumRows() override { return (int) results.size(); }

    void paintListBoxItem (int row, Graphics& g, int w, int h, bool selected) override
    {
        if (theme == nullptr || row < 0 || row >= (int) results.size()) return;
        const auto& t = *theme;
        const int p = results[(size_t) row];
        const auto& info = presetInfo (p);
        const bool current = p == proc.getPreset();
        if (current || selected) { g.setColour (t.accent.withAlpha (current ? 0.9f : 0.35f)); g.fillRect (0, 0, w, h); }
        const Colour txt = current ? (t.accent.getPerceivedBrightness() > 0.6f ? Colours::black : Colours::white) : panelLabel (e, t);
        g.setColour (Favourites::is (p) ? (current ? txt : t.accent) : txt.withAlpha (0.35f));
        g.setFont (Font (FontOptions (15.0f)));
        g.drawText (String::fromUTF8 (Favourites::is (p) ? "\xe2\x98\x85" : "\xe2\x98\x86"), 4, 0, 20, h, Justification::centred);
        g.setColour (txt);
        g.setFont (fonts.body (e, 13.0f));
        g.drawText (info.name, 28, 0, w - 250, h, Justification::centredLeft);
        g.setFont (fonts.body (e, 11.0f, false));
        g.setColour (txt.withAlpha (0.8f));
        g.drawText (String (era (info.era).year), w - 220, 0, 50, h, Justification::centredLeft);
        g.drawText (categoryName (info.category), w - 170, 0, 80, h, Justification::centredLeft);
        g.drawText (presetTypeName (info.type), w - 88, 0, 82, h, Justification::centredLeft);
    }

    // one click (or an arrow key) selects the sound: it loads and plays at once
    void selectedRowsChanged (int row) override
    {
        if (syncing || row < 0 || row >= (int) results.size()) return;
        proc.previewPreset (results[(size_t) row]);
        list.repaint();
    }

    void listBoxItemClicked (int row, const MouseEvent& ev) override
    {
        if (row < 0 || row >= (int) results.size()) return;
        const int p = results[(size_t) row];
        if (ev.x < 26) { Favourites::toggle (p); if (onlyFav) refilter(); list.repaint(); return; }
        if (p == proc.getPreset() && proc.previewingPreset() < 0) proc.previewPreset (p);   // click again: play again
    }

    int countFor (int c) const
    {
        int n = 0;
        for (int p : base) if (c < 0 || presetInfo (p).category == c) ++n;
        return n;
    }

    void refilter()
    {
        const String q = search.getText().trim().toLowerCase();
        const int eraSel = eraBox.getSelectedId() - 2, typeSel = typeBox.getSelectedId() - 2;
        base.clear(); results.clear();
        for (int p = 0; p < numPresets; ++p)
        {
            const auto& info = presetInfo (p);
            if (eraSel >= 0 && info.era != eraSel) continue;
            if (typeSel >= 0 && info.type != typeSel) continue;
            if (onlyFav && ! Favourites::is (p)) continue;
            if (q.isNotEmpty() && ! info.name.toLowerCase().contains (q) && ! String (categoryName (info.category)).toLowerCase().contains (q)
                && ! String (era (info.era).year).contains (q))
                continue;
            base.push_back (p);
            if (category < 0 || info.category == category) results.push_back (p);
        }
        list.updateContent();
        selectCurrent();
        list.repaint();
        repaint();
    }

    void timerCallback() override { if (isShowing()) list.repaint(); }

    TextEditor search;
    ComboBox eraBox, typeBox;
    ListBox list;
    Rectangle<float> favBtn, catArea;
    std::vector<int> base, results;
    int category = -1;
    bool onlyFav = false, syncing = false;
};

//==============================================================================
class Content : public Component, private Timer
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

        const char* ids[6] = { "volume", "l1_attack", "l1_decay", "l1_release", "tone", "space" };
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

        layerPage = std::make_unique<LayerPage> (proc, fonts, knobLnf);
        addChildComponent (*layerPage);
        pages[0] = std::make_unique<ModPage> (proc, fonts, knobLnf);
        pages[1] = std::make_unique<FxPage> (proc, fonts, knobLnf);
        pages[2] = std::make_unique<ArpPage> (proc, fonts, knobLnf);
        pages[3] = std::make_unique<MacroPage> (proc, fonts, knobLnf);
        pages[4] = std::make_unique<BrowsePage> (proc, fonts, knobLnf);
        for (auto& pg : pages) addChildComponent (*pg);
        lastPreset = proc.getPreset();
        setWantsKeyboardFocus (true);
        showIntro = ! proc.introSeen();
        layout();
        updateTheme();
        startTimerHz (20);
    }

    ~Content() override
    {
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
        if (page == 0)
        {
            paintTitle (g, e, t);
            paintRows (g, e, t);
            paintKnobPanel (g, e, t);
        }
        else
            paintPageTitle (g, e, t);
        if (! showIntro)
            paintTabs (g, e, t);
        paintStrip (g, e);
        if (showIntro)
            paintIntro (g);
    }

    void setPage (int p) { page = jlimit (0, numPages - 1, p); updateTheme(); }

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
        grabKeyboardFocus();   // space / arrow keys go to Bit Era while you work in it
    }

    void mouseDrag (const MouseEvent& ev) override
    {
        if (dragStarted || ev.getDistanceFromDragStart() < 6)
            return;
        if (downHit.kind == Hit::drag)
            startWavDrag (proc.getPreset(), true);
        else if (downHit.kind == Hit::rowDrag || downHit.kind == Hit::row || downHit.kind == Hit::rowPlay)
            startWavDrag (rowPreset (downHit.index));
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
                proc.setPreset (hit.index * presetsPerEra);
                followEraInBrowser();
                break;
            case Hit::prev: stepPreset (-1); break;
            case Hit::next: stepPreset (1); break;
            case Hit::rowPlay:
            case Hit::row:
            {
                const int preset = rowPreset (hit.index);
                if (proc.previewingPreset() == preset && hit.kind == Hit::rowPlay) proc.stopPreview();
                else proc.previewPreset (preset);
                repaint();
                break;
            }
            case Hit::pagePrev: stepPage (-1); break;
            case Hit::pageNext: stepPage (1); break;
            case Hit::tab:
                if (hit.index == 6) if (auto* bp = dynamic_cast<BrowsePage*> (pages[4].get())) bp->showEra (currentEra());
                grabKeyboardFocus();
                page = hit.index;
                updateTheme();
                break;
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
        enum Kind { none, start, era, prev, next, row, rowPlay, rowDrag, drag, logo, tab, pagePrev, pageNext } kind = none;
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
    static constexpr int numPages = 7;
    std::unique_ptr<Page> pages[5];
    int page = 0, lastPreset = -1;
    std::unique_ptr<LayerPage> layerPage;
    Rectangle<float> pageArea { 332, 120, 748, 388 };

    Rectangle<float> tabArea (int i) const { return { 570.0f + (float) i * 73.0f, 84.0f, 70.0f, 28.0f }; }

    void paintTabs (Graphics& g, int e, const Theme& t)
    {
        static const char* names[numPages] = { "PLAY", "LAYERS", "MOD", "FX", "ARP", "MACRO", "BROWSE" };
        for (int i = 0; i < numPages; ++i)
        {
            auto r = tabArea (i);
            const bool sel = i == page;
            g.setColour (sel ? t.accent : t.lcd);
            if (t.frame == FrameStyle::pixel || t.frame == FrameStyle::flat) g.fillRect (r); else g.fillRoundedRectangle (r, jmin (t.corner, 6.0f));
            g.setColour (sel ? (t.accent.getPerceivedBrightness() > 0.6f ? Colours::black : Colours::white) : t.lcdInk);
            g.setFont (fonts.body (e, 12.5f));
            g.drawText (names[i], r, Justification::centred);
        }
    }

    void paintPageTitle (Graphics& g, int e, const Theme& t)
    {
        const auto& E = era (e);
        g.setColour (e == 6 || e == 4 || e == 11 ? Colours::white : t.ink);
        g.setFont (fonts.display (e, 26.0f));
        const String a = String (E.title).upToFirstOccurrenceOf ("\n", false, false), b = String (E.title).fromFirstOccurrenceOf ("\n", false, false);
        const String title = b.isEmpty() ? a : a + (CharacterFunctions::isLowerCase (b[0]) ? "" : " ") + b;
        g.drawFittedText (title, Rectangle<float> (332, 82, 230, 32).toNearestInt(), Justification::centredLeft, 1, 0.6f);
    }
    Hit downHit;
    int lastPreviewing = -1, lastEra = -1;

    int currentEra() const { return eraOfPreset (proc.getPreset()); }
    // the PLAY page lists the presets 6 at a time: the block of 6 that holds the current preset
    int rowPreset (int i) const { return (proc.getPreset() / soundsPerEra) * soundsPerEra + i; }
    Rectangle<float> pagePrevBtn { 820, 204, 26, 24 }, pageNextBtn { 946, 204, 26, 24 };
    void stepPage (int d)
    {
        const int cur = proc.getPreset(), e = eraOfPreset (cur);
        const int pagesInEra = presetsPerEra / soundsPerEra;
        const int pg = ((cur % presetsPerEra) / soundsPerEra + d + pagesInEra) % pagesInEra;
        proc.setPreset (e * presetsPerEra + pg * soundsPerEra);
    }

    void layout()
    {
        layerPage->setBounds (pageArea.toNearestInt());
        for (auto& pg : pages) pg->setBounds (pageArea.toNearestInt());
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
            k.slider.setVisible (! showIntro && page == 0);
        layerPage->setTheme (&era (currentEra()).theme, currentEra());
        layerPage->setVisible (! showIntro && page == 1);
        for (int i = 0; i < 5; ++i)
        {
            pages[i]->setTheme (&era (currentEra()).theme, currentEra());
            pages[i]->setVisible (! showIntro && page == i + 2);
        }
        repaint();
    }


    void timerCallback() override
    {
        if (proc.getPreset() != lastPreset)
        {
            lastPreset = proc.getPreset();
            updateTheme();
        }
        for (auto& k : knobs)
            if (k.slider.isVisible() != (! showIntro && page == 0))
                k.slider.setVisible (! showIntro && page == 0);
        if (proc.previewingPreset() != lastPreviewing) { lastPreviewing = proc.previewingPreset(); repaint(); }
        if (currentEra() != lastEra) { lastEra = currentEra(); followEraInBrowser(); }
    }

    void followEraInBrowser()
    {
        if (auto* bp = dynamic_cast<BrowsePage*> (pages[4].get())) bp->followEra (currentEra());
    }

    // next / previous preset: loads it and plays it right away
    void stepPreset (int d)
    {
        proc.previewPreset ((proc.getPreset() + d + numPresets) % numPresets);
    }

    bool keyPressed (const KeyPress& key) override
    {
        if (showIntro) return false;
        if (key == KeyPress::spaceKey)
        {
            if (proc.previewingPreset() >= 0) proc.stopPreview();
            else proc.previewPreset (proc.getPreset());
            return true;
        }
        if (page == 6)
            if (auto* bp = dynamic_cast<BrowsePage*> (pages[4].get()))
            {
                if (key == KeyPress::downKey) { bp->step (1); return true; }
                if (key == KeyPress::upKey) { bp->step (-1); return true; }
            }
        if (page == 0)
        {
            if (key == KeyPress::downKey || key == KeyPress::rightKey) { stepPreset (1); return true; }
            if (key == KeyPress::upKey || key == KeyPress::leftKey) { stepPreset (-1); return true; }
        }
        if (key == KeyPress::pageDownKey) { proc.previewPreset ((currentEra() + 1) % numEras * presetsPerEra); return true; }
        if (key == KeyPress::pageUpKey) { proc.previewPreset ((currentEra() + numEras - 1) % numEras * presetsPerEra); return true; }
        return false;
    }

    Hit hitTestArea (Point<float> p) const
    {
        if (showIntro)
            return startBtn.contains (p) ? Hit { Hit::start, 0 } : Hit {};
        if (logoArea.contains (p)) return { Hit::logo, 0 };
        if (display.contains (p)) return { p.x < display.getCentreX() ? Hit::prev : Hit::next, 0 };
        if (dragBtn.contains (p)) return { Hit::drag, 0 };
        for (int i = 0; i < numPages; ++i)
            if (tabArea (i).contains (p)) return { Hit::tab, i };
        if (page != 0 && ! strip.contains (p)) return {};
        if (page == 0 && pagePrevBtn.contains (p)) return { Hit::pagePrev, 0 };
        if (page == 0 && pageNextBtn.contains (p)) return { Hit::pageNext, 0 };
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

    // DRAG WAV renders what the instrument plays now (layers + master); a row drags that era sound as it is
    void startWavDrag (int preset, bool renderCurrent = false)
    {
        MemoryBlock mb;
        if (! renderCurrent && ! proc.getSoundWav (preset, mb))
        {
            // designed presets are rendered: load it, then render
            if (proc.getPreset() != preset) proc.setPreset (preset);
            renderCurrent = true;
        }
        if (renderCurrent && ! proc.renderCurrentWav (mb))
            return;
        auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("Bit Era");
        dir.createDirectory();
        const auto& E = era (eraOfPreset (preset));
        auto f = dir.getChildFile (File::createLegalFileName (String ("Bit Era ") + E.year + " " + presetInfo (preset).name + ".wav"));
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
        g.drawText (String (era (e).year) + String::fromUTF8 ("  \xc2\xb7  ") + categoryName (presetInfo (preset).category) + String::fromUTF8 ("  \xc2\xb7  ")
                    + String (preset % presetsPerEra + 1) + " / " + String (presetsPerEra), d.withHeight (18).translated (0, 4), Justification::centred);
        g.setColour (t.lcdInk);
        g.setFont (fonts.display (e, e == 5 ? 13.0f : 19.0f));
        g.drawText (presetInfo (preset).name, d.withTrimmedTop (16).reduced (36, 0), Justification::centred);

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
        {
            // page navigation: 6 presets per page, 100 pages per era
            const int first = (preset % presetsPerEra) / soundsPerEra * soundsPerEra;
            g.setColour (t.lcd);
            for (auto r : { pagePrevBtn, pageNextBtn }) g.fillRoundedRectangle (r, 3.0f);
            g.setColour (t.lcdInk);
            g.setFont (fonts.body (e, 13.0f));
            g.drawText ("<", pagePrevBtn, Justification::centred);
            g.drawText (">", pageNextBtn, Justification::centred);
            g.setColour (e == 6 || e == 4 || e == 11 ? Colours::white : t.ink);
            g.setFont (fonts.body (e, 12.0f));
            g.drawText (String (first + 1) + "-" + String (first + soundsPerEra) + " / " + String (presetsPerEra),
                        Rectangle<float> (pagePrevBtn.getRight(), pagePrevBtn.getY(), pageNextBtn.getX() - pagePrevBtn.getRight(), pagePrevBtn.getHeight()), Justification::centred);
        }
        for (int i = 0; i < soundsPerEra; ++i)
        {
            const int p = rowPreset (i);
            const bool sel = p == preset, on = p == proc.previewingPreset();
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
            g.drawText (presetInfo (p).name, textR.withHeight (r.getHeight() * 0.6f), Justification::bottomLeft);
            g.setColour ((e == 6 || e == 4 || e == 11) ? Colours::white.withAlpha (0.8f) : t.ink.withAlpha (0.72f));
            g.setFont (fonts.body (e, 12.0f));
            g.drawText (String (categoryName (presetInfo (p).category)) + String::fromUTF8 ("  \xc2\xb7  ") + presetTypeName (presetInfo (p).type),
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

void BitEraEditor::showPage (int page) { content->setPage (page); }

void BitEraEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colours::black); }

void BitEraEditor::resized()
{
    const float scale = (float) getWidth() / (float) designWidth;
    content->setTransform (juce::AffineTransform::scale (scale));
}
