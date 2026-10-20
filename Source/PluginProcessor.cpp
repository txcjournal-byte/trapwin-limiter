#include "PluginProcessor.h"
#include "BitEraData.h"
#if ! BE_HEADLESS
 #include "PluginEditor.h"
#endif

namespace
{
constexpr int defaultPreset = (bitera::numEras - 1) * bitera::soundsPerEra;   // NOW era, first one-shot

const char* findResource (const char* originalName, int& size)
{
    for (int i = 0; i < BitEraData::namedResourceListSize; ++i)
    {
        const char* resName = BitEraData::namedResourceList[i];
        if (juce::String (BitEraData::getNamedResourceOriginalFilename (resName)) == originalName)
            return BitEraData::getNamedResource (resName, size);
    }
    size = 0;
    return nullptr;
}

juce::String msText (float v, int) { return v < 1000.0f ? juce::String (juce::roundToInt (v)) + " ms" : juce::String (v / 1000.0f, 2) + " s"; }
juce::String pctText (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
} // namespace

BitEraProcessor::BitEraProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "BitEra", createLayout())
{
    auto raw = [this] (const juce::String& id) { auto* p = state.getRawParameterValue (id); jassert (p != nullptr); return p; };
    for (int i = 0; i < bitera::numLayers; ++i)
    {
        auto& l = lp[i];
        auto g = [&] (const char* n) { return raw (layerId (i, n)); };
        l.on = g ("on"); l.source = g ("src"); l.wave = g ("wave"); l.level = g ("level"); l.pan = g ("pan");
        l.coarse = g ("coarse"); l.fine = g ("fine"); l.character = g ("char"); l.velSens = g ("vel");
        l.keyLo = g ("keylo"); l.keyHi = g ("keyhi");
        l.a = g ("attack"); l.d = g ("decay"); l.s = g ("sustain"); l.r = g ("release");
        l.filter = g ("filter"); l.cutoff = g ("cutoff"); l.reso = g ("reso"); l.fltEnv = g ("fenv"); l.keyTrack = g ("ktrack");
        l.fa = g ("fattack"); l.fd = g ("fdecay"); l.fs = g ("fsustain"); l.fr = g ("frelease");
        l.pitchEnv = g ("penv"); l.pitchDecay = g ("pdecay"); l.vibDepth = g ("vibdepth"); l.vibRate = g ("vibrate");
        l.mode = g ("mode"); l.glide = g ("glide");
        l.arp = g ("arp"); l.arpDir = g ("arpdir"); l.arpRate = g ("arprate"); l.arpOct = g ("arpoct"); l.arpSteps = g ("arpsteps");
        l.arpGate = g ("arpgate"); l.arpSwing = g ("arpswing"); l.arpEnd = g ("arpend");
        for (int k = 0; k < bitera::layerFxSlots; ++k)
            l.fx[k] = { raw (fxId (i, k, "type")), raw (fxId (i, k, "p1")), raw (fxId (i, k, "p2")), raw (fxId (i, k, "p3")), raw (fxId (i, k, "mix")) };
    }
    for (int k = 0; k < bitera::masterFxSlots; ++k)
        mfx[k] = { raw (fxId (-1, k, "type")), raw (fxId (-1, k, "p1")), raw (fxId (-1, k, "p2")), raw (fxId (-1, k, "p3")), raw (fxId (-1, k, "mix")) };
    for (int k = 0; k < bitera::numLfos; ++k)
    {
        const juce::String n = "lfo" + juce::String (k + 1) + "_";
        lfo[k] = { raw (n + "shape"), raw (n + "rate"), raw (n + "sync"), raw (n + "retrig"), raw (n + "fade"), raw (n + "uni") };
    }
    for (int k = 0; k < bitera::numModEnvs; ++k)
    {
        const juce::String n = "menv" + juce::String (k + 1) + "_";
        menv[k] = { raw (n + "a"), raw (n + "d"), raw (n + "s"), raw (n + "r") };
    }
    for (int k = 0; k < bitera::numModSlots; ++k)
    {
        const juce::String n = "mod" + juce::String (k + 1) + "_";
        slot[k] = { raw (n + "src"), raw (n + "dst"), raw (n + "layer"), raw (n + "amt") };
    }
    for (int k = 0; k < bitera::numMacros; ++k) macro[k] = raw ("macro" + juce::String (k + 1));
    xy[0] = raw ("xy1x"); xy[1] = raw ("xy1y"); xy[2] = raw ("xy2x"); xy[3] = raw ("xy2y");
    pBend = raw ("bend");
    pVolume = raw ("volume"); pTone = raw ("tone"); pSpace = raw ("space");

    loadBank();
    engine.setBank (&bank);
    state.state.setProperty ("preset", -1, nullptr);
    setPreset (defaultPreset);
}

juce::AudioProcessorValueTreeState::ParameterLayout BitEraProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    StringArray sources, filters;
    for (int s = 0; s < bitera::chips::numSources; ++s) sources.add (bitera::chips::sourceName (s));
    for (int f = 0; f < bitera::numFilterTypes; ++f) filters.add (bitera::filterName (f));

    auto timeRange = [] (float lo, float hi, float centre) { NormalisableRange<float> r (lo, hi, 0.1f); r.setSkewForCentre (centre); return r; };
    auto fAttr = [] (std::function<String (float, int)> f) { return AudioParameterFloatAttributes().withStringFromValueFunction (std::move (f)); };

    for (int i = 0; i < bitera::numLayers; ++i)
    {
        auto id = [i] (const char* n) { return ParameterID { layerId (i, n), 1 }; };
        const String L = "L" + String (i + 1) + " ";
        auto group = std::make_unique<AudioProcessorParameterGroup> ("layer" + String (i + 1), "Layer " + String (i + 1), "|");
        group->addChild (std::make_unique<AudioParameterChoice> (id ("on"), L + "On", StringArray { "OFF", "ON" }, i == 0 ? 1 : 0));
        group->addChild (std::make_unique<AudioParameterChoice> (id ("src"), L + "Source", sources, 0));
        group->addChild (std::make_unique<AudioParameterInt> (id ("wave"), L + "Wave", 0, 127, 0));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("level"), L + "Level", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f, fAttr (pctText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("pan"), L + "Pan", NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f,
            fAttr ([] (float v, int) { return std::abs (v) < 0.01f ? String ("C") : (v < 0 ? "L" : "R") + String (roundToInt (std::abs (v) * 100.0f)); })));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("coarse"), L + "Coarse", NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 0.0f,
            fAttr ([] (float v, int) { return (v > 0 ? "+" : "") + String (roundToInt (v)) + " st"; })));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("fine"), L + "Fine", NormalisableRange<float> (-100.0f, 100.0f, 1.0f), 0.0f,
            fAttr ([] (float v, int) { return (v > 0 ? "+" : "") + String (roundToInt (v)) + " ct"; })));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("char"), L + "Character", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, fAttr (pctText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("vel"), L + "Velocity", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, fAttr (pctText)));
        auto noteName = [] (float v, int) { return MidiMessage::getMidiNoteName (roundToInt (v), true, true, 3); };
        group->addChild (std::make_unique<AudioParameterFloat> (id ("keylo"), L + "Key Low", NormalisableRange<float> (0.0f, 127.0f, 1.0f), 0.0f, fAttr (noteName)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("keyhi"), L + "Key High", NormalisableRange<float> (0.0f, 127.0f, 1.0f), 127.0f, fAttr (noteName)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("attack"), L + "Attack", timeRange (0.0f, 5000.0f, 150.0f), 2.0f, fAttr (msText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("decay"), L + "Decay", timeRange (5.0f, 8000.0f, 500.0f), 600.0f, fAttr (msText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("sustain"), L + "Sustain", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, fAttr (pctText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("release"), L + "Release", timeRange (5.0f, 8000.0f, 500.0f), 300.0f, fAttr (msText)));
        group->addChild (std::make_unique<AudioParameterChoice> (id ("filter"), L + "Filter", filters, 0));
        NormalisableRange<float> cut (20.0f, 20000.0f, 1.0f); cut.setSkewForCentre (1000.0f);
        group->addChild (std::make_unique<AudioParameterFloat> (id ("cutoff"), L + "Cutoff", cut, 20000.0f,
            fAttr ([] (float v, int) { return v < 1000.0f ? String (roundToInt (v)) + " Hz" : String (v / 1000.0f, 1) + " kHz"; })));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("reso"), L + "Resonance", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.1f, fAttr (pctText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("fenv"), L + "Filter Env", NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f, fAttr (pctText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("ktrack"), L + "Key Track", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f, fAttr (pctText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("fattack"), L + "Filter Attack", timeRange (0.0f, 5000.0f, 150.0f), 2.0f, fAttr (msText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("fdecay"), L + "Filter Decay", timeRange (5.0f, 8000.0f, 500.0f), 400.0f, fAttr (msText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("fsustain"), L + "Filter Sustain", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f, fAttr (pctText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("frelease"), L + "Filter Release", timeRange (5.0f, 8000.0f, 500.0f), 300.0f, fAttr (msText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("penv"), L + "Pitch Env", NormalisableRange<float> (-48.0f, 48.0f, 0.1f), 0.0f,
            fAttr ([] (float v, int) { return (v > 0 ? "+" : "") + String (v, 1) + " st"; })));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("pdecay"), L + "Pitch Decay", timeRange (1.0f, 4000.0f, 200.0f), 100.0f, fAttr (msText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("vibdepth"), L + "Vibrato", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
            fAttr ([] (float v, int) { return String (roundToInt (v)) + " ct"; })));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("vibrate"), L + "Vibrato Rate", NormalisableRange<float> (0.1f, 12.0f, 0.01f), 5.5f,
            fAttr ([] (float v, int) { return String (v, 1) + " Hz"; })));
        group->addChild (std::make_unique<AudioParameterChoice> (id ("mode"), L + "Voice Mode", StringArray { "POLY", "MONO", "LEGATO" }, 0));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("glide"), L + "Glide", timeRange (0.0f, 2000.0f, 200.0f), 0.0f, fAttr (msText)));

        // arpeggiator / sequencer
        StringArray dirs, rates;
        for (int d = 0; d < bitera::numArpDirs; ++d) dirs.add (bitera::arpDirName (d));
        group->addChild (std::make_unique<AudioParameterChoice> (id ("arp"), L + "Arp Mode", StringArray { "OFF", "ARP", "SEQ" }, 0));
        group->addChild (std::make_unique<AudioParameterChoice> (id ("arpdir"), L + "Arp Direction", dirs, 0));
        group->addChild (std::make_unique<AudioParameterChoice> (id ("arprate"), L + "Arp Rate", bitera::syncNames(), 7));
        group->addChild (std::make_unique<AudioParameterInt> (id ("arpoct"), L + "Arp Octaves", 1, 4, 1));
        group->addChild (std::make_unique<AudioParameterInt> (id ("arpsteps"), L + "Arp Steps", 1, bitera::maxSteps, 8));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("arpgate"), L + "Arp Gate", NormalisableRange<float> (0.05f, 1.0f, 0.001f), 0.6f, fAttr (pctText)));
        group->addChild (std::make_unique<AudioParameterFloat> (id ("arpswing"), L + "Arp Swing", NormalisableRange<float> (0.0f, 0.9f, 0.001f), 0.0f, fAttr (pctText)));
        group->addChild (std::make_unique<AudioParameterChoice> (id ("arpend"), L + "Arp End", StringArray { "LOOP", "ONE-SHOT", "HOLD" }, 0));

        StringArray fxTypes;
        for (int t = 0; t < bitera::numFxTypes; ++t) fxTypes.add (bitera::fxName (t));
        for (int k = 0; k < bitera::layerFxSlots; ++k)
        {
            const String F = L + "FX " + String::charToString ((juce_wchar) ('A' + k)) + " ";
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { fxId (i, k, "type"), 1 }, F + "Type", fxTypes, 0));
            for (int q = 0; q < 3; ++q)
                group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { fxId (i, k, ("p" + std::to_string (q + 1)).c_str()), 1 }, F + "Control " + String (q + 1),
                                                                         NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, fAttr (pctText)));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { fxId (i, k, "mix"), 1 }, F + "Mix", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, fAttr (pctText)));
        }
        layout.add (std::move (group));
    }

    // modulation
    {
        auto group = std::make_unique<AudioProcessorParameterGroup> ("mod", "Modulation", "|");
        StringArray shapes, srcs, dsts;
        for (int sh = 0; sh < bitera::numLfoShapes; ++sh) shapes.add (bitera::lfoShapeName (sh));
        for (int m = 0; m < bitera::numModSources; ++m) srcs.add (bitera::modSourceName (m));
        for (int d = 0; d < bitera::numModDests; ++d) dsts.add (bitera::modDestName (d));
        for (int k = 0; k < bitera::numLfos; ++k)
        {
            const String n = "lfo" + String (k + 1) + "_", N = "LFO " + String (k + 1) + " ";
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { n + "shape", 1 }, N + "Shape", shapes, 0));
            NormalisableRange<float> rr (0.01f, 30.0f, 0.001f); rr.setSkewForCentre (2.0f);
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { n + "rate", 1 }, N + "Rate", rr, 2.0f,
                fAttr ([] (float v, int) { return String (v, v < 1.0f ? 2 : 1) + " Hz"; })));
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { n + "sync", 1 }, N + "Sync", bitera::syncNames(), 0));
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { n + "retrig", 1 }, N + "Retrigger", StringArray { "FREE", "RETRIG" }, 1));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { n + "fade", 1 }, N + "Fade In", timeRange (0.0f, 5000.0f, 500.0f), 0.0f, fAttr (msText)));
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { n + "uni", 1 }, N + "Polarity", StringArray { "BI", "UNI" }, 0));
        }
        for (int k = 0; k < bitera::numModEnvs; ++k)
        {
            const String n = "menv" + String (k + 1) + "_", N = "Mod Env " + String (k + 1) + " ";
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { n + "a", 1 }, N + "Attack", timeRange (0.0f, 5000.0f, 150.0f), 10.0f, fAttr (msText)));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { n + "d", 1 }, N + "Decay", timeRange (5.0f, 8000.0f, 500.0f), 500.0f, fAttr (msText)));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { n + "s", 1 }, N + "Sustain", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f, fAttr (pctText)));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { n + "r", 1 }, N + "Release", timeRange (5.0f, 8000.0f, 500.0f), 300.0f, fAttr (msText)));
        }
        for (int k = 0; k < bitera::numModSlots; ++k)
        {
            const String n = "mod" + String (k + 1) + "_", N = "Mod " + String (k + 1) + " ";
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { n + "src", 1 }, N + "Source", srcs, 0));
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { n + "dst", 1 }, N + "Destination", dsts, 0));
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { n + "layer", 1 }, N + "Layer", StringArray { "ALL", "L1", "L2", "L3", "L4" }, 0));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { n + "amt", 1 }, N + "Amount", NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f,
                fAttr ([] (float v, int) { return (v > 0 ? "+" : "") + String (roundToInt (v * 100.0f)) + " %"; })));
        }
        for (int k = 0; k < bitera::numMacros; ++k)
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { "macro" + String (k + 1), 1 }, "Macro " + String (k + 1),
                                                                     NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f, fAttr (pctText)));
        for (auto n : { "xy1x", "xy1y", "xy2x", "xy2y" })
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { n, 1 }, String ("XY ") + String (n).substring (2, 3) + " " + String (n).getLastCharacters (1).toUpperCase(),
                                                                     NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, fAttr (pctText)));
        layout.add (std::move (group));
    }

    // master FX
    {
        auto group = std::make_unique<AudioProcessorParameterGroup> ("master", "Master", "|");
        StringArray fxTypes;
        for (int t = 0; t < bitera::numFxTypes; ++t) fxTypes.add (bitera::fxName (t));
        for (int k = 0; k < bitera::masterFxSlots; ++k)
        {
            const String F = "Master FX " + String (k + 1) + " ";
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { fxId (-1, k, "type"), 1 }, F + "Type", fxTypes, 0));
            for (int q = 0; q < 3; ++q)
                group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { fxId (-1, k, ("p" + std::to_string (q + 1)).c_str()), 1 }, F + "Control " + String (q + 1),
                                                                         NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, fAttr (pctText)));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { fxId (-1, k, "mix"), 1 }, F + "Mix", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, fAttr (pctText)));
        }
        layout.add (std::move (group));
    }

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "bend", 1 }, "Bend Range", NormalisableRange<float> (0.0f, 24.0f, 1.0f), 2.0f,
        fAttr ([] (float v, int) { return String (roundToInt (v)) + " st"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume", NormalisableRange<float> (-36.0f, 6.0f, 0.1f), -6.0f,
        fAttr ([] (float v, int) { return String (v, 1) + " dB"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "tone", 1 }, "Tone", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, fAttr (pctText)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "space", 1 }, "Space", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.15f, fAttr (pctText)));
    return layout;
}

void BitEraProcessor::loadBank()
{
    juce::WavAudioFormat wav;
    bank.resize ((size_t) bitera::numPresets);
    for (int i = 0; i < bitera::numPresets; ++i)
    {
        auto& s = bank[(size_t) i];
        s.name = bitera::sound (i).name;
        s.root = bitera::sound (i).root;
        int size = 0;
        auto* data = findResource (bitera::sound (i).resource, size);
        if (data == nullptr) continue;
        std::unique_ptr<juce::AudioFormatReader> reader (wav.createReaderFor (new juce::MemoryInputStream (data, (size_t) size, false), true));
        if (reader == nullptr) continue;
        juce::AudioBuffer<float> buf ((int) reader->numChannels, (int) reader->lengthInSamples);
        reader->read (&buf, 0, buf.getNumSamples(), 0, true, true);
        s.sampleRate = reader->sampleRate;
        s.data.assign ((size_t) buf.getNumSamples(), 0.0f);
        for (int c = 0; c < buf.getNumChannels(); ++c)
            for (int k = 0; k < buf.getNumSamples(); ++k)
                s.data[(size_t) k] += buf.getSample (c, k) / (float) buf.getNumChannels();
        s.blockPeak.assign ((s.data.size() + 15) / 16, 0.0f);
        for (size_t k = 0; k < s.data.size(); ++k)
            s.blockPeak[k / 16] = juce::jmax (s.blockPeak[k / 16], std::abs (s.data[k]));
    }
    bitera::chips::registerBank (&bank);
}

void BitEraProcessor::readParams (bitera::EngineParams& p)
{
    auto fxRead = [] (const FxPtrs& f, bitera::FxParams& o)
    {
        o.type = (int) f.type->load(); o.p[0] = f.p1->load(); o.p[1] = f.p2->load(); o.p[2] = f.p3->load(); o.mix = f.mix->load();
    };
    // step tables: take the newest copy if the message thread is not writing it right now
    if (audioStepsVersion != stepsVersion.load())
    {
        const juce::SpinLock::ScopedTryLockType lock (stepLock);
        if (lock.isLocked())
        {
            std::copy (std::begin (pendingSteps), std::end (pendingSteps), std::begin (audioSteps));
            audioStepsVersion = stepsVersion.load();
        }
    }
    for (int i = 0; i < bitera::numLayers; ++i)
    {
        const auto& l = lp[i];
        auto& o = p.layer[i];
        o.on = l.on->load() > 0.5f;
        o.source = (int) l.source->load(); o.wave = (int) l.wave->load();
        o.level = l.level->load(); o.pan = l.pan->load(); o.coarse = l.coarse->load(); o.fine = l.fine->load();
        o.character = l.character->load(); o.velSens = l.velSens->load();
        o.keyLo = (int) l.keyLo->load(); o.keyHi = (int) l.keyHi->load();
        o.a = l.a->load() * 0.001f; o.d = l.d->load() * 0.001f; o.s = l.s->load(); o.r = l.r->load() * 0.001f;
        o.filter = (int) l.filter->load(); o.cutoff = l.cutoff->load(); o.reso = l.reso->load();
        o.fltEnv = l.fltEnv->load(); o.keyTrack = l.keyTrack->load();
        o.fa = l.fa->load() * 0.001f; o.fd = l.fd->load() * 0.001f; o.fs = l.fs->load(); o.fr = l.fr->load() * 0.001f;
        o.pitchEnv = l.pitchEnv->load(); o.pitchDecay = l.pitchDecay->load() * 0.001f;
        o.vibDepth = l.vibDepth->load(); o.vibRate = l.vibRate->load();
        o.mode = (int) l.mode->load(); o.glide = l.glide->load() * 0.001f;
        o.arpMode = (int) l.arp->load(); o.arpDir = (int) l.arpDir->load(); o.arpSync = (int) l.arpRate->load();
        o.arpOctaves = (int) l.arpOct->load(); o.arpSteps = (int) l.arpSteps->load();
        o.arpGate = l.arpGate->load(); o.arpSwing = l.arpSwing->load(); o.arpEnd = (int) l.arpEnd->load();
        const auto& st = audioSteps[i];
        for (int k = 0; k < bitera::maxSteps; ++k)
        {
            o.stepVel[k] = st.vel[k]; o.stepGate[k] = st.gate[k]; o.stepPan[k] = st.pan[k];
            o.stepNote[k] = st.note[k]; o.stepWave[k] = st.wave[k]; o.stepOn[k] = st.on[k];
        }
        for (int k = 0; k < bitera::layerFxSlots; ++k) fxRead (l.fx[k], o.fx[k]);
    }
    for (int k = 0; k < bitera::masterFxSlots; ++k) fxRead (mfx[k], p.masterFx[k]);
    for (int k = 0; k < bitera::numLfos; ++k)
        p.lfo[k] = { (int) lfo[k].shape->load(), lfo[k].rate->load(), (int) lfo[k].sync->load(), lfo[k].retrig->load() > 0.5f,
                     lfo[k].fade->load() * 0.001f, lfo[k].uni->load() > 0.5f };
    for (int k = 0; k < bitera::numModEnvs; ++k)
        p.menv[k] = { menv[k].a->load() * 0.001f, menv[k].d->load() * 0.001f, menv[k].s->load(), menv[k].r->load() * 0.001f };
    for (int k = 0; k < bitera::numModSlots; ++k)
        p.slot[k] = { (int) slot[k].src->load(), (int) slot[k].dst->load(), (int) slot[k].layer->load(), slot[k].amt->load() };
    for (int k = 0; k < bitera::numMacros; ++k) p.macro[k] = macro[k]->load();
    for (int k = 0; k < 4; ++k) p.xy[k] = xy[k]->load();
    p.bendRange = pBend->load();
    p.volumeDb = pVolume->load(); p.tone = pTone->load(); p.space = pSpace->load();
}

int BitEraProcessor::getPreset() const
{
    return juce::jlimit (0, bitera::numPresets - 1, (int) state.state.getProperty ("preset", defaultPreset));
}

void BitEraProcessor::setParam (const juce::String& id, float value)
{
    if (auto* p = state.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BitEraProcessor::setPreset (int preset)
{
    preset = juce::jlimit (0, bitera::numPresets - 1, preset);
    state.state.setProperty ("preset", preset, nullptr);
    // every parameter back to its default (except the master volume), then the preset's own settings
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->getParameterID() != "volume")
                rp->setValueNotifyingHost (rp->getDefaultValue());
    for (int i = 0; i < bitera::numLayers; ++i)
    {
        auto steps = stepsTree (i);
        steps.removeAllProperties (nullptr);
    }
    // era preset: layer 1 plays the era sound
    setParam (layerId (0, "on"), 1.0f);
    setParam (layerId (0, "wave"), (float) preset);
    pushSteps();
}

juce::ValueTree BitEraProcessor::stepsTree (int layer)
{
    auto steps = state.state.getOrCreateChildWithName ("steps", nullptr);
    return steps.getOrCreateChildWithName ("layer" + juce::String (layer + 1), nullptr);
}

float BitEraProcessor::getStep (int layer, const char* field, int step) const
{
    const juce::String f (field);
    const float def = (f == "vel" || f == "gate" || f == "on") ? 1.0f : (f == "wave" ? -1.0f : 0.0f);
    auto steps = state.state.getChildWithName ("steps").getChildWithName ("layer" + juce::String (layer + 1));
    return steps.isValid() ? (float) steps.getProperty (f + juce::String (step), def) : def;
}

void BitEraProcessor::setStep (int layer, const char* field, int step, float value)
{
    stepsTree (layer).setProperty (juce::String (field) + juce::String (step), value, nullptr);
    pushSteps();
}

void BitEraProcessor::pushSteps()
{
    const juce::SpinLock::ScopedLockType lock (stepLock);
    for (int l = 0; l < bitera::numLayers; ++l)
        for (int k = 0; k < bitera::maxSteps; ++k)
        {
            auto& t = pendingSteps[l];
            t.vel[k] = getStep (l, "vel", k); t.gate[k] = getStep (l, "gate", k); t.pan[k] = getStep (l, "pan", k);
            t.note[k] = juce::roundToInt (getStep (l, "note", k)); t.wave[k] = juce::roundToInt (getStep (l, "wave", k));
            t.on[k] = getStep (l, "on", k) > 0.5f;
        }
    ++stepsVersion;
}

int BitEraProcessor::previewNote (int preset) const
{
    return bitera::sound (juce::jlimit (0, bitera::numPresets - 1, preset)).root;
}

bool BitEraProcessor::getSoundWav (int preset, juce::MemoryBlock& out) const
{
    int size = 0;
    auto* data = findResource (bitera::sound (juce::jlimit (0, bitera::numPresets - 1, preset)).resource, size);
    if (data == nullptr) return false;
    out.replaceAll (data, (size_t) size);
    return true;
}

void BitEraProcessor::previewPreset (int preset)
{
    setPreset (preset);
    previewRequest = preset;
}

void BitEraProcessor::stopPreview() { previewStop = 1; }

double BitEraProcessor::getSoundLengthSeconds (int preset) const
{
    const auto& s = bank[(size_t) juce::jlimit (0, bitera::numPresets - 1, preset)];
    return s.data.empty() ? 1.0 : (double) s.data.size() / s.sampleRate;
}

const juce::String BitEraProcessor::getProgramName (int index)
{
    return juce::String (bitera::era (bitera::eraOfPreset (index)).year) + " " + bitera::sound (index).name;
}

bool BitEraProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void BitEraProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
}

void BitEraProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    readParams (params);
    params.bpm = 120.0; params.playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) params.bpm = *bpm;
            if (auto ppq = pos->getPpqPosition()) params.ppq = *ppq;
            params.playing = pos->getIsPlaying();
        }

    // play buttons in the editor: a note on the sound's own key at the start of this block
    if (previewStop.exchange (0) != 0 && previewPlaying >= 0)
    {
        midi.addEvent (juce::MidiMessage::noteOff (1, previewPlaying), 0);
        previewPlaying = -1;
    }
    const int req = previewRequest.exchange (-1);
    if (req >= 0)
    {
        if (previewPlaying >= 0)
            midi.addEvent (juce::MidiMessage::noteOff (1, previewPlaying), 0);
        previewPlaying = previewNote (req);
        midi.addEvent (juce::MidiMessage::noteOn (1, previewPlaying, (juce::uint8) 110), 0);
    }
    engine.process (buffer, midi, params);
}

bool BitEraProcessor::renderCurrentWav (juce::MemoryBlock& out)
{
    constexpr double rate = 44100.0;
    constexpr int block = 512;
    bitera::Engine offline;
    offline.setBank (&bank);
    offline.prepare (rate, block);
    bitera::EngineParams p;
    readParams (p);
    const int note = previewNote (getPreset());
    const bool sampleLayer = p.layer[0].on && bitera::chips::isSampler (p.layer[0].source);
    const double holdSec = sampleLayer ? juce::jmin (12.0, getSoundLengthSeconds (getPreset()) + 0.05) : 1.5;
    const int holdSamples = (int) (holdSec * rate), maxSamples = (int) ((holdSec + 8.0) * rate);

    juce::AudioBuffer<float> result (2, maxSamples);
    result.clear();
    int written = 0, quiet = 0;
    while (written < maxSamples)
    {
        juce::AudioBuffer<float> buf (2, block);
        buf.clear();
        juce::MidiBuffer midi;
        if (written == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 110), 0);
        if (written <= holdSamples && written + block > holdSamples) midi.addEvent (juce::MidiMessage::noteOff (1, note), holdSamples - written);
        offline.process (buf, midi, p);
        const int n = juce::jmin (block, maxSamples - written);
        for (int c = 0; c < 2; ++c) result.copyFrom (c, written, buf, c, 0, n);
        written += n;
        if (written > holdSamples)
        {
            quiet = (offline.isSilent() && buf.getMagnitude (0, n) < 1.0e-4f) ? quiet + n : 0;
            if (quiet > (int) (0.3 * rate)) break;
        }
    }
    // trim the silent end
    int end = written;
    while (end > 1 && std::abs (result.getSample (0, end - 1)) < 1.0e-4f && std::abs (result.getSample (1, end - 1)) < 1.0e-4f) --end;
    end = juce::jmin (written, end + 64);

    out.reset();
    std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::MemoryOutputStream> (out, false);
    juce::WavAudioFormat wav;
    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate (rate).withNumChannels (2).withBitsPerSample (24));
    if (writer == nullptr) return false;
    writer->writeFromAudioSampleBuffer (result, 0, end);
    writer.reset();
    return out.getSize() > 100;
}

juce::AudioProcessorEditor* BitEraProcessor::createEditor()
{
#if BE_HEADLESS
    return nullptr;
#else
    return new BitEraEditor (*this);
#endif
}

void BitEraProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BitEraProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
    pushSteps();
}

#if ! BE_HEADLESS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BitEraProcessor(); }
#endif
