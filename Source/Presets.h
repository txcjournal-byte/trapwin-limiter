#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include "Eras.h"

// BIT ERA factory bank: 12 eras x 600 presets = 7,200. Preset 0..5 of every era are the era sounds; the other 594 are
// designed from era-true recipes (sources, envelopes, filters, layers, arp / sequencer patterns, FX and modulation)
// with a deterministic seed per preset, so every preset is its own sound and the bank costs no disk space.
namespace bitera
{
constexpr int presetsPerEra = 600;
constexpr int numPresets = numEras * presetsPerEra;

enum Category { cBass = 0, cLead, cPad, cKeys, cPluck, cBells, cBrass, cStrings, cOrgan, cWinds, cWaves, cDrums, cPerc, cFx, cArp, cSeq, numCategories };
enum PresetType { ptShot = 0, ptSustain, ptPhrase, ptLoop, numPresetTypes };   // shot = plays out by itself, sustain = holds while the key is down

const char* categoryName (int c);
const char* presetTypeName (int t);
const char* platformName (int eraIndex);   // the game platform(s) an era plays

struct PresetInfo
{
    juce::String name;
    int era = 0, category = 0, type = ptShot, root = 60;
    float previewSeconds = 1.5f;
};

const PresetInfo& presetInfo (int index);
inline int eraOfPreset (int index) { return juce::jlimit (0, numPresets - 1, index) / presetsPerEra; }

struct Patch
{
    std::vector<std::pair<juce::String, float>> params;
    struct Step { int layer; const char* field; int step; float value; };
    std::vector<Step> steps;
};

// the full parameter set of a preset (applied on top of the defaults)
Patch buildPatch (int index);
} // namespace bitera
