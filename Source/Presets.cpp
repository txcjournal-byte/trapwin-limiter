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
    { "Underground", "Vault", "Rage", "Drill", "Phonk", "Cage", "Opium", "Block", "Smoke", "Concrete", "Plug", "Dark", "Cold", "Ice", "Shadow",
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
        L (l, "char", rnd (0.25f, 0.85f));
    }

    void chipSource (int l, std::initializer_list<int> preferred)
    {
        std::vector<int> avail;
        for (int s : preferred) if (std::find (pal.synths.begin(), pal.synths.end(), s) != pal.synths.end()) avail.push_back (s);
        if (avail.empty()) { source (l, 0.0f); return; }
        const int src = pick (avail);
        L (l, "src", (float) src);
        L (l, "wave", (float) pick (wavesFor (src, cat)));
        L (l, "char", rnd (0.25f, 0.85f));
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

// ---- game idioms -------------------------------------------------------------------------------------------------
// a looping note pattern on a layer (the classic chip "arpeggio chord", trills, octave bounces)
void pattern (Builder& b, int l, std::initializer_list<int> notes, int rate, int end = aeLoop, float gate = 1.0f)
{
    b.L (l, "arp", amSeq); b.L (l, "arprate", (float) rate); b.L (l, "arpsteps", (float) notes.size());
    b.L (l, "arpend", (float) end); b.L (l, "arpgate", gate);
    int s = 0;
    for (int n : notes) b.p.steps.push_back ({ l, "note", s++, (float) n });
}

const std::vector<std::vector<int>> chords { { 0, 4, 7 }, { 0, 3, 7 }, { 0, 4, 7, 12 }, { 0, 3, 7, 10 }, { 0, 5, 7 }, { 0, 4, 7, 11 }, { 0, 7, 12 }, { 0, 3, 7, 12 } };
void chordPattern (Builder& b, int l, int rate)
{
    const auto& c = chords[(size_t) b.r.nextInt ((int) chords.size())];
    b.L (l, "arp", amSeq); b.L (l, "arprate", (float) rate); b.L (l, "arpsteps", (float) c.size()); b.L (l, "arpgate", 1.0f);
    for (size_t i = 0; i < c.size(); ++i) b.p.steps.push_back ({ l, "note", (int) i, (float) c[i] });
}

constexpr int r64 = 12, r128 = 13, r32 = 8, r16 = 7, r8 = 6, r16t = 11;

void chipLead (Builder& b, int l = 0) { b.chipSource (l, { chips::console8, chips::handheld4, chips::psg, chips::computerSid, chips::polyCounter }); }

void designBass (Builder& b, int v)
{
    switch (v % 8)
    {
        case 0:   // triangle / sine sub: the console bass channel
            b.chipSource (0, { chips::console8, chips::handheld4, chips::computerSid, chips::psg });
            b.env (0, 1, b.rndExp (250, 900), b.rnd (0.5f, 0.9f), 60);
            break;
        case 1:   // octave bounce
            b.source (0, 0.2f); b.env (0, 0, 180, 0.6f, 50);
            pattern (b, 0, { 0, 12 }, b.chance (0.5f) ? r16 : r8, aeLoop, b.rnd (0.5f, 0.85f));
            break;
        case 2:   // plucked pulse bass with a filter snap
            b.source (0, 0.15f); b.env (0, 0, b.rndExp (120, 350), 0.0f, 60);
            b.filter (0, fLp24, b.rndExp (300, 1200), b.rnd (0.3f, 0.7f), b.rnd (0.4f, 0.8f), 1, b.rndExp (60, 200), 0, 80);
            break;
        case 3:   // slap / FM bass
            b.chipSource (0, { chips::fm16, chips::pcFm, chips::computerSid, chips::handheld4 });
            b.env (0, 0, b.rndExp (200, 600), b.rnd (0.2f, 0.5f), 80);
            b.L (0, "penv", b.rnd (0.5f, 3.0f)); b.L (0, "pdecay", 25);
            break;
        case 4:   // 808-style glide bass: pitch punch, long tail, glides between notes
            b.chipSource (0, { chips::handheld4, chips::console8, chips::computerSid, chips::fm16, chips::psg });
            if (b.e >= 9 && b.chance (0.5f)) { b.L (0, "src", (float) chips::eraSample); b.L (0, "wave", 11 * soundsPerEra + 1); }
            b.env (0, 0, b.rndExp (900, 2500), 0.0f, 300);
            b.L (0, "penv", b.rnd (7, 14)); b.L (0, "pdecay", b.rnd (30, 70));
            b.L (0, "mode", vmLegato); b.L (0, "glide", b.rnd (40, 110));
            b.fx (0, 0, fxDrive, b.rnd (0.15f, 0.4f), 0.0f, 0.6f, b.rnd (0.3f, 0.7f));
            break;
        case 5:   // wobble: synced LFO on a resonant filter
            b.source (0, 0.1f); b.env (0, 1, 800, 0.9f, 120);
            b.filter (0, fLp24, b.rndExp (300, 900), b.rnd (0.45f, 0.75f), 0.0f, 1, 300, 1, 100);
            b.set ("lfo1_sync", (float) b.pick (std::vector<int> { 6, 7, 10 })); b.set ("lfo1_shape", (float) b.pick (std::vector<int> { lsSine, lsTriangle, lsSquare }));
            b.mod (msLfo1, mdCutoff, b.rnd (0.25f, 0.45f));
            break;
        case 6:   // buzz bass
            b.chipSource (0, { chips::psg, chips::polyCounter, chips::console8, chips::handheld4 });
            b.env (0, 0, 600, 0.8f, 60);
            b.fx (0, 0, fxDrive, b.rnd (0.3f, 0.6f), b.rnd (0.0f, 0.6f), 0.5f, 0.8f);
            break;
        default:  // detuned double bass (reese)
            for (int l = 0; l < 2; ++l)
            {
                b.L (l, "on", 1); b.source (l, 0.05f); b.env (l, 3, 900, 0.85f, 120);
                b.L (l, "fine", l == 0 ? -b.rnd (8, 16) : b.rnd (8, 16)); b.L (l, "level", 0.6f); b.L (l, "pan", l == 0 ? -0.2f : 0.2f);
                b.filter (l, fLp24, b.rndExp (500, 1600), 0.3f, 0.2f, 1, 300, 0.4f, 100);
            }
            break;
    }
    if (v % 8 != 7 && b.chance (0.35f))   // sub layer
    {
        b.L (1, "on", 1); b.chipSource (1, { chips::console8, chips::handheld4, chips::computerSid, chips::psg });
        b.L (1, "coarse", -12); b.L (1, "level", b.rnd (0.35f, 0.55f)); b.env (1, 1, 700, 0.7f, 100);
    }
    b.L (0, "vel", b.rnd (0.2f, 0.6f));
}

void designLead (Builder& b, int v)
{
    switch (v % 10)
    {
        case 0:   // classic pulse lead with a late vibrato
            chipLead (b); b.env (0, 2, 800, 0.8f, 120);
            b.L (0, "vibdepth", b.rnd (12, 30)); b.L (0, "vibrate", b.rnd (5.0f, 7.0f));
            break;
        case 1:   // duty / pulse-width motion
            b.chipSource (0, { chips::console8, chips::computerSid, chips::handheld4 });
            b.env (0, 2, 800, 0.85f, 150);
            b.set ("lfo2_rate", b.rnd (0.4f, 3.0f)); b.set ("lfo2_shape", lsTriangle);
            b.mod (msLfo1 + 1, mdChar, b.rnd (0.3f, 0.6f));
            break;
        case 2:   // echo lead: the "second channel" echo
            chipLead (b); b.env (0, 2, 600, 0.75f, 150);
            b.fx (0, 0, fxDelay, b.pick (std::vector<float> { 0.58f, 0.67f, 0.42f }), b.rnd (0.35f, 0.55f), 0.45f, b.rnd (0.3f, 0.45f));
            b.L (0, "vibdepth", b.rnd (6, 18));
            break;
        case 3:   // chip chord: one key plays a whole chord as a super fast arpeggio
            chipLead (b); b.env (0, 0, 900, 0.9f, 120);
            chordPattern (b, 0, b.chance (0.6f) ? r64 : r128);
            break;
        case 4:   // octave double
            chipLead (b); b.env (0, 2, 800, 0.8f, 150);
            b.L (1, "on", 1); chipLead (b, 1);
            b.L (1, "coarse", 12); b.L (1, "level", 0.4f); b.env (1, 2, 800, 0.8f, 150);
            b.L (0, "vibdepth", 10); b.L (1, "vibdepth", 14);
            break;
        case 5:   // portamento lead
            b.source (0, 0.15f); b.env (0, 5, 900, 0.9f, 150);
            b.L (0, "mode", vmLegato); b.L (0, "glide", b.rnd (60, 180));
            b.L (0, "vibdepth", b.rnd (10, 25));
            break;
        case 6:   // bend-in lead: every note slides up into pitch
            chipLead (b); b.env (0, 2, 700, 0.8f, 120);
            b.L (0, "penv", -b.rnd (1.0f, 3.0f)); b.L (0, "pdecay", b.rnd (50, 120));
            b.L (0, "vibdepth", b.rnd (8, 20));
            break;
        case 7:   // FM / metal / sync lead with drive
            b.chipSource (0, { chips::fm16, chips::computerSid, chips::pcFm, chips::handheld4 });
            b.env (0, 2, 900, 0.8f, 150);
            b.fx (0, 0, fxDrive, b.rnd (0.2f, 0.45f), b.rnd (0.0f, 0.8f), 0.7f, 0.6f);
            break;
        case 8:   // square + noise breath
            chipLead (b); b.env (0, 15, 700, 0.85f, 150);
            b.L (1, "on", 1); b.L (1, "src", (float) chips::psg); b.L (1, "wave", 3); b.L (1, "level", 0.2f);
            b.filter (1, fBp12, 3000, 0.3f, 0, 1, 200, 0, 100); b.env (1, 10, 300, 0.4f, 100);
            b.L (0, "vibdepth", 12);
            break;
        default:  // trill
            chipLead (b); b.env (0, 1, 800, 0.85f, 120);
            pattern (b, 0, { 0, b.chance (0.5f) ? 1 : 2 }, b.chance (0.5f) ? r64 : r32);
            break;
    }
    if (b.chance (0.3f)) b.layerFx (0, 1, b.rnd (0.25f, 0.6f));
}

void designPad (Builder& b, int v)
{
    const int kind = v % 6;
    const int layers = kind == 0 ? 1 : 2;
    for (int l = 0; l < layers; ++l)
    {
        b.L (l, "on", 1); b.source (l, 0.3f);
        b.env (l, b.rndExp (300, 1800), 2000, b.rnd (0.7f, 1.0f), b.rndExp (900, 3500));
        b.L (l, "fine", l == 0 ? 0.0f : b.rnd (6, 14));
        b.L (l, "level", l == 0 ? 0.6f : 0.45f); b.L (l, "pan", l == 0 ? -0.25f : 0.25f);
    }
    switch (kind)
    {
        case 0: chordPattern (b, 0, r64); b.fx (0, 0, fxConsoleEcho, 0.3f, 0.45f, 0.5f, 0.35f); break;   // chord-arp pad
        case 1: b.set ("lfo2_rate", b.rnd (0.15f, 0.8f)); b.mod (msLfo1 + 1, mdChar, 0.45f); break;   // PWM pad
        case 2: for (int l = 0; l < 2; ++l) b.L (l, "vibdepth", b.rnd (6, 14)); b.fx (0, 0, fxChorus, 0.25f, 0.6f, 0.6f, 0.5f); break;
        case 3: b.L (1, "coarse", 12); b.fx (0, 0, fxReverb, 0.8f, 0.3f, 1.0f, 0.5f); break;   // shimmer octave
        case 4:   // slow filter sweep
            for (int l = 0; l < 2; ++l) b.filter (l, fLp24, b.rndExp (400, 1500), b.rnd (0.3f, 0.6f), 0.0f, 1, 300, 1, 300);
            b.set ("lfo1_rate", b.rnd (0.05f, 0.25f)); b.set ("lfo1_retrig", 0); b.mod (msLfo1, mdCutoff, b.rnd (0.3f, 0.5f));
            break;
        default:  // tremolo pad
            b.fx (0, 0, fxTremolo, b.rnd (0.45f, 0.7f), b.rnd (0.3f, 0.6f), b.chance (0.5f) ? 0.8f : 0.2f, 1.0f);
            break;
    }
    b.fx (-1, 0, fxReverb, b.rnd (0.5f, 0.85f), b.rnd (0.3f, 0.6f), 1.0f, b.rnd (0.25f, 0.45f));
}

void designKeys (Builder& b, int v)
{
    switch (v % 5)
    {
        case 0: b.chipSource (0, { chips::fm16, chips::pcFm, chips::handheld4, chips::computerSid }); b.env (0, 0, b.rndExp (900, 2500), 0.1f, 300); break;   // FM e-piano
        case 1:   // chip piano: pulse with a bell tine
            b.chipSource (0, { chips::console8, chips::handheld4, chips::psg, chips::computerSid }); b.env (0, 0, b.rndExp (400, 1200), 0.0f, 200);
            b.filter (0, fLp12, 3000, 0.1f, 0.4f, 1, 300, 0, 200);
            b.L (1, "on", 1); b.chipSource (1, { chips::handheld4, chips::console8, chips::fm16, chips::computerSid });
            b.L (1, "coarse", 12); b.L (1, "level", 0.3f); b.env (1, 0, 300, 0, 150);
            break;
        case 2: b.chipSource (0, { chips::pcFm, chips::fm16, chips::computerSid, chips::console8 }); b.env (0, 0, b.rndExp (300, 800), 0.0f, 120); break;   // clav
        case 3:   // toy piano
            b.source (0, 0.3f); b.L (0, "coarse", 12); b.env (0, 0, b.rndExp (300, 900), 0.0f, 200);
            b.fx (0, 0, fxReverb, 0.4f, 0.4f, 1.0f, 0.3f);
            break;
        default: b.source (0, 0.6f); b.env (0, 0, b.rndExp (900, 3000), 0.2f, 300); break;   // era piano / sampled keys
    }
    b.L (0, "vel", b.rnd (0.5f, 0.9f));
    b.mod (msVelocity, mdCutoff, 0.35f);
}

void designPluck (Builder& b, int v)
{
    switch (v % 6)
    {
        case 0: chipLead (b); b.env (0, 0, b.rndExp (120, 400), 0, 100); b.fx (0, 0, fxDelay, 0.58f, 0.45f, 0.45f, 0.35f); break;
        case 1: b.chipSource (0, { chips::computerSid, chips::console8, chips::handheld4 });
                b.env (0, 0, 500, 0, 150); b.filter (0, fLp24, b.rndExp (300, 900), b.rnd (0.4f, 0.75f), b.rnd (0.5f, 0.9f), 1, b.rndExp (60, 220), 0, 100); break;
        case 2: b.chipSource (0, { chips::fm16, chips::pcFm, chips::handheld4 }); b.env (0, 0, b.rndExp (200, 600), 0, 150); break;
        case 3: b.source (0, 0.3f); b.env (0, 0, b.rndExp (300, 900), 0, 200); b.L (0, "penv", b.rnd (0.6f, 2.0f)); b.L (0, "pdecay", 40); break;   // koto bend
        case 4: b.source (0, 0.4f); b.env (0, 0, b.rndExp (500, 1500), 0, 400); b.fx (0, 0, fxReverb, 0.6f, 0.4f, 1.0f, 0.35f); break;   // harp
        default:  // kalimba
            b.chipSource (0, { chips::handheld4, chips::fm16, chips::console8, chips::psg }); b.L (0, "coarse", 12);
            b.env (0, 0, b.rndExp (250, 600), 0, 200); b.fx (0, 0, fxConsoleEcho, 0.25f, 0.35f, 0.4f, 0.3f);
            break;
    }
}

void designBells (Builder& b, int v)
{
    switch (v % 5)
    {
        case 0: b.chipSource (0, { chips::fm16, chips::pcFm, chips::handheld4 }); b.env (0, 0, b.rndExp (1200, 3500), 0, 1200); break;
        case 1:   // music box
            b.source (0, 0.4f); b.L (0, "coarse", 12); b.env (0, 0, b.rndExp (600, 1500), 0, 600);
            b.fx (0, 0, fxReverb, 0.6f, 0.3f, 1.0f, 0.35f); break;
        case 2: b.chipSource (0, { chips::computerSid, chips::fm16, chips::handheld4 }); b.L (0, "wave", 7); b.env (0, 0, 1500, 0, 800); break;   // ring bell
        case 3: b.chipSource (0, { chips::console8, chips::handheld4, chips::psg }); b.L (0, "coarse", 12); b.env (0, 0, b.rndExp (200, 500), 0, 200); break;   // glock
        default:  // chime run: one key rings a little up-run
            b.source (0, 0.3f); b.env (0, 0, 700, 0, 500);
            pattern (b, 0, { 0, 7, 12, 19 }, r32, aeOneShot, 0.9f);
            break;
    }
    b.fx (-1, 0, fxReverb, b.rnd (0.4f, 0.8f), b.rnd (0.2f, 0.5f), 1.0f, b.rnd (0.25f, 0.45f));
}

const char* fxNouns[20] = { "Coin", "1-Up", "Jump", "Power-Up", "Hurt", "Laser", "Explosion", "Menu Select", "Menu Back", "Pause",
                            "Teleport", "Game Over", "Stage Clear", "Fireball", "Splash", "Alarm", "Hover Engine", "Text Blip", "Boss Roar", "Charge Up" };

void designFx (Builder& b, int v)
{
    auto chip = [&] (int l)   // a tonal chip voice (lead waves, not noise)
    {
        const int keep = b.cat; b.cat = cLead;
        b.chipSource (l, { chips::console8, chips::handheld4, chips::psg, chips::computerSid, chips::polyCounter, chips::fm16 });
        b.cat = keep;
    };
    auto noise = [&] (int l) { b.L (l, "src", (float) chips::psg); b.L (l, "wave", 3); };
    switch (v % 20)
    {
        case 0: chip (0); b.env (0, 0, 400, 0.7f, 250); pattern (b, 0, { 0, 5 }, r16, aeHold, 1.0f); b.L (0, "coarse", 12); break;   // coin: B -> E
        case 1: chip (0); b.env (0, 0, 120, 0.8f, 60); pattern (b, 0, { 0, 4, 7, 12, 16, 19 }, r16, aeOneShot, 0.9f); b.L (0, "coarse", 12); break;   // 1-up
        case 2: chip (0); b.env (0, 0, 220, 0, 60); b.L (0, "penv", -b.rnd (5, 12)); b.L (0, "pdecay", b.rnd (60, 140)); break;   // jump
        case 3: chip (0); b.env (0, 0, 100, 0.6f, 60); pattern (b, 0, { 0, 7, 12, 4, 11, 16, 7, 14, 19, 12, 19, 24 }, r64, aeOneShot, 0.9f); break;   // power-up
        case 4: chip (0); b.env (0, 0, 300, 0, 80); b.L (0, "penv", 7); b.L (0, "pdecay", 120);   // hurt
                b.L (1, "on", 1); noise (1); b.env (1, 0, 150, 0, 50); b.L (1, "level", 0.4f); break;
        case 5: chip (0); b.L (0, "penv", b.rnd (24, 40)); b.L (0, "pdecay", b.rnd (60, 200)); b.env (0, 0, b.rndExp (150, 400), 0, 80);   // laser
                b.fx (0, 0, fxDelay, 0.3f, 0.4f, 0.0f, 0.25f); break;
        case 6: noise (0); b.env (0, 0, b.rndExp (900, 2000), 0, 600); b.filter (0, fLp24, b.rndExp (1500, 5000), 0.2f, -0.5f, 1, 900, 0, 500);   // explosion
                b.L (1, "on", 1); chip (1); b.L (1, "coarse", -24); b.L (1, "penv", 12); b.L (1, "pdecay", 200); b.env (1, 0, 600, 0, 300); b.L (1, "level", 0.5f);
                b.fx (0, 0, fxBite, 0.35f, 0.4f, 0.2f, 0.6f); break;
        case 7: chip (0); b.L (0, "coarse", 24); b.env (0, 0, 60, 0, 30); pattern (b, 0, { 0, 12 }, r64, aeOneShot, 0.9f); break;   // menu select
        case 8: chip (0); b.L (0, "coarse", 12); b.env (0, 0, 70, 0, 30); pattern (b, 0, { 7, 0 }, r64, aeOneShot, 0.9f); break;   // menu back
        case 9: chip (0); b.L (0, "coarse", 12); b.env (0, 0, 160, 0, 80); pattern (b, 0, { 0, 12, 0, 12 }, r32, aeOneShot, 0.8f); break;   // pause
        case 10: chip (0); b.env (0, 10, 1200, 0.8f, 500);   // teleport
                 b.set ("lfo1_shape", lsSawUp); b.set ("lfo1_rate", b.rnd (6, 14)); b.mod (msLfo1, mdPitch, b.rnd (0.2f, 0.45f));
                 b.L (0, "penv", -12); b.L (0, "pdecay", 600); b.fx (0, 0, fxPhaser, 0.6f, 0.7f, 0.6f, 0.6f); break;
        case 11: chip (0); b.env (0, 2, 300, 0.8f, 200); pattern (b, 0, { 12, 7, 4, 0, -5, -12 }, r8, aeOneShot, 0.85f); break;   // game over
        case 12: chip (0); b.env (0, 2, 200, 0.8f, 150); pattern (b, 0, { 0, 4, 7, 12, 7, 12, 16, 19, 24 }, r16, aeOneShot, 0.8f); break;   // stage clear
        case 13: noise (0); b.env (0, 0, 500, 0, 200); b.filter (0, fLp12, 6000, 0.4f, -0.8f, 1, 400, 0, 200);   // fireball
                 b.L (1, "on", 1); chip (1); b.L (1, "penv", 12); b.L (1, "pdecay", 150); b.env (1, 0, 250, 0, 100); b.L (1, "level", 0.5f); break;
        case 14: noise (0); b.env (0, 5, 700, 0, 300); b.filter (0, fBp12, 1500, 0.6f, 0.4f, 1, 300, 0, 200);   // splash
                 b.set ("lfo1_rate", 9); b.set ("lfo1_shape", lsSampleHold); b.mod (msLfo1, mdCutoff, 0.3f); break;
        case 15: chip (0); b.env (0, 0, 500, 1, 100); b.set ("lfo1_shape", lsSquare); b.set ("lfo1_rate", b.rnd (3, 8)); b.mod (msLfo1, mdPitch, b.rnd (0.15f, 0.3f)); break;   // alarm
        case 16: b.chipSource (0, { chips::polyCounter, chips::psg, chips::console8, chips::computerSid }); b.L (0, "coarse", -12);   // hover engine
                 b.env (0, 80, 1000, 1, 300); b.set ("lfo1_rate", b.rnd (5, 12)); b.mod (msLfo1, mdPitch, 0.05f); b.mod (msLfo1, mdLevel, -0.3f); break;
        case 17: chip (0); b.L (0, "coarse", 12); b.env (0, 0, 35, 0, 20); b.mod (msRandom, mdPitch, 0.25f); break;   // text blip: each note a random pitch
        case 18: for (int l = 0; l < 2; ++l) { b.L (l, "on", 1); chip (l); b.L (l, "coarse", -12); b.L (l, "fine", l ? 30.0f : -30.0f); b.env (l, 30, 1200, 0, 400);   // boss roar
                     b.L (l, "penv", 5); b.L (l, "pdecay", 900); }
                 b.fx (0, 0, fxDrive, 0.6f, 0.5f, 0.4f, 0.8f); b.fx (0, 1, fxRingMod, 0.2f, 0.5f, 0.3f, 0.4f); break;
        default: chip (0); b.L (0, "penv", -24); b.L (0, "pdecay", b.rnd (900, 1800)); b.env (0, 300, 2500, 1, 200);   // charge up
                 b.set ("lfo1_rate", 14); b.mod (msLfo1, mdLevel, -0.4f); break;
    }
}

void designArp (Builder& b, int v)
{
    const int prevCat = b.cat;
    b.cat = b.pick (std::vector<int> { cLead, cPluck, cBells, cKeys });
    b.source (0, 0.2f);
    b.cat = prevCat;
    b.env (0, 0, b.rndExp (100, 400), b.rnd (0, 0.5f), b.rndExp (60, 200));
    switch (v % 6)
    {
        case 0: b.L (0, "arp", amArp); b.L (0, "arpdir", adUp); b.L (0, "arprate", (float) r64); b.L (0, "arpoct", 1); b.L (0, "arpgate", 1.0f); break;   // chord arp from held keys
        case 1: b.L (0, "arp", amArp); b.L (0, "arpdir", adUp); b.L (0, "arprate", (float) r16); b.L (0, "arpoct", 2); break;
        case 2: b.L (0, "arp", amArp); b.L (0, "arpdir", adUpDown); b.L (0, "arprate", (float) r16t); b.L (0, "arpoct", 2); break;
        case 3: b.L (0, "arp", amArp); b.L (0, "arpdir", adRandom); b.L (0, "arprate", (float) r32); b.L (0, "arpoct", 3); break;
        case 4: b.L (0, "arp", amArp); b.L (0, "arpdir", adConverge); b.L (0, "arprate", (float) r16); b.L (0, "arpoct", 2); b.L (0, "arpswing", 0.35f); break;
        default:  // gallop
            b.L (0, "arp", amArp); b.L (0, "arpdir", adUp); b.L (0, "arprate", (float) r16); b.L (0, "arpoct", 1); b.L (0, "arpsteps", 4);
            b.p.steps.push_back ({ 0, "on", 1, 0.0f }); break;
    }
    if (v % 6 != 5) b.L (0, "arpsteps", (float) b.pick (std::vector<int> { 8, 16, 12 }));
    b.L (0, "arpgate", v % 6 == 0 ? 1.0f : b.rnd (0.3f, 0.8f));
    for (int s = 0; s < 16; ++s) if (s % 4 != 0) b.p.steps.push_back ({ 0, "vel", s, b.rnd (0.5f, 0.85f) });
    b.fx (0, 0, b.pick (std::vector<int> { fxDelay, fxConsoleEcho }), b.rnd (0.4f, 0.7f), b.rnd (0.25f, 0.5f), b.rnd (0, 0.7f), b.rnd (0.2f, 0.4f));
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
    static const char* n[] = { "ONE-SHOT", "SUSTAIN", "PHRASE", "LOOP" };
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
            const bool sustained = cat == cBass || cat == cLead || cat == cPad || cat == cBrass || cat == cStrings || cat == cOrgan || cat == cWinds || cat == cVoice;
            info.type = cat == cArp ? ptPhrase : (cat == cSeq ? ptLoop : (sustained ? ptSustain : ptShot));
            info.root = rootOf[cat];
            if (cat == cDrums && v % 8 == 0) info.root = 36;
            info.previewSeconds = cat == cArp || cat == cSeq ? 4.0f : (cat == cPad || cat == cStrings || cat == cVoice ? 3.0f : 1.6f);
            const juce::String noun = cat == cDrums ? juce::String (nouns[cDrums][v % 8]) : cat == cPerc ? juce::String (nouns[cPerc][v % 12])
                                    : cat == cFx ? juce::String (fxNouns[v % 20]) : juce::String (nouns[cat][(v * 5 + v / 25 + e * 7) % 12]);
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
        case cBass: designBass (b, v); break;
        case cLead: designLead (b, v); break;
        case cPad: designPad (b, v); break;
        case cKeys: designKeys (b, v); break;
        case cPluck: designPluck (b, v); break;
        case cBells: designBells (b, v); break;
        case cBrass: designBrass (b); break;
        case cStrings: designStrings (b); break;
        case cOrgan: designOrgan (b); break;
        case cWinds: designWinds (b); break;
        case cVoice: designVoice (b); break;
        case cDrums: designDrum (b, v % 8); break;
        case cPerc: designPerc (b, v); break;
        case cFx: designFx (b, v); break;
        case cArp: designArp (b, v); break;
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
