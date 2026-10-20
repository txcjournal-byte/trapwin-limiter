#pragma once
#include <juce_graphics/juce_graphics.h>

// The 12 eras of BIT ERA: content (title, sounds, cover) and the visual skin of each era.
namespace bitera
{
enum class KnobStyle { metal, brass, bakelite, soft, chamfer, pixel, neon, flat, gloss };
enum class FrameStyle { film, photo, crt, retro, chamfer, pixel, neon, polaroid, gloss, card, flat, trap };

struct SoundInfo
{
    const char* name;
    const char* category;
    const char* resource;   // original file name in BinaryData
    int root = 60;          // MIDI key that plays the sound at its recorded pitch
};

struct Theme
{
    juce::Colour bgTop, bgBottom;   // window background gradient
    juce::Colour header;            // header band
    juce::Colour ink, muted;        // text
    juce::Colour accent, accent2;   // main highlight, second highlight
    juce::Colour panel, panelEdge;  // knob panel / sound rows
    juce::Colour lcd, lcdInk;       // preset display
    juce::Colour dragBg, dragInk;   // DRAG WAV button
    juce::Colour headerInk;         // logo / text on the header band
    KnobStyle knob;
    FrameStyle frame;
    float corner;                   // corner radius of panels / rows
    bool serifTitle, monoBody, upperTitle, scanlines, stars, grid, darkCover;
};

struct Era
{
    const char* ep;
    const char* year;
    const char* shortName;
    const char* title;   // two lines separated by '\n'
    const char* line;
    const char* cover;   // original file name in BinaryData
    SoundInfo sounds[6];   // 0-2: one-shots (single note / single hit), 3-5: phrases, loops, FX
    Theme theme;
};

constexpr int numEras = 12;
constexpr int soundsPerEra = 6;
constexpr int numPresets = numEras * soundsPerEra;

const Era& era (int index);
int eraOfPreset (int preset);
const SoundInfo& sound (int preset);
} // namespace bitera
