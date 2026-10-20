#include "Presets.h"
#include "Engine.h"
#include "Instruments.h"
#include <set>
#include <map>

// BIT ERA factory bank, v0.6: every era is a game platform (or a family of them), and every preset is built the way
// game musicians built their instruments: a chip channel (or a console ROM sample), tracker macros at 60 Hz
// (volume, arpeggio, duty / wave, pitch), the platform's own echo / output, a little modulation for the MACRO page.
namespace bitera
{
using namespace chips;
namespace
{
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
    { "Lead", "Hero", "Solo", "Line", "Melody", "Cry", "Theme", "Whistle", "Saw", "Shine", "Blade", "Call" },
    { "Pad", "Drift", "Cloud", "Haze", "Horizon", "Field", "Glow", "Veil", "Bloom", "Mist", "Atmos", "Tide" },
    { "Keys", "Piano", "Upright", "Grand", "E-Piano", "Clav", "Ivory", "Parlor Keys", "Tines", "Felt Keys", "Chord Keys", "Hammer" },
    { "Pluck", "Pizz", "Harp", "Pick", "Snap", "Drop", "Kalimba", "Koto", "Zither", "Twang", "Flick", "Guitar" },
    { "Bells", "Chime", "Celesta", "Glockenspiel", "Music Box", "Gong", "Toll", "Carillon", "Tinkle", "Shimmer", "Crystal", "Marimba" },
    { "Brass", "Horns", "Fanfare", "Stab", "Section", "Trumpet", "Blast", "Brass Hit", "Tuba", "Call", "Charge", "Salute" },
    { "Strings", "Violins", "Ensemble", "Cellos", "Section", "Quartet", "Bows", "Tremolo", "Legato", "Orchestra", "Violas", "Swell" },
    { "Organ", "Drawbar", "Chapel", "Combo Organ", "Pipe", "Reed", "Harmonium", "Calliope", "Rotary", "Theatre Organ", "Church", "Fairground" },
    { "Flute", "Whistle", "Ocarina", "Pan Pipe", "Recorder", "Shakuhachi", "Clarinet", "Oboe", "Piccolo", "Breath", "Reed", "Fife" },
    { "Pulse", "Square", "Triangle", "Saw", "Wave", "Tone", "Beep", "Bleep", "Blip", "Chirp", "Buzz", "Ping" },
    { "Kick", "Snare", "Hat", "Open Hat", "Tom", "Clap", "Rim", "Crash", "Kick", "Snare", "Hat", "Tom" },
    { "Cowbell", "Woodblock", "Blip", "Shaker", "Conga", "Triangle", "Clave", "Tick", "Agogo", "Bongo", "Cabasa", "Ping" },
    { "Laser", "Riser", "Drop", "Explosion", "Coin", "Power-Up", "Jump", "Zap", "Alarm", "UI Blip", "Warp", "Impact" },
    { "Arp", "Run", "Cascade", "Spiral", "Climb", "Ladder", "Sparkle", "Ripple", "Chase", "Stairs", "Gallop", "Twirl" },
    { "Loop", "Groove", "Pattern", "Sequence", "March", "Bounce", "Shuffle", "Engine", "Drive", "Pulse", "Cycle", "Rhythm" },
};

const char* fxNouns[20] = { "Coin", "1-Up", "Jump", "Power-Up", "Hurt", "Laser", "Explosion", "Menu Select", "Menu Back", "Pause",
                            "Teleport", "Game Over", "Stage Clear", "Fireball", "Splash", "Alarm", "Hover Engine", "Text Blip", "Boss Roar", "Charge Up" };

const int counts[numCategories] = { 70, 70, 45, 40, 45, 35, 30, 25, 20, 20, 30, 60, 30, 45, 20, 15 };   // = 600
const int rootOf[numCategories] = { 36, 60, 60, 60, 60, 72, 60, 60, 60, 72, 60, 48, 60, 60, 60, 48 };
// the first 6 presets of every era: its showcase
const int showcase[6] = { cLead, cBass, cArp, cSeq, cFx, cPad };

// ---------------------------------------------------------------------------------------------------------------
// the platforms
struct Platform
{
    const char* name;
    std::vector<int> chips;       // synth chips
    std::vector<int> samplers;    // sampler sources (they play the ROM instruments)
    std::vector<int> fx;          // the platform's typical effects
    bool echo = false;            // console echo on the master (16-bit sampler consoles)
};

const Platform& platform (int e)
{
    static const std::vector<Platform> p = {
        { "TTL ARCADE + POLY COUNTER CONSOLE", { ttl, polyCounter }, {}, { fxBite, fxDelay } },
        { "POKEY COMPUTER + 16-BIT COMPUTER PSG", { pokey, psg }, {}, { fxDelay, fxChorus } },
        { "8-BIT SAMPLER COMPUTER + TRACKER + SOFT SYNTH", { softSynth }, { sampler8 }, { fxDelay, fxChorus } },
        { "SID HOME COMPUTER", { computerSid }, {}, { fxDelay } },
        { "PSG COMPUTERS + WAVETABLE CART", { psg, scc }, {}, { fxDelay, fxChorus } },
        { "8-BIT CONSOLE + EXPANSIONS", { console8, vrc6, opll, diskWave }, {}, { fxDelay } },
        { "16-BIT FM CONSOLE + PC FM CARD", { fm16, pcFm, psg }, { sampler8 }, { fxChorus, fxDelay } },
        { "16-BIT SAMPLER CONSOLE", {}, { sampler16 }, { fxConsoleEcho, fxChorus }, true },
        { "4-BIT HANDHELD", { handheld4 }, {}, { fxDelay } },
        { "32-BIT CD CONSOLE + 64-BIT CARTRIDGE + 90s ROMPLER", {}, { sampler32, console64, rompler90s }, { fxReverb, fxChorus, fxDelay } },
        { "32-BIT HANDHELD", { handheld4 }, { handheldPcm }, { fxDelay, fxReverb } },
        { "MODERN RETRO (2023+)", { console8, vrc6, handheld4, computerSid, fm16, scc, psg }, { sampler16, sampler32 }, { fxDrive, fxWowFlutter, fxBite, fxDelay, fxReverb } },
    };
    return p[(size_t) juce::jlimit (0, numEras - 1, e)];
}

// chip channels that suit a category
std::vector<int> chipWaves (int src, int cat)
{
    const bool bass = cat == cBass, lead = cat == cLead || cat == cWinds || cat == cBrass, pad = cat == cPad || cat == cStrings || cat == cOrgan,
               keys = cat == cKeys || cat == cPluck || cat == cBells || cat == cArp, drum = cat == cDrums || cat == cFx;
    switch (src)
    {
        case console8: if (bass) return { 4, 4, 2 }; if (drum) return { 5, 6, 4 }; if (cat == cWaves) return { 0, 1, 2, 4 }; return { 0, 1, 2, 7 };
        case vrc6: if (bass) return { 8, 3, 7 }; if (pad) return { 8, 9, 6 }; return { 1, 3, 5, 7, 8 };
        case opll:
            if (bass) return { 12, 13 }; if (cat == cBrass) return { 6, 8 }; if (cat == cWinds) return { 3, 4, 5 }; if (cat == cOrgan) return { 7 };
            if (cat == cStrings || cat == cPad) return { 0, 7, 3 }; if (cat == cPluck) return { 1, 14, 10 }; if (cat == cBells) return { 11, 2 };
            if (cat == cKeys) return { 2, 10, 11 }; return { 9, 0, 6, 14 };
        case diskWave: if (bass) return { 2, 1 }; if (cat == cBells) return { 4, 0 }; if (cat == cWinds) return { 6, 0 }; if (cat == cOrgan) return { 3 }; return { 1, 2, 5, 7, 3 };
        case scc: if (bass) return { 7, 0 }; if (pad) return { 5, 2, 6 }; if (cat == cBrass) return { 4, 0 }; if (cat == cWinds) return { 3, 6 }; return { 0, 1, 2, 4 };
        case pokey: if (bass) return { 7, 1 }; if (drum) return { 4, 2, 5 }; if (cat == cPad) return { 6, 1 }; return { 0, 1, 6, 3 };
        case ttl: if (drum) return { 2, 3 }; if (cat == cFx) return { 4, 0 }; return { 0, 1 };
        case softSynth: if (bass) return { 3, 2 }; if (pad) return { 6, 4, 0 }; if (cat == cBells) return { 0, 1 }; return { 2, 3, 5, 7 };
        case handheld4:
            if (bass) return { 10, 4, 2 }; if (drum) return { 12, 13 }; if (pad) return { 5, 6, 11, 9 }; if (cat == cBells) return { 7, 5 };
            if (cat == cWinds) return { 5, 9 }; return { 0, 1, 2, 4, 8 };
        case computerSid:
            if (bass) return { 0, 2, 4 }; if (drum) return { 3 }; if (pad) return { 0, 2, 4 }; if (cat == cBells) return { 7, 1 };
            if (cat == cWinds) return { 1, 6 }; return { 2, 0, 5, 8, 4 };
        case fm16:
            if (bass) return { 1, 8 }; if (keys) return { 0, 3, 6 }; if (cat == cBrass) return { 2 }; if (pad) return { 9, 5 }; if (cat == cOrgan) return { 5 };
            if (drum) return { 7, 8 }; return { 4, 2, 7 };
        case pcFm:
            if (bass) return { 1 }; if (keys) return { 4, 7, 5 }; if (cat == cBrass) return { 2 }; if (cat == cWinds) return { 3 }; if (pad) return { 0 };
            return { 6, 2, 3 };
        case psg: if (bass) return { 2, 0 }; if (drum) return { 3, 4 }; if (pad) return { 5, 6, 0 }; return { 0, 1, 7 };
        case polyCounter: if (bass) return { 1, 0 }; if (drum) return { 4, 3 }; return { 0, 5 };
        default: return { 0 };
    }
}

// ROM instruments that suit a category (sampler platforms)
std::vector<int> instrumentsFor (int cat)
{
    switch (cat)
    {
        case cBass: return { insFingerBass, insSlapBass, insFingerBass, insSawLead };
        case cLead: return { insSquareLead, insSawLead, insTrumpet, insSax, insDriveGuitar, insFlute };
        case cPad: return { insStrings, insChoirAah, insChoirOoh, insSinePad, insOrgan };
        case cKeys: return { insPiano, insEPiano, insVibraphone, insPiano };
        case cPluck: return { insNylonGuitar, insSteelGuitar, insHarp, insPizzicato, insMarimba };
        case cBells: return { insGlock, insMusicBox, insVibraphone, insSteelDrum, insMarimba };
        case cBrass: return { insBrass, insTrumpet, insOrchHit };
        case cStrings: return { insStrings, insStrings, insPizzicato };
        case cOrgan: return { insOrgan };
        case cWinds: return { insFlute, insClarinet, insSax };
        case cWaves: return { insSquareLead, insSawLead, insSinePad, insChoirOoh };
        case cPerc: return { insMarimba, insSteelDrum, insTimpani, insRim, insClap, insGlock };
        case cArp: return { insPiano, insHarp, insGlock, insSquareLead, insMarimba, insEPiano };
        default: return { insSquareLead };
    }
}

// ---------------------------------------------------------------------------------------------------------------
struct Builder
{
    Patch& p;
    juce::Random r;
    int e, cat;
    const Platform& pf;
    int modSlot = 0;

    void set (const juce::String& id, float v) { p.params.push_back ({ id, v }); }
    void L (int l, const char* n, float v) { set ("l" + juce::String (l + 1) + "_" + n, v); }
    float rnd (float a, float b) { return a + (b - a) * r.nextFloat(); }
    float rndExp (float a, float b) { return a * std::pow (b / a, r.nextFloat()); }
    bool chance (float c) { return r.nextFloat() < c; }
    template <typename T> T pick (const std::vector<T>& v) { return v[(size_t) r.nextInt ((int) v.size())]; }

    void fx (int l, int slot, int type, float p1, float p2, float p3, float mix)
    {
        const juce::String base = l < 0 ? "mfx" + juce::String (slot + 1) : "l" + juce::String (l + 1) + "_fx" + juce::String (slot + 1);
        set (base + "_type", (float) type); set (base + "_p1", p1); set (base + "_p2", p2); set (base + "_p3", p3); set (base + "_mix", mix);
    }
    void mod (int src, int dst, float amt, int layer = 0)
    {
        if (modSlot >= numModSlots) return;
        const juce::String n = "mod" + juce::String (++modSlot) + "_";
        set (n + "src", (float) src); set (n + "dst", (float) dst); set (n + "layer", (float) layer); set (n + "amt", amt);
    }
    void env (int l, float a, float d, float s, float rel) { L (l, "attack", a); L (l, "decay", d); L (l, "sustain", s); L (l, "release", rel); }
    void filter (int l, int type, float cutoff, float reso, float fenv, float fa, float fd, float fs, float fr)
    {
        L (l, "filter", (float) type); L (l, "cutoff", cutoff); L (l, "reso", reso); L (l, "fenv", fenv);
        L (l, "fattack", fa); L (l, "fdecay", fd); L (l, "fsustain", fs); L (l, "frelease", fr);
    }
    // tracker macro on a layer: kind 'v' volume, 'a' arpeggio, 'w' wave, 'p' pitch
    void macro (int l, char kind, std::vector<int> values, int loop = -1)
    {
        const juce::String k = juce::String ("m") + juce::String::charToString ((juce::juce_wchar) kind);
        const int n = juce::jmin ((int) values.size(), Macro::maxLen);
        for (int i = 0; i < n; ++i) p.steps.push_back ({ l, strdupField (k), i, (float) values[(size_t) i] });
        p.steps.push_back ({ l, strdupField (k + "len"), 0, (float) n });
        p.steps.push_back ({ l, strdupField (k + "loop"), 0, (float) loop });
    }
    void speed (int l, int framesPerStep) { p.steps.push_back ({ l, "mspeed", 0, (float) framesPerStep }); }
    static const char* strdupField (const juce::String& s)
    {
        static std::set<std::string> pool;   // field names live as long as the program
        return pool.insert (s.toStdString()).first->c_str();
    }

    bool hasChips() const { return ! pf.chips.empty(); }
    bool hasSampler() const { return ! pf.samplers.empty(); }

    // a sound source for a layer: a chip channel or a ROM instrument of this category; returns the chip (or -1)
    int voice (int l, int forCat = -1, float samplerBias = 0.5f)
    {
        const int c = forCat < 0 ? cat : forCat;
        const bool useSampler = hasSampler() && (! hasChips() || chance (samplerBias));
        if (useSampler)
        {
            L (l, "src", (float) pick (pf.samplers));
            L (l, "wave", (float) (numSounds + pick (instrumentsFor (c))));
            L (l, "char", rnd (0.35f, 0.75f));
            return -1;
        }
        const int src = pick (pf.chips);
        L (l, "src", (float) src);
        L (l, "wave", (float) pick (chipWaves (src, c)));
        L (l, "char", rnd (0.3f, 0.7f));
        return src;
    }
    int chipVoice (int l, int forCat)   // always a chip when the platform has one
    {
        if (! hasChips()) return voice (l, forCat, 1.0f);
        const int src = pick (pf.chips);
        L (l, "src", (float) src); L (l, "wave", (float) pick (chipWaves (src, forCat))); L (l, "char", rnd (0.3f, 0.7f));
        return src;
    }
    void instrument (int l, int ins)
    {
        L (l, "src", (float) (hasSampler() ? pick (pf.samplers) : (int) sampler16));
        L (l, "wave", (float) (numSounds + ins));
    }
    // a noise channel (drums, FX)
    void noise (int l, bool metallic = false)
    {
        for (int src : pf.chips)
        {
            const auto w = chipWaves (src, cDrums);
            if (src == console8 || src == handheld4 || src == computerSid || src == psg || src == polyCounter || src == pokey || src == ttl)
            {
                L (l, "src", (float) src); L (l, "wave", (float) (metallic && w.size() > 1 ? w[1] : w[0])); L (l, "char", 0.5f);
                return;
            }
        }
        instrument (l, metallic ? insClosedHat : insSnare);
    }

    // the performance controls every preset shares (MACRO page)
    void standardMacros()
    {
        mod (msMacro1, mdCutoff, -0.6f);        // M1 DARK
        mod (msMacro1, mdTone, -0.5f);
        mod (msMacro1 + 1, mdChar, 0.8f);       // M2 CHARACTER
        mod (msMacro1 + 2, mdSpace, 0.7f);      // M3 SPACE
        mod (msMacro1 + 3, mdVibrato, 0.4f);    // M4 MOTION
        mod (msMacro1 + 4, mdMFx4, -0.85f);     // M5 CRUNCH (master slot 4 = bit crusher)
        mod (msMacro1 + 5, mdPitch, 0.5f);      // M6 PITCH (+12 st at full)
        mod (msMacro1 + 6, mdFxAMix, 1.0f);     // M7 FX A
        mod (msMacro8, mdMFx3, 0.5f);           // M8 WIDTH
        mod (msWheel, mdVibrato, 0.3f);
        fx (-1, 2, fxWidth, 0.5f, 0.0f, 0.5f, 1.0f);
        fx (-1, 3, fxBite, 1.0f, 1.0f, 0.0f, 1.0f);
    }
};

// ---- tracker macro library ----------------------------------------------------------------------------------------
const std::vector<std::vector<int>> volShapes {
    { 15, 13, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },                  // pluck
    { 15, 14, 13, 12, 11, 10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 5 },         // piano (holds 5)
    { 6, 8, 10, 12, 13, 14, 15 },                                      // swell
    { 15, 11, 7, 4, 2, 0 },                                            // staccato
    { 15, 12, 10, 8, 0, 0, 7, 6, 5, 0, 0, 3, 2, 1, 0 },                // fake echo
    { 15, 14, 13, 12, 12, 12 },                                        // lead (holds 12)
};
const std::vector<std::vector<int>> arpShapes { { 0, 4, 7 }, { 0, 3, 7 }, { 0, 12 }, { 0, 7, 12 }, { 0, 4, 7, 11 }, { 0, 5, 7 }, { 0, 3, 7, 10 }, { 0, 7 } };
const std::vector<int> vibrato { 0, 0, 0, 0, 0, 0, 6, 6, -6, -6, -6, -6, 6, 6 };   // delayed vibrato (cents per frame, loops)

void chipVolume (Builder& b, int l, int shape) { b.macro (l, 'v', volShapes[(size_t) shape], shape == 1 || shape == 5 ? -1 : -1); }
void chipArp (Builder& b, int l) { b.macro (l, 'a', b.pick (arpShapes), 0); }
void chipVibrato (Builder& b, int l) { b.macro (l, 'p', vibrato, 6); }

// ---- recipes --------------------------------------------------------------------------------------------------------
void designBass (Builder& b, int v)
{
    const int src = b.voice (0, cBass, 0.6f);
    b.env (0, 0, 900, 1.0f, 60);
    switch (v % 7)
    {
        case 0: chipVolume (b, 0, 5); break;                                                  // steady channel bass
        case 1: chipVolume (b, 0, 0); b.env (0, 0, 400, 0.0f, 40); break;                      // plucked
        case 2: b.macro (0, 'a', { 0, 12 }, 0); b.speed (0, 6); chipVolume (b, 0, 5); break;   // octave wobble
        case 3: b.macro (0, 'p', { -40, -20, -10, 0 }); chipVolume (b, 0, 1); break;          // slap-in
        case 4: b.L (0, "mode", vmLegato); b.L (0, "glide", b.rnd (40, 110)); chipVolume (b, 0, 5); break;   // glide bass
        case 5: b.filter (0, fLp24, b.rndExp (400, 1400), b.rnd (0.3f, 0.65f), b.rnd (0.3f, 0.7f), 1, b.rndExp (80, 300), 0.1f, 80); break;
        default: if (src == computerSid || src == console8 || src == vrc6) b.macro (0, 'w', src == computerSid ? std::vector<int> { 5, 0 } : std::vector<int> { 1, 2 }, 1);
                 chipVolume (b, 0, 5); break;
    }
    if (b.chance (0.3f)) { b.L (1, "on", 1); b.chipVoice (1, cBass); b.L (1, "coarse", -12); b.L (1, "level", 0.45f); b.env (1, 0, 900, 1, 60); }
    b.L (0, "vel", 0.3f);
}

void designLead (Builder& b, int v)
{
    const int src = b.voice (0, cLead, 0.5f);
    b.env (0, 0, 1000, 1.0f, 90);
    chipVolume (b, 0, v % 3 == 0 ? 0 : 5);
    switch (v % 9)
    {
        case 0: chipVibrato (b, 0); break;
        case 1: chipArp (b, 0); break;                                                         // the chip chord
        case 2: chipVibrato (b, 0); b.fx (0, 0, fxDelay, 0.58f, 0.4f, 0.45f, 0.3f); break;    // echo channel
        case 3: b.macro (0, 'p', { -60, -30, -10, 0 }); chipVibrato (b, 0); break;            // bend-in
        case 4: if (src == console8) b.macro (0, 'w', { 0, 1, 2, 1 }, 0);                       // duty cycling
                else if (src == vrc6) b.macro (0, 'w', { 7, 5, 3, 1, 3, 5 }, 0);
                else if (src == computerSid) { b.set ("lfo2_rate", b.rnd (0.5f, 3.0f)); b.mod (msLfo1 + 1, mdChar, 0.5f); }
                else chipVibrato (b, 0);
                break;
        case 5: b.L (0, "mode", vmLegato); b.L (0, "glide", b.rnd (40, 120)); chipVibrato (b, 0); break;
        case 6: b.macro (0, 'a', { 0, 0, 12, 12 }, 0); break;                                  // octave trill
        case 7: { b.L (1, "on", 1); b.chipVoice (1, cLead); b.L (1, "fine", b.rnd (8, 15)); b.L (1, "level", 0.55f); b.env (1, 0, 1000, 1, 90);
                  chipVolume (b, 1, 5); chipVibrato (b, 0); break; }                           // two channels detuned
        default: b.macro (0, 'a', { 0, 2 }, 0); break;                                         // trill
    }
}

void designPad (Builder& b, int v)
{
    for (int l = 0; l < 2; ++l)
    {
        b.L (l, "on", 1); b.voice (l, cPad, 0.6f);
        b.env (l, b.rndExp (200, 1200), 1500, 0.9f, b.rndExp (600, 2000));
        b.L (l, "fine", l == 0 ? -b.rnd (4, 9) : b.rnd (4, 9)); b.L (l, "pan", l == 0 ? -0.3f : 0.3f); b.L (l, "level", 0.55f);
    }
    switch (v % 5)
    {
        case 0: chipArp (b, 0); b.env (0, 5, 1500, 0.9f, 1500); break;   // chord shimmer
        case 1: chipVibrato (b, 0); chipVibrato (b, 1); break;
        case 2: b.L (1, "coarse", 12); break;
        case 3: for (int l = 0; l < 2; ++l) b.filter (l, fLp12, b.rndExp (700, 2500), 0.3f, 0.0f, 1, 300, 1, 300);
                b.set ("lfo1_rate", b.rnd (0.08f, 0.3f)); b.set ("lfo1_retrig", 0); b.mod (msLfo1, mdCutoff, 0.35f); break;
        default: b.fx (0, 0, fxChorus, 0.25f, 0.6f, 0.6f, 0.5f); break;
    }
    if (b.pf.echo) b.fx (-1, 0, fxConsoleEcho, b.rnd (0.3f, 0.6f), 0.45f, b.rnd (0.2f, 0.6f), 0.35f);
    else b.fx (-1, 0, fxReverb, b.rnd (0.5f, 0.8f), 0.4f, 1.0f, b.rnd (0.2f, 0.35f));
}

void designKeys (Builder& b, int v)
{
    b.voice (0, cKeys, 0.7f);
    b.env (0, 0, b.rndExp (900, 2500), 0.0f, 250);
    chipVolume (b, 0, 1);
    if (v % 3 == 1) chipArp (b, 0);
    if (v % 4 == 2) { b.L (1, "on", 1); b.chipVoice (1, cBells); b.L (1, "coarse", 12); b.L (1, "level", 0.3f); b.env (1, 0, 300, 0, 150); chipVolume (b, 1, 3); }
    b.L (0, "vel", 0.7f);
}

void designPluck (Builder& b, int v)
{
    b.voice (0, cPluck, 0.6f);
    b.env (0, 0, b.rndExp (200, 700), 0.0f, 150);
    chipVolume (b, 0, v % 2 ? 0 : 3);
    switch (v % 4)
    {
        case 0: b.fx (0, 0, b.pf.echo ? fxConsoleEcho : fxDelay, 0.5f, 0.4f, 0.4f, 0.3f); break;
        case 1: b.filter (0, fLp24, b.rndExp (400, 1500), 0.5f, 0.6f, 1, b.rndExp (60, 200), 0, 100); break;
        case 2: b.macro (0, 'p', { 30, 15, 5, 0 }); break;   // a little koto bend
        default: break;
    }
}

void designBells (Builder& b, int v)
{
    b.voice (0, cBells, 0.6f);
    b.env (0, 0, b.rndExp (900, 2500), 0.0f, 800);
    chipVolume (b, 0, 1);
    if (v % 3 == 1) b.macro (0, 'a', { 0, 12 }, 0);
    if (v % 4 == 3) b.macro (0, 'a', { 0, 7, 12, 19 }, 3);
    b.fx (-1, 0, b.pf.echo ? fxConsoleEcho : fxReverb, b.rnd (0.3f, 0.6f), 0.4f, b.pf.echo ? 0.4f : 1.0f, 0.3f);
}

void designBrass (Builder& b, int v)
{
    for (int l = 0; l < (v % 2 ? 2 : 1); ++l)
    {
        b.L (l, "on", 1); b.voice (l, cBrass, 0.6f);
        b.env (l, b.rnd (10, 50), 800, 0.85f, 150);
        b.macro (l, 'v', { 9, 12, 14, 15, 14, 13 }, -1);
        b.filter (l, fLp24, b.rndExp (900, 2500), 0.2f, 0.4f, b.rnd (20, 60), 400, 0.5f, 150);
        b.L (l, "fine", l == 0 ? 0.0f : b.rnd (6, 12)); b.L (l, "level", 0.6f);
    }
    if (v % 3 == 0) b.macro (0, 'p', { -30, -15, -5, 0 });
    chipVibrato (b, 0);
}

void designStrings (Builder& b, int v)
{
    for (int l = 0; l < 2; ++l)
    {
        b.L (l, "on", 1); b.voice (l, cStrings, 0.7f);
        b.env (l, b.rndExp (150, 600), 900, 0.9f, b.rndExp (300, 900));
        b.L (l, "fine", l == 0 ? -b.rnd (4, 9) : b.rnd (4, 9)); b.L (l, "pan", l == 0 ? -0.35f : 0.35f); b.L (l, "level", 0.55f);
        chipVibrato (b, l);
    }
    if (v % 3 == 0) b.L (1, "coarse", -12);
    b.fx (0, 0, fxChorus, 0.2f, 0.5f, 0.6f, 0.4f);
}

void designOrgan (Builder& b, int v)
{
    b.voice (0, cOrgan, 0.4f);
    b.env (0, 2, 100, 1.0f, 60);
    b.L (1, "on", 1); b.voice (1, cOrgan, 0.4f); b.L (1, "coarse", 12); b.L (1, "level", 0.4f); b.env (1, 2, 100, 1.0f, 60);
    if (v % 2) b.fx (0, 0, fxTremolo, 0.55f, 0.35f, 0.8f, 1.0f);
    else b.fx (0, 0, fxChorus, 0.5f, 0.5f, 0.7f, 0.5f);
}

void designWinds (Builder& b, int v)
{
    b.voice (0, cWinds, 0.6f);
    b.env (0, b.rnd (20, 80), 800, 0.9f, 150);
    b.macro (0, 'v', { 8, 11, 13, 14, 15, 14 }, -1);
    chipVibrato (b, 0);
    if (v % 3 == 0) { b.L (0, "mode", vmLegato); b.L (0, "glide", b.rnd (20, 70)); }
    b.filter (0, fLp12, b.rndExp (2500, 6000), 0.1f, 0.1f, 40, 300, 0.7f, 200);
}

void designWaves (Builder& b, int v)
{
    // the raw channels, the way a tracker hands them to you: steady, in tune, ready to sequence.
    // Every (chip, wave, shape, octave) combination once, in order, so no two are alike.
    std::vector<std::pair<int, int>> combos;
    for (int src : b.pf.chips) for (int w : chipWaves (src, cWaves)) combos.push_back ({ src, w });
    if (combos.empty()) { b.voice (0, cWaves, 1.0f); combos.push_back ({ -1, -1 }); }
    const int n = (int) combos.size();
    const auto c = combos[(size_t) (v % n)];
    if (c.first >= 0) { b.L (0, "src", (float) c.first); b.L (0, "wave", (float) c.second); b.L (0, "char", 0.5f); }
    b.env (0, 0, 1000, 1.0f, 30.0f + (float) (v % 7) * 12.0f);
    switch ((v / n) % 4)
    {
        case 0: break;
        case 1: chipVolume (b, 0, 5); break;
        case 2: chipVolume (b, 0, 0); break;
        default: chipVibrato (b, 0); break;
    }
    b.L (0, "coarse", (float) (((v / (n * 4)) % 3) - 1) * 12.0f);
}

void designDrum (Builder& b, int kind, int layerBase = 0)
{
    const int l = layerBase;
    const bool chips = b.hasChips();
    switch (kind)
    {
        case 0:   // kick: a tone with a fast pitch macro drop (+ click)
            if (chips) { const int keep = b.cat; b.cat = cBass; b.chipVoice (l, cBass); b.cat = keep;
                         b.macro (l, 'a', { 24, 15, 9, 5, 2, 0, -2 }); b.macro (l, 'v', { 15, 14, 12, 10, 8, 6, 4, 2, 0 }); b.L (l, "coarse", -12); }
            else b.instrument (l, insKick);
            b.env (l, 0, b.rnd (250, 550), 0.0f, 60);
            break;
        case 1:   // snare: noise burst + tone
            b.noise (l); b.env (l, 0, b.rnd (180, 320), 0.0f, 60); b.L (l, "coarse", (float) b.pick (std::vector<int> { 0, 7, 12, 19 }));
            if (chips) b.macro (l, 'v', { 15, 12, 10, 8, 6, 4, 3, 2, 1, 0 });
            if (chips) { b.L (l + 1, "on", 1); b.chipVoice (l + 1, cBass); b.macro (l + 1, 'a', { 12, 5, 0 }); b.macro (l + 1, 'v', { 12, 6, 0 }); b.env (l + 1, 0, 150, 0, 40); b.L (l + 1, "level", 0.6f); }
            break;
        case 2: case 3:   // hats
            b.noise (l, true); b.env (l, 0, kind == 2 ? b.rnd (40, 110) : b.rnd (220, 480), 0.0f, 40);
            if (chips) b.macro (l, 'v', kind == 2 ? std::vector<int> { 12, 6, 2, 0 } : std::vector<int> { 12, 10, 8, 7, 6, 5, 4, 3, 2, 1, 0 });
            b.L (l, "coarse", (float) b.pick (std::vector<int> { 12, 19, 24, 31 }));
            b.filter (l, fHp12, b.rndExp (4500, 10000), b.rnd (0.1f, 0.5f), 0, 1, 100, 0, 50);
            break;
        case 4:   // tom
            if (chips) { b.chipVoice (l, cBass); b.macro (l, 'a', { 12, 7, 4, 2, 0 }); b.macro (l, 'v', { 15, 13, 11, 9, 7, 5, 3, 1, 0 }); }
            else b.instrument (l, insTom);
            b.env (l, 0, 400, 0, 80);
            break;
        case 5:   // clap
            if (chips) { b.noise (l); b.macro (l, 'v', { 15, 0, 13, 0, 11, 9, 6, 3, 1, 0 }); }
            else b.instrument (l, insClap);
            b.env (l, 0, 300, 0, 80);
            break;
        case 6:   // rim
            if (chips) { b.chipVoice (l, cWaves); b.L (l, "coarse", 24); b.macro (l, 'v', { 15, 6, 0 }); }
            else b.instrument (l, insRim);
            b.env (l, 0, 80, 0, 20);
            break;
        default:  // crash
            if (chips) { b.noise (l, true); b.macro (l, 'v', { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 }); b.speed (l, 6); }
            else b.instrument (l, insCrash);
            b.env (l, 0, 1800, 0, 600);
            break;
    }
}

void designPerc (Builder& b, int v)
{
    b.L (0, "coarse", (float) b.pick (std::vector<int> { -5, 0, 0, 5, 7, 12 }));
    b.L (0, "fine", b.rnd (-15, 15));
    switch (v % 5)
    {
        case 0: b.chipVoice (0, cPerc); b.L (1, "on", 1); b.chipVoice (1, cPerc); b.L (1, "coarse", 7); b.L (1, "fine", -20);   // cowbell
                for (int l = 0; l < 2; ++l) { b.env (l, 0, 300, 0, 80); b.macro (l, 'v', { 15, 11, 8, 6, 4, 2, 1, 0 }); } break;
        case 1: b.voice (0, cPerc, 0.6f); b.env (0, 0, b.rnd (60, 200), 0, 40); b.macro (0, 'v', { 15, 8, 3, 0 }); break;   // block
        case 2: b.chipVoice (0, cWaves); b.L (0, "coarse", 12); b.macro (0, 'a', { 12, 0 }); b.macro (0, 'v', { 15, 10, 4, 0 }); b.env (0, 0, 100, 0, 30); break;   // blip
        case 3: b.noise (0, true); b.macro (0, 'v', { 4, 9, 12, 8, 4, 2, 0 }); b.env (0, 0, 200, 0, 50); b.filter (0, fHp12, 7000, 0.2f, 0, 1, 100, 0, 50); break;   // shaker
        default: b.voice (0, cPerc, 0.7f); b.macro (0, 'a', { 7, 3, 0 }); b.env (0, 0, b.rnd (200, 450), 0, 80); break;   // conga
    }
}

void designFx (Builder& b, int v)
{
    auto tone = [&] (int l) { b.chipVoice (l, cWaves); };
    switch (v % 20)
    {
        case 0: tone (0); b.L (0, "coarse", 12); b.env (0, 0, 600, 1, 200); b.macro (0, 'a', { 0, 0, 0, 0, 0, 5 }); b.macro (0, 'v', { 15, 15, 15, 15, 15, 15, 14, 12, 10, 8, 6, 4, 2, 0 }); break;   // coin
        case 1: tone (0); b.L (0, "coarse", 12); b.env (0, 0, 900, 1, 100); b.macro (0, 'a', { 0, 0, 0, 4, 4, 4, 7, 7, 7, 12, 12, 12, 16, 16, 19, 19 }); break;   // 1-up
        case 2: tone (0); b.env (0, 0, 300, 1, 40); b.macro (0, 'p', { -60, -40, -30, -20, -10, -5 }); b.macro (0, 'v', { 15, 14, 12, 10, 8, 6, 4, 2, 0 }); break;   // jump
        case 3: tone (0); b.env (0, 0, 900, 1, 60); b.macro (0, 'a', { 0, 7, 12, 4, 11, 16, 7, 14, 19, 12, 19, 24 }); b.macro (0, 'v', { 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 13, 10, 7, 4, 0 }); break;   // power-up
        case 4: tone (0); b.env (0, 0, 300, 1, 50); b.macro (0, 'p', { 80, 60, 40, -30, -60, -90 }); b.macro (0, 'v', { 15, 12, 9, 6, 3, 0 });   // hurt
                b.L (1, "on", 1); b.noise (1); b.env (1, 0, 150, 0, 40); b.L (1, "level", 0.4f); break;
        case 5: tone (0); b.L (0, "coarse", 12); b.env (0, 0, 400, 1, 60); b.macro (0, 'p', { -150, -120, -100, -80, -60, -40 }, 5); b.macro (0, 'v', { 15, 14, 12, 10, 8, 6, 4, 2, 0 }); break;   // laser
        case 6: b.noise (0); b.env (0, 0, 1500, 1, 500); b.speed (0, 4); b.macro (0, 'v', { 15, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 0 });   // explosion
                b.macro (0, 'p', { -20, -20, -20, -20 }, 0); b.L (0, "coarse", -12); break;
        case 7: tone (0); b.L (0, "coarse", 24); b.env (0, 0, 120, 1, 30); b.macro (0, 'a', { 0, 0, 12, 12 }); b.macro (0, 'v', { 15, 15, 15, 15, 0 }); break;   // menu select
        case 8: tone (0); b.L (0, "coarse", 12); b.env (0, 0, 120, 1, 30); b.macro (0, 'a', { 7, 7, 0, 0 }); b.macro (0, 'v', { 15, 15, 15, 15, 0 }); break;   // menu back
        case 9: tone (0); b.L (0, "coarse", 12); b.env (0, 0, 400, 1, 60); b.speed (0, 4); b.macro (0, 'a', { 0, 12, 0, 12 }); b.macro (0, 'v', { 15, 15, 15, 0 }); break;   // pause
        case 10: tone (0); b.env (0, 0, 1200, 1, 300); b.macro (0, 'p', { 40, 40, 40, 40, -160 }, 0); break;   // teleport
        case 11: tone (0); b.env (0, 0, 1500, 1, 200); b.speed (0, 8); b.macro (0, 'a', { 12, 7, 4, 0, -5, -12 }); b.macro (0, 'v', { 15, 15, 15, 15, 15, 15, 12, 8, 4, 0 }); break;   // game over
        case 12: tone (0); b.env (0, 0, 1200, 1, 200); b.speed (0, 5); b.macro (0, 'a', { 0, 4, 7, 12, 7, 12, 16, 19, 24 }); b.macro (0, 'v', { 15, 15, 15, 15, 15, 15, 15, 15, 15, 12, 9, 6, 3, 0 }); break;   // stage clear
        case 13: b.noise (0); b.env (0, 0, 500, 1, 100); b.macro (0, 'p', { 60, 40, 20, 0, -20, -40 }); b.macro (0, 'v', { 15, 13, 11, 9, 7, 5, 3, 1, 0 }); break;   // fireball
        case 14: b.noise (0); b.env (0, 0, 700, 1, 200); b.macro (0, 'p', { -30, 30 }, 0); b.macro (0, 'v', { 6, 10, 14, 12, 10, 8, 6, 4, 2, 0 }); b.filter (0, fBp12, 1500, 0.5f, 0, 1, 300, 0, 200); break;   // splash
        case 15: tone (0); b.env (0, 0, 800, 1, 100); b.speed (0, 8); b.macro (0, 'a', { 0, 5 }, 0); break;   // alarm
        case 16: b.chipVoice (0, cBass); b.L (0, "coarse", -12); b.env (0, 60, 1000, 1, 300); b.macro (0, 'p', { 8, -8 }, 0); b.macro (0, 'v', { 12, 10 }, 0); break;   // hover engine
        case 17: tone (0); b.L (0, "coarse", 12); b.env (0, 0, 60, 0, 20); b.mod (msRandom, mdPitch, 0.25f); break;   // text blip
        case 18: for (int l = 0; l < 2; ++l) { b.L (l, "on", 1); b.chipVoice (l, cBass); b.L (l, "coarse", -12); b.L (l, "fine", l ? 30.0f : -30.0f);   // boss roar
                     b.env (l, 30, 1200, 0, 400); b.macro (l, 'p', { -10 }, 0); }
                 b.fx (0, 0, fxDrive, 0.6f, 0.5f, 0.4f, 0.8f); break;
        default: tone (0); b.env (0, 200, 2500, 1, 200); b.macro (0, 'p', { 10 }, 0); b.macro (0, 'v', { 8, 15 }, 0); break;   // charge up
    }
}

void designArp (Builder& b, int v)
{
    b.voice (0, cArp, 0.6f);
    b.env (0, 0, b.rndExp (120, 400), 0.0f, 100);
    chipVolume (b, 0, 0);
    static const int rates[] = { 12, 7, 11, 8, 7, 6 };
    static const int dirs[] = { adUp, adUp, adUpDown, adRandom, adConverge, adDown };
    b.L (0, "arp", amArp); b.L (0, "arpdir", (float) dirs[v % 6]); b.L (0, "arprate", (float) rates[v % 6]);
    b.L (0, "arpoct", (float) (v % 6 == 0 ? 1 : 2)); b.L (0, "arpgate", v % 6 == 0 ? 1.0f : b.rnd (0.4f, 0.8f));
    b.L (0, "arpsteps", 8);
    for (int s = 0; s < 8; ++s) if (s % 4 != 0) b.p.steps.push_back ({ 0, "vel", s, b.rnd (0.55f, 0.85f) });
    b.fx (0, 0, b.pf.echo ? fxConsoleEcho : fxDelay, b.rnd (0.4f, 0.65f), b.rnd (0.25f, 0.45f), 0.4f, 0.3f);
}

void designSeq (Builder& b, int v)
{
    // a little loop the platform could play: bass line + hats + kick/snare, all on the held key
    static const int scales[4][5] = { { 0, 3, 5, 7, 10 }, { 0, 2, 3, 7, 8 }, { 0, 2, 4, 7, 9 }, { 0, 3, 5, 6, 7 } };
    const int* sc = scales[v % 4];
    b.voice (0, cBass, 0.5f); b.env (0, 0, 250, 0.0f, 50); chipVolume (b, 0, 0);
    b.L (0, "arp", amSeq); b.L (0, "arprate", 7); b.L (0, "arpsteps", 16); b.L (0, "arpgate", b.rnd (0.5f, 0.8f));
    for (int s = 0; s < 16; ++s)
    {
        if (! (s % 4 == 0 || b.chance (0.4f))) { b.p.steps.push_back ({ 0, "on", s, 0.0f }); continue; }
        b.p.steps.push_back ({ 0, "note", s, (float) (sc[b.r.nextInt (5)] - (b.chance (0.25f) ? 12 : 0)) });
    }
    b.L (1, "on", 1); designDrum (b, 2, 1);
    b.L (1, "arp", amSeq); b.L (1, "arprate", (float) (b.e == 11 && b.chance (0.5f) ? 8 : 7)); b.L (1, "arpsteps", 16); b.L (1, "arpgate", 0.4f); b.L (1, "level", 0.4f);
    for (int s = 0; s < 16; ++s) b.p.steps.push_back ({ 1, "vel", s, s % 2 == 0 ? 1.0f : 0.5f });
    b.L (2, "on", 1); designDrum (b, 0, 2);
    b.L (2, "arp", amSeq); b.L (2, "arprate", 7); b.L (2, "arpsteps", 16); b.L (2, "arpgate", 0.6f);
    for (int s = 0; s < 16; ++s)
    {
        const bool kick = s == 0 || s == 10 || (s == 7 && b.chance (0.5f));
        const bool snare = s == 4 || s == 12;
        if (! kick && ! snare) b.p.steps.push_back ({ 2, "on", s, 0.0f });
        else if (snare) b.p.steps.push_back ({ 2, "note", s, 12.0f });
    }
}
} // namespace

const char* categoryName (int c)
{
    static const char* n[] = { "BASS", "LEAD", "PAD", "KEYS", "PLUCK", "BELLS", "BRASS", "STRINGS", "ORGAN", "WINDS", "WAVES", "DRUMS", "PERC", "FX", "ARP", "SEQUENCE" };
    return n[juce::jlimit (0, numCategories - 1, c)];
}

const char* presetTypeName (int t)
{
    static const char* n[] = { "ONE-SHOT", "SUSTAIN", "PHRASE", "LOOP" };
    return n[juce::jlimit (0, numPresetTypes - 1, t)];
}

const char* platformName (int eraIndex) { return platform (eraIndex).name; }

namespace
{
// position in the era -> (category, variant)
std::pair<int, int> slotOf (int j)
{
    if (j < 6) return { showcase[j], 100 + j };
    int k = j - 6;
    for (int c = 0; c < numCategories; ++c)
    {
        const int n = counts[c] - (std::count (std::begin (showcase), std::end (showcase), c) > 0 ? 1 : 0);
        if (k < n) return { c, k };
        k -= n;
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
        const auto [cat, v] = slotOf (j);
        info.category = cat;
        const bool sustained = cat == cBass || cat == cLead || cat == cPad || cat == cBrass || cat == cStrings || cat == cOrgan || cat == cWinds || cat == cWaves;
        info.type = cat == cArp ? ptPhrase : (cat == cSeq ? ptLoop : (sustained ? ptSustain : ptShot));
        info.root = rootOf[cat];
        if (cat == cDrums && v % 8 == 0) info.root = 36;
        info.previewSeconds = cat == cArp || cat == cSeq ? 4.0f : (cat == cPad || cat == cStrings ? 3.0f : cat == cFx ? 2.0f : 1.4f);
        const juce::String noun = cat == cDrums ? juce::String (nouns[cDrums][v % 8]) : cat == cPerc ? juce::String (nouns[cPerc][v % 12])
                                : cat == cFx ? juce::String (fxNouns[v % 20]) : juce::String (nouns[cat][(v * 5 + v / 25 + e * 7) % 12]);
        const int a = (v * 7 + cat * 3) % 25;
        juce::String name;
        for (int tries = 0; tries < 25; ++tries)
        {
            name = juce::String (adjectives[e][(a + tries) % 25]) + " " + noun;
            if (used.count (name) == 0) break;
        }
        for (int k = 2; used.count (name) > 0; ++k) name = juce::String (adjectives[e][a]) + " " + noun + " " + juce::String (k);
        info.name = name;
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
    Builder b { p, juce::Random ((juce::int64) (index * 7919 + 104729)), e, 0, platform (e) };
    b.standardMacros();
    p.params.push_back ({ "l1_on", 1.0f });
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
        case cBrass: designBrass (b, v); break;
        case cStrings: designStrings (b, v); break;
        case cOrgan: designOrgan (b, v); break;
        case cWinds: designWinds (b, v); break;
        case cWaves: designWaves (b, v); break;
        case cDrums: designDrum (b, v % 8); break;
        case cPerc: designPerc (b, v); break;
        case cFx: designFx (b, v); break;
        case cArp: designArp (b, v); break;
        default: designSeq (b, v); break;
    }
    // the platform's own colour
    if (b.pf.echo && cat != cSeq && cat != cDrums) b.fx (-1, 1, fxConsoleEcho, b.rnd (0.25f, 0.5f), b.rnd (0.3f, 0.5f), b.rnd (0.2f, 0.6f), b.rnd (0.15f, 0.3f));
    if (e == 11 && b.chance (0.4f)) b.fx (-1, 1, b.pick (b.pf.fx), b.rnd (0.3f, 0.6f), b.rnd (0.3f, 0.6f), b.rnd (0.3f, 0.6f), b.rnd (0.2f, 0.4f));
    b.set ("space", b.rnd (0.04f, cat == cPad || cat == cStrings || cat == cBells ? 0.25f : 0.12f));
    return p;
}
} // namespace bitera
