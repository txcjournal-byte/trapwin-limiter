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
    }
    pMode = raw ("mode"); pGlide = raw ("glide"); pBend = raw ("bend");
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
        group->addChild (std::make_unique<AudioParameterBool> (id ("on"), L + "On", i == 0));
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
        layout.add (std::move (group));
    }

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Voice Mode", StringArray { "POLY", "MONO", "LEGATO" }, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "glide", 1 }, "Glide", timeRange (0.0f, 2000.0f, 200.0f), 0.0f, fAttr (msText)));
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

void BitEraProcessor::readParams (bitera::EngineParams& p) const
{
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
    }
    p.mode = (int) pMode->load(); p.glide = pGlide->load() * 0.001f; p.bendRange = pBend->load();
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
    // era preset: layer 1 plays the era sound, the other layers are off
    for (int i = 0; i < bitera::numLayers; ++i)
    {
        auto set = [&] (const char* n, float v) { setParam (layerId (i, n), v); };
        set ("on", i == 0 ? 1.0f : 0.0f);
        set ("src", 0.0f); set ("wave", i == 0 ? (float) preset : 0.0f);
        set ("level", 0.8f); set ("pan", 0.0f); set ("coarse", 0.0f); set ("fine", 0.0f); set ("char", 0.5f); set ("vel", 0.5f);
        set ("keylo", 0.0f); set ("keyhi", 127.0f);
        set ("attack", 2.0f); set ("decay", 600.0f); set ("sustain", 1.0f); set ("release", 300.0f);
        set ("filter", 0.0f); set ("cutoff", 20000.0f); set ("reso", 0.1f); set ("fenv", 0.0f); set ("ktrack", 0.0f);
        set ("fattack", 2.0f); set ("fdecay", 400.0f); set ("fsustain", 0.0f); set ("frelease", 300.0f);
        set ("penv", 0.0f); set ("pdecay", 100.0f); set ("vibdepth", 0.0f); set ("vibrate", 5.5f);
    }
    setParam ("mode", 0.0f); setParam ("glide", 0.0f);
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
}

#if ! BE_HEADLESS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BitEraProcessor(); }
#endif
