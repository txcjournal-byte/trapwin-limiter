#include "Presets.h"
#include "Engine.h"
#include <set>
#include <map>

namespace bitera
{
using chips::Source;
namespace
{
// ---------------------------------------------------------------------------------------------------------------
// names
const char* adjectives[numEras][25] = {
    { "Silent", "Sepia", "Ink", "Reel", "Lantern", "Nitrate", "Paper", "Moonlit", "Kinema", "Celluloid", "Shadow", "Velvet", "Flicker",
      "Gaslight", "Sumi", "Lacquer", "Tin", "Gramophone", "Lotus", "Cinder", "Foghorn", "Parlor", "Ash", "Pathos", "Iris" },
    { "Technicolor", "Pastel", "Storybook", "Lagoon", "Ribbon", "Daydream", "Cotton", "Carousel", "Orchard", "Spring", "Pearl", "Meadow",
      "Cherry", "Halo", "Matinee", "Paper Moon", "Golden Age", "Silk", "Fable", "Willow", "Dew", "Sunbeam", "Garden", "Lullaby", "Bluebird" },
    { "Rocket", "Tube", "Antenna", "Atom", "Valve", "Test Card", "Cathode", "Signal", "Orbit", "Gamma", "Relay", "Monotone", "Broadcast",
      "Rabbit Ear", "Telex", "Jet", "Ion", "Morse", "Radar", "Satellite", "Spark", "Tin Robot", "Zinc", "Channel 3", "Countdown" },
    { "Grit", "Rain", "Blood Moon", "Alley", "Dojo", "Steel", "Gutter", "Ronin", "Smoke", "Rust", "Iron", "Asphalt", "Scar", "Vendetta",
      "Concrete", "Lone", "Storm", "Graveyard", "Ember", "Drifter", "Raw", "Knuckle", "Tarmac", "Sweat", "Bruise" },
    { "Mecha", "Orbital", "Colony", "Titan", "Ion Drive", "Hangar", "Thruster", "Zero-G", "Beam", "Plasma", "Armored", "Cockpit", "Nebula",
      "Pilot", "Starship", "Hyperdrive", "Bulkhead", "Reactor", "Comet", "Galactic", "Command", "Payload", "Sentinel", "Warp", "Launch Bay" },
    { "Wasteland", "Pixel", "Cartridge", "Dust", "Fist", "Sprite", "Ruin", "Outlaw", "Scrap", "Desert", "Pocket", "Rebel", "Overworld",
      "Dungeon", "Boss", "Arcade", "Power", "Hero", "Coin", "Respawn", "Combo", "Turbo", "Barren", "Raider", "Bit" },
    { "Neon", "Cyber", "Chrome", "Midnight", "Grid", "Overdrive", "Laser", "Hologram", "Circuit", "Voltage", "Highway", "Downtown",
      "Turbine", "Mirror", "Riot", "Siren", "Nightshift", "Skyline", "Afterburn", "Prism", "Arcology", "Megacity", "Magnetic", "Data", "Visor" },
    { "Golden", "Cel", "Sunset", "Harbor", "Summer", "Spirit", "Cicada", "Dusk", "Ocean", "Wind", "Petal", "Twilight", "Shrine", "Firefly",
      "Horizon", "Tidal", "Crane", "Amber", "Haze", "Breeze", "Starlight", "Coral", "Seaside", "Kite", "Lantern Festival" },
    { "Digital", "Subway", "Phantom", "Glass", "Server", "Wired", "Bluescreen", "Raster", "Mono", "Cold", "Fiber", "Void", "Silicon",
      "Frost", "Cipher", "Archive", "Proxy", "Binary", "Polygon", "Firewall", "Ghost Shell", "Network", "Terminal", "Sector", "Packet" },
    { "Rooftop", "HD", "Lo-Fi", "Late Night", "Window", "Cassette", "Commuter", "Cloud", "Bedroom", "Vapor", "Fluorescent", "Afterglow",
      "Soft", "Polaroid", "Melancholy", "Gentle", "Hush", "Station", "Umbrella", "Train", "Balcony", "Streetlamp", "Notebook", "Tea", "Sleepy" },
    { "Inferno", "Chained", "Blade", "Wall", "Crimson", "Overlord", "Vanguard", "Ashen", "Slayer", "Hellfire", "Molten", "Wrath", "Cursed",
      "Demon", "Ironclad", "Furnace", "Abyss", "Scorch", "Brimstone", "Onslaught", "Warcry", "Titan Fall", "Colossal", "Fury", "Ruinous" },
    { "Underground", "Trap", "Rage", "Drill", "Phonk", "Cage", "Opium", "Block", "Smoke", "Concrete", "Plug", "Dark", "Cold", "Ice", "Shadow",
      "Drip", "Basement", "Street", "Knockout", "Venom", "Pressure", "Night Shift", "Hoodie", "Codeine Sky", "Last Round" },
};

const char* nouns[numCategories][12] = {
    { "Bass", "Sub", "Thump", "Growl", "Rumble", "Low", "Boom", "Floor", "Weight", "Undertow", "Bottom", "Knock" },
    { "Lead", "Hero", "Solo", "Line", "Melody", "Cry", "Scream", "Whistle", "Saw", "Shine", "Blade", "Call" },
    { "Pad", "Drift", "Cloud", "Haze", "Horizon", "Field", "Glow", "Veil", "Bloom", "Mist", "Atmos", "Tide" },
    { "Keys", "Piano", "Upright", "Grand", "E-Piano", "Clav", "Ivory", "Parlor Keys", "Tines", "Felt Keys", "Chord Keys", "Hammer" },
    { "Pluck", "Pizz", "Harp", "Pick", "Snap", "Drop", "Kalimba", "Koto", "Zither", "Twang", "Flick", "Tick" },
    { "Bells", "Chime", "Celesta", "Glockenspiel", "Music Box", "Gong", "Toll", "Carillon", "Tinkle", "Shimmer", "Crystal", "Tubular" },
    { "Brass", "Horns", "Fanfare", "Stab", "Section", "Trumpet", "Blast", "Brass Hit", "Tuba", "Call", "Charge", "Salute" },
    { "Strings", "Violins", "Ensemble", "Cellos", "Section", "Quartet", "Bows", "Tremolo", "Legato", "Orchestra", "Violas", "Swell" },
    { "Organ", "Drawbar", "Chapel", "Combo Organ", "Pipe", "Reed", "Harmonium", "Calliope", "Rotary", "Theatre Organ", "Church", "Fairground" },
    { "Flute", "Whistle", "Ocarina", "Pan Pipe", "Recorder", "Shakuhachi", "Clarinet", "Oboe", "Piccolo", "Breath", "Reed", "Fife" },
    { "Choir", "Voice", "Aahs", "Oohs", "Vox", "Chant", "Hum", "Choral", "Spirit Voice", "Siren", "Whisper", "Hymn" },
    { "Kick", "Snare", "Hat", "Open Hat", "Tom", "Clap", "Rim", "Crash", "Kick", "Snare", "Hat", "Tom" },
    { "Cowbell", "Woodblock", "Blip", "Shaker", "Conga", "Triangle", "Clave", "Tick", "Agogo", "Bongo", "Cabasa", "Ping" },
    { "Laser", "Riser", "Drop", "Explosion", "Coin", "Power-Up", "Jump", "Zap", "Alarm", "UI Blip", "Warp", "Impact" },
    { "Arp", "Run", "Cascade", "Spiral", "Climb", "Ladder", "Sparkle", "Ripple", "Chase", "Stairs", "Gallop", "Twirl" },
    { "Loop", "Groove", "Pattern", "Sequence", "March", "Bounce", "Shuffle", "Engine", "Drive", "Pulse", "Cycle", "Rhythm" },
};

// generated presets per category and era (594 = 600 minus the 6 era sounds)
const int counts[numCategories] = { 70, 68, 50, 40, 45, 35, 30, 25, 20, 20, 15, 50, 30, 36, 35, 25 };
const int rootOf[numCategories] = { 36, 72, 60, 60, 72, 84, 60, 60, 60, 72, 60, 48, 60, 60, 60, 48 };

// ---------------------------------------------------------------------------------------------------------------
// what each era has to play with
struct Palette { std::vector<int> synths, samplers, sampleEras, layerFx, masterFx; };
Palette palette (int e)
{
    using namespace chips;
    switch (e)
    {
        case 0:  return { { psg }, { eraSample, sampler8, rompler90s }, { 0, 1 }, { fxWowFlutter, fxRadio, fxReverb, fxEq }, { fxRadio, fxWowFlutter, fxReverb } };
        case 1:  return { { psg }, { eraSample, sampler16, rompler90s }, { 0, 1, 2 }, { fxChorus, fxReverb, fxWowFlutter, fxEq }, { fxReverb, fxChorus, fxWowFlutter } };
        case 2:  return { { psg, polyCounter, computerSid }, { eraSample, sampler8 }, { 1, 2, 3 }, { fxDelay, fxWowFlutter, fxRadio, fxRingMod }, { fxDelay, fxReverb, fxRadio } };
        case 3:  return { { polyCounter, psg }, { sampler8, eraSample }, { 2, 3, 4 }, { fxBite, fxDrive, fxDelay, fxFilter }, { fxBite, fxDrive, fxReverb } };
        case 4:  return { { computerSid, polyCounter, psg }, { eraSample, sampler8 }, { 3, 4, 5 }, { fxPhaser, fxDelay, fxChorus, fxFlanger }, { fxDelay, fxReverb, fxPhaser } };
        case 5:  return { { console8, handheld4, psg }, { sampler8 }, { 4, 5, 6 }, { fxBite, fxDelay, fxConsoleEcho, fxDrive }, { fxBite, fxConsoleEcho, fxReverb } };
        case 6:  return { { fm16, pcFm, psg, computerSid }, { sampler8, rompler90s }, { 5, 6, 7 }, { fxChorus, fxDelay, fxFlanger, fxDrive }, { fxChorus, fxDelay, fxReverb } };
        case 7:  return { { fm16, handheld4 }, { sampler16, rompler90s }, { 6, 7, 8 }, { fxConsoleEcho, fxChorus, fxReverb, fxEq }, { fxConsoleEcho, fxReverb, fxChorus } };
        case 8:  return { { fm16, handheld4 }, { sampler32, handheldPcm, rompler90s }, { 7, 8, 9 }, { fxReverb, fxDelay, fxFilter, fxWidth }, { fxReverb, fxDelay, fxCompressor } };
        case 9:  return { { computerSid, fm16 }, { console64, sampler32, eraSample }, { 8, 9, 10 }, { fxWowFlutter, fxDelay, fxChorus, fxEq }, { fxWowFlutter, fxReverb, fxDelay } };
        case 10: return { { console8, handheld4, computerSid, psg }, { handheldPcm, eraSample }, { 9, 10, 11 }, { fxDrive, fxDelay, fxBite, fxPhaser }, { fxDrive, fxDelay, fxLimiter } };
        default: return { { console8, handheld4, computerSid, fm16, psg }, { eraSample, sampler16, sampler32 }, { 10, 11, 5, 6 }, { fxDrive, fxDelay, fxBite, fxCompressor }, { fxDrive, fxLimiter, fxDelay } };
    }
}

// chip waves that suit a category
std::vector<int> wavesFor (int src, int cat)
{
    using namespace chips;
    switch (src)
    {
        case console8:
            switch (cat) { case cBass: return { 4, 2, 1 }; case cPad: case cOrgan: case cStrings: return { 2, 1 }; case cBells: case cWinds: return { 4, 2 };
                           case cDrums: case cFx: return { 5, 6, 4, 0 }; case cPerc: return { 0, 1, 6, 4 }; default: return { 0, 1, 2, 7 }; }
        case handheld4:
            switch (cat) { case cBass: return { 10, 4, 2 }; case cPad: return { 5, 7, 11, 9 }; case cKeys: case cOrgan: return { 6, 11, 5 }; case cBells: return { 7, 5 };
                           case cWinds: return { 5, 9 }; case cVoice: return { 9, 11 }; case cStrings: case cBrass: return { 4, 8 }; case cDrums: case cFx: return { 12, 13, 5 };
                           case cPerc: return { 0, 13, 7 }; default: return { 0, 1, 2, 4, 8, 9 }; }
        case computerSid:
            switch (cat) { case cBass: return { 0, 2, 1, 4 }; case cPad: case cStrings: return { 0, 2, 4 }; case cBells: case cPerc: return { 7, 1 }; case cVoice: return { 6, 1 };
                           case cWinds: case cKeys: return { 1, 2 }; case cDrums: case cFx: return { 3, 1, 8 }; case cOrgan: return { 2, 1 }; default: return { 0, 2, 4, 5, 8 }; }
        case fm16:
            switch (cat) { case cBass: return { 1, 8 }; case cKeys: return { 0, 5 }; case cBells: return { 3, 0, 7 }; case cBrass: return { 2 }; case cStrings: case cPad: case cVoice: return { 9, 5 };
                           case cOrgan: return { 5 }; case cPluck: return { 6, 0 }; case cPerc: case cDrums: case cFx: return { 7, 3, 8 }; case cWinds: return { 4, 9 }; default: return { 4, 2, 7, 6 }; }
        case pcFm:
            switch (cat) { case cBass: return { 1, 6 }; case cKeys: case cPluck: return { 4, 7 }; case cBells: case cPerc: return { 5, 4 }; case cBrass: return { 2 }; case cWinds: return { 3 };
                           case cOrgan: case cPad: case cStrings: case cVoice: return { 0, 3 }; case cDrums: case cFx: return { 5, 6 }; default: return { 6, 2, 3 }; }
        case psg:
            switch (cat) { case cBass: return { 2, 0 }; case cDrums: case cFx: return { 3, 4, 7 }; case cPerc: return { 4, 0 }; case cPad: case cStrings: return { 6, 5, 0 };
                           case cOrgan: return { 1, 0 }; default: return { 0, 1, 5 }; }
        case polyCounter:
            switch (cat) { case cBass: return { 1, 0 }; case cDrums: case cFx: return { 4, 3, 2, 6 }; case cPerc: return { 2, 5 }; default: return { 0, 5 }; }
        default: return { 0 };
    }
}

// era sounds that suit a category (sampler sources), from the palette's eras
std::vector<int> samplesFor (const Palette& pal, int cat)
{
    static const std::map<juce::String, std::vector<int>> catOf {
        { "BASS", { cBass, cSeq } }, { "LEAD", { cLead, cWinds, cArp } }, { "PAD", { cPad, cStrings, cVoice } }, { "KEYS", { cKeys, cPluck, cArp } },
        { "PLUCK", { cPluck, cArp, cPerc } }, { "BELLS", { cBells, cArp, cPerc } }, { "BRASS", { cBrass, cLead } }, { "STRINGS", { cStrings, cPad } },
        { "DRUMS", { cDrums, cPerc, cSeq } }, { "FX", { cFx } } };
    std::vector<int> out;
    for (int e : pal.sampleEras)
        for (int k = 0; k < soundsPerEra; ++k)
        {
            const auto& s = era (e).sounds[k];
            auto it = catOf.find (s.category);
            if (k >= 3 && juce::String (s.category) != "FX") continue;   // phrases / loops are not instruments
            if (it != catOf.end() && std::find (it->second.begin(), it->second.end(), cat) != it->second.end())
                out.push_back (e * soundsPerEra + k);
        }
    return out;
}

// ---------------------------------------------------------------------------------------------------------------
struct Builder
{
    Patch& p;
    juce::Random r;
    int e, cat;
    Palette pal;

    void set (const juce::String& id, float v) { p.params.push_back ({ id, v }); }
    void L (int l, const char* n, float v) { set ("l" + juce::String (l + 1) + "_" + n, v); }
    void fx (int l, int slot, int type, float p1, float p2, float p3, float mix)
    {
        const juce::String base = l < 0 ? "mfx" + juce::String (slot + 1) : "l" + juce::String (l + 1) + "_fx" + juce::String (slot + 1);
        set (base + "_type", (float) type); set (base + "_p1", p1); set (base + "_p2", p2); set (base + "_p3", p3); set (base + "_mix", mix);
    }
    int modSlot = 0;
    void mod (int src, int dst, float amt, int layer = 0)
    {
        if (modSlot >= numModSlots) return;
        const juce::String n = "mod" + juce::String (++modSlot) + "_";
        set (n + "src", (float) src); set (n + "dst", (float) dst); set (n + "layer", (float) layer); set (n + "amt", amt);
    }
    float rnd (float a, float b) { return a + (b - a) * r.nextFloat(); }
    float rndExp (float a, float b) { return a * std::pow (b / a, r.nextFloat()); }
    bool chance (float c) { return r.nextFloat() < c; }
    template <typename T> T pick (const std::vector<T>& v) { return v[(size_t) r.nextInt ((int) v.size())]; }

    // a source for a layer: a chip with a suitable wave, or an era sample of this category
    void source (int l, float samplerChance = 0.3f)
    {
        auto samples = samplesFor (pal, cat);
        const bool useSample = ! samples.empty() && (pal.synths.size() <= 1 ? true : chance (samplerChance));
        if (useSample || pal.synths.empty())
        {
            if (samples.empty()) samples = samplesFor (pal, cFx);
            if (samples.empty()) samples = { pal.sampleEras[0] * soundsPerEra };
            L (l, "src", (float) pick (pal.samplers));
            L (l, "wave", (float) pick (samples));
        }
        else
        {
            const int src = pick (pal.synths);
            L (l, "src", (float) src);
            L (l, "wave", (float) pick (wavesFor (src, cat)));
        }
        L (l, "char", rnd (0.15f, 0.85f));
    }

    void chipSource (int l, std::initializer_list<int> preferred)
    {
        std::vector<int> avail;
        for (int s : preferred) if (std::find (pal.synths.begin(), pal.synths.end(), s) != pal.synths.end()) avail.push_back (s);
        if (avail.empty()) { source (l, 0.0f); return; }
        const int src = pick (avail);
        L (l, "src", (float) src);
        L (l, "wave", (float) pick (wavesFor (src, cat)));
        L (l, "char", rnd (0.15f, 0.85f));
    }

    void env (int l, float a, float d, float s, float rel) { L (l, "attack", a); L (l, "decay", d); L (l, "sustain", s); L (l, "release", rel); }
    void filter (int l, int type, float cutoff, float reso, float fenv, float fa, float fd, float fs, float fr)
    {
        L (l, "filter", (float) type); L (l, "cutoff", cutoff); L (l, "reso", reso); L (l, "fenv", fenv);
        L (l, "fattack", fa); L (l, "fdecay", fd); L (l, "fsustain", fs); L (l, "frelease", fr);
    }
    void layerFx (int l, int slot, float mix)
    {
        const int t = pick (pal.layerFx);
        fx (l, slot, t, fxDefault (t, 0) + rnd (-0.15f, 0.15f), fxDefault (t, 1) + rnd (-0.15f, 0.15f), fxDefault (t, 2) + rnd (-0.1f, 0.1f), mix);
    }

    // standard performance controls of every preset (the MACRO page)
    void standardMacros()
    {
        mod (msMacro1, mdCutoff, -0.6f);   // M1 DARK
        mod (msMacro1, mdTone, -0.5f);
        mod (msMacro1 + 2 - 1, mdChar, 0.8f);      // M2 CHARACTER
        mod (msMacro1 + 3 - 1, mdSpace, 0.7f);     // M3 SPACE
        mod (msMacro1 + 4 - 1, mdVibrato, 0.4f);   // M4 MOTION
        mod (msMacro1 + 5 - 1, mdMFx4, -0.85f);    // M5 CRUNCH (master slot 4 = bit crusher)
        mod (msMacro1 + 6 - 1, mdPitch, 0.5f);     // M6 PITCH (+12 st at full)
        mod (msMacro1 + 7 - 1, mdFxAMix, 1.0f);    // M7 FX A
        mod (msMacro8, mdMFx3, 0.5f);      // M8 WIDTH
        mod (msWheel, mdVibrato, 0.3f);
        fx (-1, 2, fxWidth, 0.5f, 0.0f, 0.5f, 1.0f);
        fx (-1, 3, fxBite, 1.0f, 1.0f, 0.0f, 1.0f);
    }
};

void designBass (Builder& b)
{
    b.source (0, 0.25f);
    b.env (0, b.rnd (0, 4), b.rndExp (150, 900), b.rnd (0.0f, 0.8f), b.rndExp (40, 250));
    if (b.chance (0.6f)) b.filter (0, b.chance (0.6f) ? fLp24 : fLp12, b.rndExp (250, 3000), b.rnd (0.0f, 0.65f), b.rnd (0.2f, 0.7f), 1, b.rndExp (80, 500), b.rnd (0, 0.3f), 200);
    if (b.chance (0.45f)) { b.L (0, "mode", b.chance (0.5f) ? vmMono : vmLegato); b.L (0, "glide", b.chance (0.5f) ? b.rnd (10, 90) : 0.0f); }
    b.L (0, "vel", b.rnd (0.2f, 0.7f));
    if (b.chance (0.5f))   // sub or octave layer
    {
        b.L (1, "on", 1);
        b.chipSource (1, { chips::console8, chips::handheld4, chips::computerSid, chips::psg, chips::polyCounter });
        const bool sub = b.chance (0.6f);
        b.L (1, "coarse", sub ? -12.0f : 12.0f); b.L (1, "level", sub ? b.rnd (0.45f, 0.7f) : b.rnd (0.2f, 0.4f));
        b.env (1, 1, b.rndExp (200, 900), b.rnd (0.3f, 0.9f), 120);
    }
    if (b.chance (0.35f)) b.fx (0, 0, fxDrive, b.rnd (0.1f, 0.45f), (float) b.r.nextInt (5) / 4.0f, b.rnd (0.4f, 0.8f), b.rnd (0.4f, 1.0f));
    if (b.chance (0.3f)) b.mod (msVelocity, mdCutoff, b.rnd (0.2f, 0.5f), 1);
}

void designLead (Builder& b)
{
    b.source (0, 0.2f);
    b.env (0, b.rnd (0, 30), b.rndExp (200, 1500), b.rnd (0.5f, 1.0f), b.rndExp (80, 500));
    b.L (0, "vibdepth", b.rnd (0, 25)); b.L (0, "vibrate", b.rnd (4.5f, 6.5f));
    if (b.chance (0.55f)) { b.L (0, "mode", b.chance (0.5f) ? vmMono : vmLegato); b.L (0, "glide", b.rnd (15, 150)); }
    if (b.chance (0.35f)) b.filter (0, fLp12, b.rndExp (1500, 9000), b.rnd (0, 0.5f), b.rnd (0.1f, 0.5f), 1, b.rndExp (150, 700), b.rnd (0.3f, 0.8f), 200);
    if (b.chance (0.45f))
    {
        b.L (1, "on", 1); b.source (1, 0.1f);
        const bool oct = b.chance (0.4f);
        b.L (1, "coarse", oct ? 12.0f : 0.0f); b.L (1, "fine", oct ? 0.0f : b.rnd (6, 15) * (b.chance (0.5f) ? 1.0f : -1.0f));
        b.L (1, "level", b.rnd (0.35f, 0.65f)); b.L (1, "pan", b.rnd (-0.3f, 0.3f));
        b.env (1, b.rnd (0, 30), b.rndExp (200, 1500), b.rnd (0.5f, 1.0f), b.rndExp (80, 500));
    }
    if (b.chance (0.5f)) b.fx (0, 0, fxDelay, b.pick (std::vector<float> { 0.42f, 0.58f, 0.67f }), b.rnd (0.2f, 0.5f), b.chance (0.3f) ? 0.9f : 0.0f, b.rnd (0.15f, 0.35f));
    if (b.chance (0.25f)) b.layerFx (0, 1, b.rnd (0.3f, 0.7f));
    if (b.chance (0.3f)) b.mod (msLfo1, mdPan, 0.2f);
}

void designPad (Builder& b)
{
    const int layers = 2 + (b.chance (0.4f) ? 1 : 0);
    for (int l = 0; l < layers; ++l)
    {
        b.L (l, "on", 1); b.source (l, 0.3f);
        b.env (l, b.rndExp (300, 2500), b.rndExp (500, 3000), b.rnd (0.6f, 1.0f), b.rndExp (800, 4000));
        b.L (l, "fine", l == 0 ? 0.0f : b.rnd (5, 14) * (l % 2 ? 1.0f : -1.0f));
        b.L (l, "coarse", l == 2 ? (b.chance (0.5f) ? 12.0f : -12.0f) : 0.0f);
        b.L (l, "level", l == 0 ? 0.6f : b.rnd (0.35f, 0.5f)); b.L (l, "pan", l == 0 ? 0.0f : (l % 2 ? -0.4f : 0.4f));
        b.filter (l, fLp12, b.rndExp (800, 6000), b.rnd (0, 0.4f), b.rnd (0, 0.3f), b.rndExp (300, 2000), 1500, 0.6f, 2000);
        b.L (l, "vibdepth", b.rnd (0, 10));
    }
    b.set ("lfo1_rate", b.rndExp (0.08f, 0.6f)); b.set ("lfo1_shape", (float) b.pick (std::vector<int> { lsSine, lsTriangle, lsSmoothRandom }));
    b.mod (msLfo1, mdCutoff, b.rnd (0.1f, 0.35f));
    b.fx (0, 0, fxChorus, b.rnd (0.1f, 0.4f), b.rnd (0.3f, 0.7f), 0.5f, b.rnd (0.3f, 0.6f));
    b.fx (-1, 0, fxReverb, b.rnd (0.5f, 0.9f), b.rnd (0.3f, 0.6f), 1.0f, b.rnd (0.3f, 0.6f));
}

void designKeys (Builder& b)
{
    b.source (0, 0.4f);
    b.env (0, b.rnd (0, 5), b.rndExp (600, 3000), b.rnd (0, 0.4f), b.rndExp (150, 600));
    b.L (0, "vel", b.rnd (0.5f, 0.9f));
    if (b.chance (0.3f)) { b.L (1, "on", 1); b.chipSource (1, { chips::fm16, chips::handheld4, chips::computerSid, chips::pcFm });
                           b.L (1, "coarse", b.chance (0.5f) ? 12.0f : 19.0f); b.L (1, "level", b.rnd (0.15f, 0.3f)); b.env (1, 0, b.rndExp (200, 800), 0, 200); }
    if (b.chance (0.4f)) b.layerFx (0, 0, b.rnd (0.2f, 0.5f));
    b.mod (msVelocity, mdCutoff, 0.4f);
    if (b.chance (0.5f)) b.filter (0, fLp12, b.rndExp (2000, 12000), 0.1f, b.rnd (0.1f, 0.4f), 1, 600, 0.2f, 300);
}

void designPluck (Builder& b)
{
    b.source (0, 0.25f);
    b.env (0, b.rnd (0, 3), b.rndExp (120, 700), 0.0f, b.rndExp (80, 400));
    b.filter (0, b.chance (0.5f) ? fLp24 : fLp12, b.rndExp (400, 3000), b.rnd (0, 0.6f), b.rnd (0.3f, 0.8f), 1, b.rndExp (60, 400), 0, 150);
    if (b.chance (0.6f)) b.fx (0, 0, b.pick (std::vector<int> { fxDelay, fxConsoleEcho }), b.rnd (0.3f, 0.7f), b.rnd (0.25f, 0.55f), b.rnd (0.0f, 0.6f), b.rnd (0.2f, 0.45f));
    if (b.chance (0.3f)) { b.L (1, "on", 1); b.source (1, 0.1f); b.L (1, "coarse", 12); b.L (1, "level", 0.3f); b.env (1, 0, b.rndExp (80, 300), 0, 100); }
}

void designBells (Builder& b)
{
    b.source (0, 0.3f);
    b.env (0, b.rnd (0, 4), b.rndExp (900, 4000), 0.0f, b.rndExp (500, 2500));
    if (b.chance (0.45f)) { b.L (1, "on", 1); b.source (1, 0.2f); b.L (1, "coarse", b.chance (0.5f) ? 12.0f : 19.0f); b.L (1, "level", b.rnd (0.2f, 0.45f)); b.env (1, 0, b.rndExp (400, 2000), 0, 800); }
    b.fx (-1, 0, fxReverb, b.rnd (0.4f, 0.8f), b.rnd (0.2f, 0.5f), 1.0f, b.rnd (0.25f, 0.5f));
    if (b.chance (0.3f)) b.fx (0, 0, fxConsoleEcho, b.rnd (0.2f, 0.6f), b.rnd (0.3f, 0.5f), b.rnd (0, 1), 0.3f);
}

void designBrass (Builder& b)
{
    for (int l = 0; l < 2; ++l)
    {
        b.L (l, "on", 1); b.source (l, 0.25f);
        b.env (l, b.rnd (10, 80), b.rndExp (300, 1200), b.rnd (0.6f, 0.9f), b.rndExp (120, 400));
        b.filter (l, fLp24, b.rndExp (800, 2500), b.rnd (0, 0.3f), b.rnd (0.3f, 0.6f), b.rnd (20, 90), b.rndExp (200, 700), 0.4f, 200);
        b.L (l, "fine", l == 0 ? 0.0f : b.rnd (6, 12)); b.L (l, "level", l == 0 ? 0.65f : 0.5f);
        b.L (l, "vibdepth", b.rnd (0, 8));
    }
    if (b.chance (0.3f)) b.L (0, "penv", -b.rnd (0.5f, 2.0f)), b.L (0, "pdecay", 60);
    b.mod (msVelocity, mdCutoff, 0.35f);
}

void designStrings (Builder& b)
{
    for (int l = 0; l < 2; ++l)
    {
        b.L (l, "on", 1); b.source (l, 0.35f);
        b.env (l, b.rndExp (150, 900), 800, b.rnd (0.8f, 1.0f), b.rndExp (400, 1500));
        b.L (l, "fine", l == 0 ? -b.rnd (3, 9) : b.rnd (3, 9)); b.L (l, "pan", l == 0 ? -0.35f : 0.35f); b.L (l, "level", 0.55f);
        b.L (l, "vibdepth", b.rnd (8, 18)); b.L (l, "vibrate", b.rnd (4.8f, 6.0f));
        b.filter (l, fLp12, b.rndExp (2500, 9000), 0.1f, 0.1f, 300, 900, 0.8f, 800);
    }
    if (b.chance (0.4f)) b.L (0, "coarse", -12.0f);
    b.fx (0, 0, fxChorus, 0.2f, 0.5f, 0.6f, b.rnd (0.25f, 0.5f));
    b.fx (-1, 0, fxReverb, b.rnd (0.5f, 0.85f), 0.4f, 1.0f, b.rnd (0.25f, 0.45f));
}

void designOrgan (Builder& b)
{
    b.source (0, 0.15f);
    b.env (0, b.rnd (2, 15), 100, 1.0f, b.rnd (30, 150));
    b.L (1, "on", 1); b.source (1, 0.1f); b.L (1, "coarse", b.chance (0.6f) ? 12.0f : 19.0f); b.L (1, "level", b.rnd (0.3f, 0.5f)); b.env (1, 5, 100, 1.0f, 80);
    if (b.chance (0.5f)) b.fx (0, 0, fxTremolo, b.rnd (0.45f, 0.65f), b.rnd (0.2f, 0.5f), b.chance (0.5f) ? 0.8f : 0.2f, 1.0f);
    else b.fx (0, 0, fxChorus, b.rnd (0.4f, 0.7f), 0.5f, 0.7f, 0.5f);
}

void designWinds (Builder& b)
{
    b.source (0, 0.25f);
    b.env (0, b.rnd (30, 120), 600, b.rnd (0.75f, 0.95f), b.rnd (100, 300));
    b.L (0, "vibdepth", b.rnd (10, 20)); b.L (0, "vibrate", b.rnd (4.8f, 5.6f));
    b.filter (0, fLp12, b.rndExp (2000, 7000), 0.15f, 0.15f, 40, 300, 0.7f, 200);
    if (std::find (b.pal.synths.begin(), b.pal.synths.end(), (int) chips::psg) != b.pal.synths.end() && b.chance (0.6f))
    {
        b.L (1, "on", 1); b.L (1, "src", (float) chips::psg); b.L (1, "wave", 3); b.L (1, "level", b.rnd (0.12f, 0.22f));
        b.filter (1, fBp12, b.rndExp (1500, 4000), 0.3f, 0, 1, 300, 0, 200); b.env (1, 40, 300, 0.5f, 200);
    }
    if (b.chance (0.4f)) b.L (0, "mode", vmLegato), b.L (0, "glide", b.rnd (20, 80));
}

void designVoice (Builder& b)
{
    for (int l = 0; l < 2; ++l)
    {
        b.L (l, "on", 1); b.source (l, 0.4f);
        b.env (l, b.rndExp (100, 600), 900, 0.9f, b.rndExp (400, 1500));
        b.filter (l, fBp12, b.rndExp (650, 1600), b.rnd (0.3f, 0.6f), 0, 1, 500, 0, 500);
        b.L (l, "fine", l == 0 ? -6.0f : 6.0f); b.L (l, "pan", l == 0 ? -0.3f : 0.3f); b.L (l, "vibdepth", 8); b.L (l, "level", 0.6f);
    }
    b.set ("lfo1_rate", b.rnd (0.1f, 0.4f));
    b.mod (msLfo1, mdCutoff, b.rnd (0.1f, 0.25f));
    b.fx (0, 0, fxChorus, 0.25f, 0.6f, 0.7f, 0.5f);
    b.fx (-1, 0, fxReverb, 0.75f, 0.4f, 1.0f, 0.4f);
}

void designDrum (Builder& b, int kind)
{
    // kinds: 0 kick, 1 snare, 2 hat, 3 open hat, 4 tom, 5 clap, 6 rim, 7 crash
    auto noiseSrc = [&] (int l)
    {
        std::vector<std::pair<int, int>> opts;
        for (int s : b.pal.synths)
        {
            if (s == chips::console8) opts.push_back ({ s, b.chance (0.7f) ? 5 : 6 });
            if (s == chips::handheld4) opts.push_back ({ s, b.chance (0.7f) ? 12 : 13 });
            if (s == chips::computerSid) opts.push_back ({ s, 3 });
            if (s == chips::psg) opts.push_back ({ s, 3 });
            if (s == chips::polyCounter) opts.push_back ({ s, b.chance (0.5f) ? 4 : 3 });
        }
        if (opts.empty()) opts.push_back ({ chips::psg, 3 });
        auto o = b.pick (opts);
        b.L (l, "src", (float) o.first); b.L (l, "wave", (float) o.second); b.L (l, "char", b.rnd (0.2f, 0.8f));
    };
    auto toneSrc = [&] (int l)
    {
        std::vector<std::pair<int, int>> opts;
        for (int s : b.pal.synths)
        {
            if (s == chips::console8) opts.push_back ({ s, 4 });
            if (s == chips::handheld4) opts.push_back ({ s, 5 });
            if (s == chips::computerSid) opts.push_back ({ s, 1 });
            if (s == chips::psg) opts.push_back ({ s, 0 });
            if (s == chips::fm16) opts.push_back ({ s, 8 });
            if (s == chips::polyCounter) opts.push_back ({ s, 0 });
        }
        if (opts.empty()) opts.push_back ({ chips::psg, 0 });
        auto o = b.pick (opts);
        b.L (l, "src", (float) o.first); b.L (l, "wave", (float) o.second);
    };
    switch (kind)
    {
        case 0:
            toneSrc (0); b.L (0, "coarse", -12); b.L (0, "penv", b.rnd (24, 40)); b.L (0, "pdecay", b.rnd (20, 70));
            b.env (0, 0, b.rndExp (150, 450), 0, 80);
            if (b.chance (0.5f)) { b.L (1, "on", 1); noiseSrc (1); b.L (1, "level", 0.3f); b.env (1, 0, 25, 0, 20); }
            break;
        case 1:
            noiseSrc (0); b.env (0, 0, b.rndExp (120, 260), 0, 80); b.filter (0, fHp12, b.rndExp (300, 1500), 0.2f, 0, 1, 100, 0, 50);
            b.L (1, "on", 1); toneSrc (1); b.L (1, "penv", b.rnd (7, 14)); b.L (1, "pdecay", 30); b.L (1, "level", b.rnd (0.35f, 0.6f)); b.env (1, 0, 90, 0, 50);
            break;
        case 2: case 3:
            noiseSrc (0); b.env (0, 0, kind == 2 ? b.rndExp (30, 90) : b.rndExp (200, 450), 0, kind == 2 ? 30.0f : 200.0f);
            b.filter (0, fHp12, b.rndExp (5000, 10000), b.rnd (0.1f, 0.5f), 0, 1, 100, 0, 50);
            break;
        case 4:
            toneSrc (0); b.L (0, "penv", b.rnd (7, 14)); b.L (0, "pdecay", b.rnd (40, 120)); b.env (0, 0, b.rndExp (200, 450), 0, 120);
            break;
        case 5:
            noiseSrc (0); b.env (0, 0, b.rndExp (120, 220), 0, 120); b.filter (0, fBp12, b.rndExp (900, 1800), 0.4f, 0, 1, 100, 0, 50);
            b.fx (0, 0, fxReverb, 0.3f, 0.5f, 1.0f, 0.25f);
            break;
        case 6:
            toneSrc (0); b.L (0, "coarse", 24); b.L (0, "penv", 12); b.L (0, "pdecay", 8); b.env (0, 0, b.rndExp (20, 45), 0, 20);
            break;
        default:
            noiseSrc (0); b.env (0, 0, b.rndExp (900, 2200), 0, 900); b.filter (0, fHp12, b.rndExp (2500, 6000), 0.2f, 0, 1, 100, 0, 50);
            break;
    }
    b.L (0, "vel", 0.7f);
}

void designPerc (Builder& b, int kind)
{
    switch (kind % 6)
    {
        case 0:   // cowbell: two pulses a fifth-ish apart
            b.chipSource (0, { chips::console8, chips::handheld4, chips::psg, chips::computerSid, chips::polyCounter });
            b.L (1, "on", 1); b.chipSource (1, { chips::console8, chips::handheld4, chips::psg, chips::computerSid, chips::polyCounter });
            b.L (1, "coarse", 7); b.L (1, "fine", -20);
            for (int l = 0; l < 2; ++l) { b.env (l, 0, b.rndExp (120, 400), 0, 100); b.filter (l, fBp12, 1800, 0.3f, 0, 1, 100, 0, 50); }
            break;
        case 1:   // woodblock
            b.source (0, 0.2f); b.env (0, 0, b.rndExp (40, 120), 0, 40); b.L (0, "penv", 3); b.L (0, "pdecay", 15); break;
        case 2:   // blip
            b.source (0, 0.1f); b.env (0, 0, b.rndExp (25, 90), 0, 20); b.L (0, "penv", b.rnd (5, 19)); b.L (0, "pdecay", b.rnd (10, 40)); break;
        case 3:   // shaker
            b.chipSource (0, { chips::psg, chips::console8, chips::handheld4, chips::computerSid, chips::polyCounter });
            b.L (0, "wave", 3); b.env (0, b.rnd (5, 25), b.rndExp (60, 160), 0, 50); b.filter (0, fHp12, b.rndExp (4000, 9000), 0.2f, 0, 1, 100, 0, 50);
            if (std::find (b.pal.synths.begin(), b.pal.synths.end(), (int) chips::psg) == b.pal.synths.end()) b.source (0, 0.0f);
            break;
        case 4:   // conga / bongo
            b.chipSource (0, { chips::console8, chips::handheld4, chips::computerSid, chips::fm16, chips::psg });
            b.L (0, "penv", b.rnd (4, 9)); b.L (0, "pdecay", b.rnd (20, 60)); b.env (0, 0, b.rndExp (150, 350), 0, 100); break;
        default:  // triangle / ping
            b.source (0, 0.3f); b.L (0, "coarse", 24); b.env (0, 0, b.rndExp (500, 1500), 0, 500); break;
    }
    if (b.chance (0.3f)) b.fx (0, 1, fxConsoleEcho, b.rnd (0.1f, 0.4f), 0.3f, b.rnd (0, 1), 0.25f);
}

void designFx (Builder& b, int kind)
{
    switch (kind % 12)
    {
        case 0:   // laser
            b.source (0, 0.0f); b.L (0, "penv", b.rnd (24, 48)); b.L (0, "pdecay", b.rnd (60, 250)); b.env (0, 0, b.rndExp (150, 450), 0, 100);
            if (b.chance (0.5f)) b.fx (0, 0, fxDelay, 0.3f, 0.4f, 0.0f, 0.3f);
            break;
        case 1:   // riser: the pitch envelope starts low and rises back to the key over seconds
            b.source (0, 0.0f); b.L (0, "penv", -b.rnd (18, 36)); b.L (0, "pdecay", b.rnd (900, 2500)); b.env (0, b.rnd (500, 2000), 4000, 1.0f, 300);
            b.L (1, "on", 1); b.chipSource (1, { chips::psg, chips::console8, chips::handheld4, chips::computerSid, chips::polyCounter });
            b.L (1, "wave", 3); b.env (1, 1500, 3000, 1, 300); b.filter (1, fLp12, 300, 0.4f, 0.8f, 2500, 3000, 1, 300); b.L (1, "level", 0.3f);
            break;
        case 2:   // drop
            b.source (0, 0.0f); b.L (0, "penv", b.rnd (12, 30)); b.L (0, "pdecay", b.rnd (500, 1500)); b.env (0, 0, b.rndExp (800, 2000), 0, 300); break;
        case 3:   // explosion
            designDrum (b, 7); b.filter (0, fLp24, b.rndExp (1500, 5000), 0.2f, -0.4f, 1, 900, 0, 500); b.env (0, 0, b.rndExp (800, 2000), 0, 600);
            b.L (1, "on", 1); b.chipSource (1, { chips::console8, chips::handheld4, chips::psg, chips::computerSid, chips::fm16 });
            b.L (1, "coarse", -24); b.L (1, "penv", 12); b.L (1, "pdecay", 200); b.env (1, 0, 600, 0, 300); b.L (1, "level", 0.5f);
            break;
        case 4:   // coin: a one-shot two-note arp
            b.chipSource (0, { chips::console8, chips::handheld4, chips::psg, chips::computerSid });
            b.env (0, 0, 300, 0.6f, 200);
            b.L (0, "arp", amArp); b.L (0, "arpdir", adUp); b.L (0, "arprate", 8); b.L (0, "arpoct", 2); b.L (0, "arpsteps", 2); b.L (0, "arpend", aeHold); b.L (0, "arpgate", 0.9f);
            break;
        case 5:   // power-up: fast one-shot run over octaves
            b.chipSource (0, { chips::console8, chips::handheld4, chips::psg, chips::computerSid, chips::fm16 });
            b.env (0, 0, 120, 0.5f, 80);
            b.L (0, "arp", amSeq); b.L (0, "arprate", 8); b.L (0, "arpsteps", 8); b.L (0, "arpend", aeOneShot); b.L (0, "arpgate", 0.8f);
            {
                const int scale[] = { 0, 4, 7, 12, 16, 19, 24, 28 }, alt[] = { 0, 3, 7, 10, 12, 15, 19, 24 };
                const bool minor = b.chance (0.5f);
                for (int s = 0; s < 8; ++s) b.p.steps.push_back ({ 0, "note", s, (float) (minor ? alt[s] : scale[s]) });
            }
            break;
        case 6:   // jump
            b.chipSource (0, { chips::console8, chips::handheld4, chips::psg, chips::computerSid });
            b.L (0, "penv", -b.rnd (5, 12)); b.L (0, "pdecay", b.rnd (60, 150)); b.env (0, 0, b.rndExp (150, 300), 0, 60); break;
        case 7:   // zap
            b.source (0, 0.0f); b.L (0, "penv", b.rnd (30, 48)); b.L (0, "pdecay", b.rnd (10, 40)); b.env (0, 0, b.rndExp (60, 200), 0, 40);
            b.fx (0, 0, fxRingMod, b.rnd (0.3f, 0.7f), b.rnd (0, 1), 0.3f, 0.6f);
            break;
        case 8:   // alarm: square LFO on pitch
            b.source (0, 0.0f); b.env (0, 0, 500, 1, 100);
            b.set ("lfo1_shape", lsSquare); b.set ("lfo1_rate", b.rnd (2, 7)); b.mod (msLfo1, mdPitch, b.rnd (0.1f, 0.3f));
            break;
        case 9:   // UI blip
            b.chipSource (0, { chips::console8, chips::handheld4, chips::psg, chips::computerSid, chips::fm16, chips::polyCounter });
            b.L (0, "coarse", 12); b.env (0, 0, b.rndExp (40, 120), 0, 40); break;
        case 10:  // warp: saw LFO on pitch through a delay
            b.source (0, 0.0f); b.env (0, 20, 2000, 0.8f, 600);
            b.set ("lfo1_shape", lsSawDown); b.set ("lfo1_rate", b.rnd (3, 9)); b.mod (msLfo1, mdPitch, b.rnd (0.15f, 0.4f));
            b.fx (0, 0, fxDelay, 0.25f, 0.5f, 0.5f, 0.4f);
            break;
        default:  // impact
            designDrum (b, 0); b.L (0, "penv", 48); b.L (0, "pdecay", 120); b.env (0, 0, 900, 0, 400);
            b.fx (0, 0, fxReverb, 0.8f, 0.5f, 1.0f, 0.5f);
            break;
    }
}

void designArp (Builder& b)
{
    const int prevCat = b.cat;
    b.cat = b.pick (std::vector<int> { cLead, cPluck, cBells, cKeys });
    b.source (0, 0.25f);
    b.cat = prevCat;
    b.env (0, 0, b.rndExp (100, 500), b.rnd (0, 0.5f), b.rndExp (60, 250));
    if (b.chance (0.5f)) b.filter (0, fLp12, b.rndExp (1200, 6000), b.rnd (0, 0.5f), b.rnd (0.1f, 0.5f), 1, 200, 0.2f, 100);
    b.L (0, "arp", amArp);
    b.L (0, "arpdir", (float) b.pick (std::vector<int> { adUp, adDown, adUpDown, adDownUp, adPlayed, adRandom, adConverge }));
    b.L (0, "arprate", (float) b.pick (std::vector<int> { 7, 7, 6, 11, 8, 10 }));
    b.L (0, "arpoct", (float) (1 + b.r.nextInt (3)));
    const int steps = b.pick (std::vector<int> { 8, 16, 16, 12 });
    b.L (0, "arpsteps", (float) steps);
    b.L (0, "arpgate", b.rnd (0.25f, 0.8f)); b.L (0, "arpswing", b.chance (0.3f) ? b.rnd (0.1f, 0.4f) : 0.0f);
    for (int s = 0; s < steps; ++s)
    {
        if (s % 4 != 0) b.p.steps.push_back ({ 0, "vel", s, b.rnd (0.45f, 0.85f) });
        if (b.chance (0.12f)) b.p.steps.push_back ({ 0, "on", s, 0.0f });
    }
    b.fx (0, 0, b.pick (std::vector<int> { fxDelay, fxConsoleEcho }), b.rnd (0.4f, 0.7f), b.rnd (0.25f, 0.5f), b.rnd (0, 0.7f), b.rnd (0.2f, 0.4f));
    if (b.chance (0.4f)) { b.L (1, "on", 1); b.source (1, 0.1f); b.L (1, "coarse", -12); b.L (1, "level", 0.4f); b.env (1, 20, 800, 0.6f, 400); }
}

void designSeq (Builder& b)
{
    static const int scales[5][7] = { { 0, 3, 5, 7, 10, 12, 15 }, { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 3, 5, 7, 9, 10 }, { 0, 2, 4, 5, 7, 9, 11 }, { 0, 1, 3, 5, 7, 8, 10 } };
    const int* sc = scales[b.e == 11 ? 4 : (b.e == 1 || b.e == 7) ? 3 : b.r.nextInt (3)];
    const int steps = 16;
    // L1 bass line
    const int prevCat = b.cat;
    b.cat = cBass; b.source (0, b.e == 11 ? 0.6f : 0.2f); b.cat = prevCat;
    b.env (0, 0, b.rndExp (120, 400), 0.0f, 60);
    b.L (0, "arp", amSeq); b.L (0, "arprate", 7); b.L (0, "arpsteps", steps); b.L (0, "arpgate", b.rnd (0.4f, 0.8f));
    for (int s = 0; s < steps; ++s)
    {
        const bool on = s % 4 == 0 || b.chance (0.35f);
        if (! on) { b.p.steps.push_back ({ 0, "on", s, 0.0f }); continue; }
        b.p.steps.push_back ({ 0, "note", s, (float) (sc[b.r.nextInt (5)] - (b.chance (0.2f) ? 12 : 0)) });
        if (s % 4 != 0) b.p.steps.push_back ({ 0, "vel", s, b.rnd (0.5f, 0.85f) });
    }
    // L2 hats
    b.L (1, "on", 1);
    b.L (1, "arp", amSeq); b.L (1, "arprate", b.e == 11 && b.chance (0.5f) ? 8 : 7); b.L (1, "arpsteps", steps); b.L (1, "arpgate", 0.3f);
    b.L (1, "coarse", 24); b.L (1, "level", 0.35f);
    for (int s = 0; s < steps; ++s) b.p.steps.push_back ({ 1, "vel", s, s % 2 == 0 ? 1.0f : b.rnd (0.3f, 0.6f) });
    // L3 kick / snare pattern
    b.L (2, "on", 1);
    b.L (2, "arp", amSeq); b.L (2, "arprate", 7); b.L (2, "arpsteps", steps); b.L (2, "arpgate", 0.5f);
    for (int s = 0; s < steps; ++s)
    {
        const bool kick = s == 0 || s == 10 || (s == 7 && b.chance (0.5f)) || (s == 3 && b.chance (0.3f));
        const bool snare = s == 4 || s == 12;
        if (! kick && ! snare) b.p.steps.push_back ({ 2, "on", s, 0.0f });
        else if (snare) b.p.steps.push_back ({ 2, "note", s, 12.0f });
    }
}
} // namespace

const char* categoryName (int c)
{
    static const char* n[] = { "BASS", "LEAD", "PAD", "KEYS", "PLUCK", "BELLS", "BRASS", "STRINGS", "ORGAN", "WINDS", "VOICE", "DRUMS", "PERC", "FX", "ARP", "SEQUENCE" };
    return n[juce::jlimit (0, numCategories - 1, c)];
}

const char* presetTypeName (int t)
{
    static const char* n[] = { "ONE-SHOT", "PHRASE", "LOOP" };
    return n[juce::jlimit (0, numPresetTypes - 1, t)];
}

namespace
{
int categoryFromSound (const SoundInfo& s, int slot, int eraIndex)
{
    const juce::String c (s.category);
    if (c == "FX") return cFx;
    if (c == "LOOP") return cSeq;
    if (c == "PHRASE") return categoryFromSound (era (eraIndex).sounds[slot - 3], slot - 3, eraIndex);
    static const std::map<juce::String, int> m { { "BASS", cBass }, { "LEAD", cLead }, { "PAD", cPad }, { "KEYS", cKeys }, { "PLUCK", cPluck },
                                                 { "BELLS", cBells }, { "BRASS", cBrass }, { "STRINGS", cStrings }, { "DRUMS", cDrums } };
    auto it = m.find (c);
    return it != m.end() ? it->second : cFx;
}

// position in the era -> (category, variant)
std::pair<int, int> slotOf (int j)
{
    int k = j - soundsPerEra;
    for (int c = 0; c < numCategories; ++c)
    {
        if (k < counts[c]) return { c, k };
        k -= counts[c];
    }
    return { cFx, 0 };
}

std::vector<PresetInfo> buildInfos()
{
    std::vector<PresetInfo> out ((size_t) numPresets);
    std::set<juce::String> used;
    for (int i = 0; i < numPresets; ++i)
    {
        const int e = i / presetsPerEra, j = i % presetsPerEra;
        auto& info = out[(size_t) i];
        info.era = e;
        if (j < soundsPerEra)
        {
            const auto& s = era (e).sounds[j];
            info.name = s.name;
            info.category = categoryFromSound (s, j, e);
            info.type = juce::String (s.category) == "LOOP" ? ptLoop : (j >= 3 && juce::String (s.category) != "FX" ? ptPhrase : ptShot);
            info.root = s.root;
            info.previewSeconds = 0.0f;   // the sample's own length
        }
        else
        {
            const auto [cat, v] = slotOf (j);
            info.category = cat;
            info.type = cat == cArp ? ptPhrase : (cat == cSeq ? ptLoop : ptShot);
            info.root = rootOf[cat];
            if (cat == cDrums && v % 8 == 0) info.root = 36;
            info.previewSeconds = cat == cArp || cat == cSeq ? 4.0f : (cat == cPad || cat == cStrings || cat == cVoice ? 3.0f : 1.6f);
            const juce::String noun = cat == cDrums ? juce::String (nouns[cDrums][v % 8]) : cat == cPerc ? juce::String (nouns[cPerc][v % 12])
                                    : cat == cFx ? juce::String (nouns[cFx][v % 12]) : juce::String (nouns[cat][(v * 5 + v / 25 + e * 7) % 12]);
            int a = (v * 7 + cat * 3) % 25;
            juce::String name;
            for (int tries = 0; tries < 25; ++tries)
            {
                name = juce::String (adjectives[e][(a + tries) % 25]) + " " + noun;
                if (used.count (name) == 0) break;
            }
            if (used.count (name) > 0)
                for (int k = 2; used.count (name) > 0; ++k) name = juce::String (adjectives[e][a]) + " " + noun + " " + juce::String (k);
            info.name = name;
        }
        if (used.count (info.name) > 0) info.name << " " << era (e).year;
        used.insert (info.name);
    }
    return out;
}
} // namespace

const PresetInfo& presetInfo (int index)
{
    static const std::vector<PresetInfo> infos = buildInfos();
    return infos[(size_t) juce::jlimit (0, numPresets - 1, index)];
}

Patch buildPatch (int index)
{
    Patch p;
    index = juce::jlimit (0, numPresets - 1, index);
    const int e = index / presetsPerEra, j = index % presetsPerEra;
    Builder b { p, juce::Random ((juce::int64) (index * 7919 + 104729)), e, 0, palette (e) };
    b.standardMacros();
    p.params.push_back ({ "l1_on", 1.0f });

    if (j < soundsPerEra)
    {
        b.L (0, "src", (float) chips::eraSample);
        b.L (0, "wave", (float) (e * soundsPerEra + j));
        return p;
    }
    const auto [cat, v] = slotOf (j);
    b.cat = cat;
    switch (cat)
    {
        case cBass: designBass (b); break;
        case cLead: designLead (b); break;
        case cPad: designPad (b); break;
        case cKeys: designKeys (b); break;
        case cPluck: designPluck (b); break;
        case cBells: designBells (b); break;
        case cBrass: designBrass (b); break;
        case cStrings: designStrings (b); break;
        case cOrgan: designOrgan (b); break;
        case cWinds: designWinds (b); break;
        case cVoice: designVoice (b); break;
        case cDrums: designDrum (b, v % 8); break;
        case cPerc: designPerc (b, v); break;
        case cFx: designFx (b, v); break;
        case cArp: designArp (b); break;
        default:
        {
            designSeq (b);
            // the hat / drum layers were designed on layer 1 (index 0) by designDrum: give layers 2 and 3 their own sources
            Patch tmp; Builder tb { tmp, juce::Random ((juce::int64) (index * 131 + 3)), e, cDrums, b.pal };
            designDrum (tb, 2);
            for (auto& pr : tmp.params) if (pr.first.startsWith ("l1_")) p.params.push_back ({ "l2_" + pr.first.substring (3), pr.second });
            Patch tmp2; Builder kb { tmp2, juce::Random ((juce::int64) (index * 137 + 5)), e, cDrums, b.pal };
            designDrum (kb, 0);
            for (auto& pr : tmp2.params) if (pr.first.startsWith ("l1_")) p.params.push_back ({ "l3_" + pr.first.substring (3), pr.second });
            // re-apply the sequencer settings that the drum design must not override
            p.params.push_back ({ "l2_arp", (float) amSeq }); p.params.push_back ({ "l3_arp", (float) amSeq });
            p.params.push_back ({ "l2_coarse", 24.0f }); p.params.push_back ({ "l2_level", 0.35f });
            p.params.push_back ({ "l3_level", 0.8f });
            break;
        }
    }
    // era colour on the whole preset
    if (b.chance (0.35f) && cat != cSeq) b.fx (-1, 1, b.pick (b.pal.masterFx), b.rnd (0.3f, 0.7f), b.rnd (0.3f, 0.6f), b.rnd (0.3f, 0.7f), b.rnd (0.15f, 0.35f));
    b.set ("space", b.rnd (0.05f, cat == cPad || cat == cStrings || cat == cBells ? 0.35f : 0.2f));
    return p;
}
} // namespace bitera
