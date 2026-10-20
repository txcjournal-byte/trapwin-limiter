#include "Eras.h"

namespace bitera
{
namespace
{
juce::Colour c (juce::uint32 rgb) { return juce::Colour (0xff000000u | rgb); }
juce::Colour ca (juce::uint32 argb) { return juce::Colour (argb); }

// skins follow the canvas design "Bit Era – návrhy UI" (one visual world per era, light and dark mixed)
const Era eras[numEras] = {
    { "EP01", "1917", "Silent", "Silent\nLegends", "Paper, ink and a flickering projector.", "e01.jpg",
      { { "Reel Upright", "KEYS", "e01_o1.wav", 60 }, { "Music Box Note", "BELLS", "e01_o2.wav", 72 }, { "Pit Double Bass", "BASS", "e01_o3.wav", 36 },
        { "Silent Piano Reel", "PHRASE", "e01_s1.wav" }, { "Music Box Moon", "PHRASE", "e01_s2.wav" }, { "Projector Flicker", "FX", "e01_s3.wav" } },
      { c (0xefe3c6), c (0xc9b48a), c (0xe2d2ad), c (0x2a1d10), c (0x6b5435), c (0x8b2e16), c (0xc99a3a),
        ca (0x8cfff8e4), c (0x2a1d10), c (0x2a1d10), c (0xefe3c6), c (0x8b2e16), c (0xefe3c6), c (0x2a1d10),
        KnobStyle::brass, FrameStyle::film, 0.0f, true, true, false, false, false, false, false } },
    { "EP02", "1958", "Color film", "First\nColor", "Watercolor skies and a full orchestra.", "e02.jpg",
      { { "Celesta", "BELLS", "e02_o1.wav", 72 }, { "Harp Pluck", "PLUCK", "e02_o2.wav", 60 }, { "Film Violins", "STRINGS", "e02_o3.wav", 60 },
        { "Celesta Dream", "PHRASE", "e02_s1.wav" }, { "Harp Glissando", "PHRASE", "e02_s2.wav" }, { "Film Strings", "PHRASE", "e02_s3.wav" } },
      { c (0xf7eedd), c (0xefdcc0), c (0xf7eedd), c (0x3a2a22), c (0x7d6656), c (0xc8452f), c (0x2f8a86),
        c (0xfffaf0), ca (0x1a3a2a22), c (0x2f8a86), c (0xfffaf0), c (0xc8452f), c (0xfffaf0), c (0xc8452f),
        KnobStyle::soft, FrameStyle::photo, 18.0f, false, false, false, false, false, false, false } },
    { "EP03", "1963", "B&W TV", "Black &\nWhite TV", "Tape echo, oscillators and rockets.", "e03.jpg",
      { { "Tape Beep", "LEAD", "e03_o1.wav", 72 }, { "Valve Oscillator", "LEAD", "e03_o2.wav", 60 }, { "Tube Bass", "BASS", "e03_o3.wav", 36 },
        { "Tape Rocket Beep", "PHRASE", "e03_s1.wav" }, { "Oscillator Siren", "FX", "e03_s2.wav" }, { "Rocket Launch", "FX", "e03_s3.wav" } },
      { c (0xdcdcd7), c (0xc4c4be), c (0x111111), c (0x111111), c (0x555555), c (0x111111), c (0x777777),
        c (0xf2f2ee), c (0x111111), c (0xf2f2ee), c (0x111111), c (0x111111), c (0xf2f2ee), c (0xf2f2ee),
        KnobStyle::bakelite, FrameStyle::crt, 8.0f, false, true, true, true, false, false, false } },
    { "EP04", "1972", "Grit", "Blood &\nRain", "Raw bleeps from bare circuits.", "e04.jpg",
      { { "TTL Square Bass", "BASS", "e04_o1.wav", 36 }, { "Paddle Blip", "FX", "e04_o2.wav", 84 }, { "TTL Noise Hit", "DRUMS", "e04_o3.wav", 60 },
        { "Marching Bass", "LOOP", "e04_s1.wav" }, { "Laser Shot", "FX", "e04_s2.wav" }, { "Explosion", "FX", "e04_s3.wav" } },
      { c (0xf3e2bf), c (0xe9cf9c), c (0x3b1f0e), c (0x3b1f0e), c (0x8a5a33), c (0xe0601f), c (0xf0a32b),
        c (0xfff3dc), c (0x3b1f0e), c (0x3b1f0e), c (0xf0a32b), c (0xe0601f), c (0xfff3dc), c (0xf0a32b),
        KnobStyle::soft, FrameStyle::retro, 22.0f, false, false, false, false, false, false, false } },
    { "EP05", "1979", "Space robots", "Steel\nGiants", "Brass, analog synths and laser beams.", "e05.jpg",
      { { "Robot Brass", "BRASS", "e05_o1.wav", 60 }, { "Space Lead", "LEAD", "e05_o2.wav", 60 }, { "Analog Bass", "BASS", "e05_o3.wav", 36 },
        { "Robot Fanfare", "PHRASE", "e05_s1.wav" }, { "Space Synth Lead", "PHRASE", "e05_s2.wav" }, { "Beam Cannon", "FX", "e05_s3.wav" } },
      { c (0x23387a), c (0x050816), c (0x121c48), c (0xf2f4ff), c (0x9fb0e8), c (0xff7a2f), c (0xffd23a),
        ca (0xd9121c48), c (0x3d5aa8), c (0x000000), c (0xffb000), c (0xff7a2f), c (0x050816), c (0xffd23a),
        KnobStyle::chamfer, FrameStyle::chamfer, 4.0f, false, false, true, false, true, false, true } },
    { "EP06", "1985", "8-bit", "Wasteland\nWarriors", "Five channels. Nothing more.", "e06.jpg",
      { { "Pulse Lead", "LEAD", "e06_o1.wav", 72 }, { "Triangle Sub", "BASS", "e06_o2.wav", 36 }, { "Chip Snare", "DRUMS", "e06_o3.wav", 60 },
        { "8-Bit Hero Lead", "PHRASE", "e06_s1.wav" }, { "Triangle Bass", "LOOP", "e06_s2.wav" }, { "Noise Kit", "LOOP", "e06_s3.wav" } },
      { c (0xc4c4c4), c (0xb0b0b0), c (0x3a3a3a), c (0x141414), c (0x4a4a4a), c (0xd0201f), c (0x141414),
        c (0xe6e6e6), c (0x141414), c (0x141414), c (0xf8f8f8), c (0xd0201f), c (0xf8f8f8), c (0xd0201f),
        KnobStyle::pixel, FrameStyle::pixel, 0.0f, false, true, true, false, false, true, false } },
    { "EP07", "1988", "Neon OVA", "Neon\nMegacity", "Metal FM under neon rain.", "e07.jpg",
      { { "FM Steel Bass", "BASS", "e07_o1.wav", 36 }, { "FM Brass Chord", "BRASS", "e07_o2.wav", 60 }, { "FM Bell", "BELLS", "e07_o3.wav", 72 },
        { "FM Neon Steel Bass", "LOOP", "e07_s1.wav" }, { "FM Brass Stab", "PHRASE", "e07_s2.wav" }, { "FM Crystal Bell", "PHRASE", "e07_s3.wav" } },
      { c (0x0b0018), c (0x2a0040), ca (0xb30b0018), c (0xffffff), c (0xc49be0), c (0xff3df2), c (0x2fe0ff),
        ca (0xbf140028), c (0xff3df2), c (0x001018), c (0x2fe0ff), ca (0x40ff3df2), c (0xffffff), c (0xff3df2),
        KnobStyle::neon, FrameStyle::neon, 4.0f, false, false, true, true, false, false, true } },
    { "EP08", "1995", "Golden cel", "Golden\nCel Era", "Soft 16-bit samples with echo.", "e08.jpg",
      { { "Cel Flute", "LEAD", "e08_o1.wav", 72 }, { "Cel Strings", "PAD", "e08_o2.wav", 60 }, { "Cel Pluck", "PLUCK", "e08_o3.wav", 72 },
        { "Sunset Flute", "PHRASE", "e08_s1.wav" }, { "Cel Strings Pad", "PHRASE", "e08_s2.wav" }, { "Echo Pluck", "PHRASE", "e08_s3.wav" } },
      { c (0xffe3b0), c (0xb06e8e), ca (0xb3fff8ec), c (0x2b1a2e), c (0x6e4a5e), c (0xe8502a), c (0xffb27a),
        ca (0xe0fffaf0), ca (0x142b1a2e), c (0x2b1a2e), c (0xffd9a0), c (0x2b1a2e), c (0xffd9a0), c (0xe8502a),
        KnobStyle::soft, FrameStyle::polaroid, 12.0f, false, false, false, false, false, false, false } },
    { "EP09", "2003", "First digital", "First\nDigital", "CD samples, cold and wide.", "e09.jpg",
      { { "Ghost Choir", "PAD", "e09_o1.wav", 60 }, { "CD Grand", "KEYS", "e09_o2.wav", 60 }, { "PCM Bass", "BASS", "e09_o3.wav", 36 },
        { "Subway Ghost Choir", "PHRASE", "e09_s1.wav" }, { "CD Piano", "PHRASE", "e09_s2.wav" }, { "Menu Select", "FX", "e09_s3.wav" } },
      { c (0xf1f7fa), c (0xa9c1cd), c (0xe3eef3), c (0x0e2a36), c (0x4c6a78), c (0x00a3c4), c (0x7fe0c8),
        c (0xf4f8fa), c (0x8fb3c2), c (0x062a33), c (0x7fe0c8), c (0x00a3c4), c (0xffffff), c (0x00708a),
        KnobStyle::gloss, FrameStyle::gloss, 14.0f, false, false, false, false, false, false, false } },
    { "EP10", "2008", "HD TV", "Rooftop\nDusk", "Clean keys, wide pads, quiet nights.", "e10.jpg",
      { { "Rooftop E-Piano", "KEYS", "e10_o1.wav", 60 }, { "Dusk Saw Pad", "PAD", "e10_o2.wav", 60 }, { "Dusk Sub", "BASS", "e10_o3.wav", 36 },
        { "Rooftop Keys", "PHRASE", "e10_s1.wav" }, { "Dusk Pad", "PHRASE", "e10_s2.wav" }, { "Phone UI Chime", "FX", "e10_s3.wav" } },
      { c (0xfbfbfd), c (0xf1f1f6), c (0xffffff), c (0x22232a), c (0x8a8c99), c (0xff5a6e), c (0x6c8cff),
        c (0xffffff), c (0xececf2), c (0xf3f3f8), c (0xff5a6e), c (0xff5a6e), c (0xffffff), c (0x22232a),
        KnobStyle::flat, FrameStyle::card, 12.0f, false, false, false, false, false, false, false } },
    { "EP11", "2013", "Key frames", "Fire &\nChains", "Chip sounds with a modern mix.", "e11.jpg",
      { { "Supersaw Pulse", "LEAD", "e11_o1.wav", 72 }, { "Chip Pulse", "PLUCK", "e11_o2.wav", 72 }, { "Glitch Hit", "FX", "e11_o3.wav", 60 },
        { "Neo Chip Inferno", "PHRASE", "e11_s1.wav" }, { "Chip Arp Rush", "LOOP", "e11_s2.wav" }, { "Glitch Riser", "FX", "e11_s3.wav" } },
      { c (0xff6a1f), c (0xf0581a), c (0x111111), c (0x111111), c (0x5a2a0a), c (0x111111), c (0xffffff),
        c (0xffffff), c (0x111111), c (0xff6a1f), c (0x111111), c (0xffffff), c (0x111111), c (0xff6a1f),
        KnobStyle::flat, FrameStyle::flat, 0.0f, false, false, true, false, false, false, false } },
    { "EP12", "NOW", "Underground", "Under\nground", "Every era, made for trap.", "e12.jpg",
      { { "Rage Bell Note", "BELLS", "e12_o1.wav", 72 }, { "808 Sub", "BASS", "e12_o2.wav", 36 }, { "Chip Clap", "DRUMS", "e12_o3.wav", 60 },
        { "Rage Bell", "PHRASE", "e12_s1.wav" }, { "808 Chip Sub", "PHRASE", "e12_s2.wav" }, { "Trap Chip Loop", "LOOP", "e12_s3.wav" } },
      { c (0x050405), c (0x050405), ca (0x99050405), c (0xffffff), c (0x8b91a1), c (0xff1f3d), c (0xffd23a),
        ca (0xdb09070a), c (0x2a2326), c (0x0a0a08), c (0xff1f3d), c (0xffd23a), c (0x05070b), c (0xffffff),
        KnobStyle::metal, FrameStyle::trap, 6.0f, false, true, true, true, false, false, true } },
};
} // namespace

const Era& era (int index) { return eras[juce::jlimit (0, numEras - 1, index)]; }
int eraOfPreset (int preset) { return juce::jlimit (0, numPresets - 1, preset) / soundsPerEra; }
const SoundInfo& sound (int preset)
{
    preset = juce::jlimit (0, numPresets - 1, preset);
    return eras[preset / soundsPerEra].sounds[preset % soundsPerEra];
}
} // namespace bitera
