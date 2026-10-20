#include "PluginProcessor.h"
#include "BitEraData.h"
#if ! BE_HEADLESS
 #include "PluginEditor.h"
#endif

namespace
{
constexpr int rootNote = 60;   // every sound plays at its recorded pitch on C3 (MIDI 60)
constexpr float sustainLevel = 0.8f;

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
} // namespace

// A sampler sound that only answers while its preset is the active one, so all 36 sounds stay loaded and
// switching presets is a single atomic write (no allocation on the audio thread).
struct BitEraProcessor::EraSound : public juce::SamplerSound
{
    EraSound (int idx, std::atomic<int>& active, juce::AudioFormatReader& reader)
        : juce::SamplerSound (bitera::sound (idx).name, reader, allNotes(), rootNote, 0.002, 0.3, 30.0),
          index (idx), activePreset (active) {}

    bool appliesToNote (int) override { return activePreset.load() == index; }
    bool appliesToChannel (int) override { return true; }

    static juce::BigInteger allNotes() { juce::BigInteger b; b.setRange (0, 128, true); return b; }

    const int index;
    std::atomic<int>& activePreset;
};

BitEraProcessor::BitEraProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "BitEra", createLayout())
{
    pVolume  = state.getRawParameterValue ("volume");
    pAttack  = state.getRawParameterValue ("attack");
    pDecay   = state.getRawParameterValue ("decay");
    pRelease = state.getRawParameterValue ("release");
    pTone    = state.getRawParameterValue ("tone");
    pSpace   = state.getRawParameterValue ("space");
    pPreset  = state.getRawParameterValue ("preset");

    for (int i = 0; i < 16; ++i)
        synth.addVoice (new juce::SamplerVoice());
    loadSounds();
    activePreset = (int) pPreset->load();
}

juce::AudioProcessorValueTreeState::ParameterLayout BitEraProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto ms = [] (float v, int) { return v < 1000.0f ? String (roundToInt (v)) + " ms" : String (v / 1000.0f, 2) + " s"; };
    auto pct = [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; };

    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "preset", 1 }, "Preset", 0, bitera::numPresets - 1, bitera::numPresets - 3,
        AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return String (bitera::era (bitera::eraOfPreset (v)).year) + " " + bitera::sound (v).name; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume", NormalisableRange<float> (-36.0f, 6.0f, 0.1f), -6.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })));
    NormalisableRange<float> attackRange (0.0f, 2000.0f, 1.0f); attackRange.setSkewForCentre (150.0f);
    NormalisableRange<float> timeRange (10.0f, 4000.0f, 1.0f); timeRange.setSkewForCentre (400.0f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "attack", 1 }, "Attack", attackRange, 2.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (ms)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "decay", 1 }, "Decay", timeRange, 600.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (ms)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "release", 1 }, "Release", timeRange, 300.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (ms)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "tone", 1 }, "Tone", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "space", 1 }, "Space", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.15f,
        AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    return layout;
}

void BitEraProcessor::loadSounds()
{
    juce::WavAudioFormat wav;
    for (int i = 0; i < bitera::numPresets; ++i)
    {
        int size = 0;
        auto* data = findResource (bitera::sound (i).resource, size);
        jassert (data != nullptr);
        if (data == nullptr)
            continue;
        auto stream = std::make_unique<juce::MemoryInputStream> (data, (size_t) size, false);
        std::unique_ptr<juce::AudioFormatReader> reader (wav.createReaderFor (stream.release(), true));
        if (reader == nullptr)
            continue;
        soundLength[i] = (double) reader->lengthInSamples / reader->sampleRate;
        synth.addSound (new EraSound (i, activePreset, *reader));
    }
}

double BitEraProcessor::getSoundLengthSeconds (int preset) const
{
    return soundLength[juce::jlimit (0, bitera::numPresets - 1, preset)];
}

bool BitEraProcessor::getSoundWav (int preset, juce::MemoryBlock& out) const
{
    int size = 0;
    auto* data = findResource (bitera::sound (preset).resource, size);
    if (data == nullptr)
        return false;
    out.replaceAll (data, (size_t) size);
    return true;
}

int BitEraProcessor::getPreset() const { return juce::jlimit (0, bitera::numPresets - 1, (int) pPreset->load()); }

void BitEraProcessor::setPreset (int preset)
{
    preset = juce::jlimit (0, bitera::numPresets - 1, preset);
    if (auto* p = state.getParameter ("preset"))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) preset));
    activePreset = preset;
}

void BitEraProcessor::previewPreset (int preset)
{
    setPreset (preset);
    previewRequest = preset;
}

void BitEraProcessor::stopPreview() { previewStop = 1; }

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
    synth.setCurrentPlaybackSampleRate (sampleRate);
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) juce::jmax (1, getTotalNumOutputChannels()) };
    tone.prepare (spec);
    tone.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    tone.setResonance (0.707f);
    reverb.prepare (spec);
    gain.reset (sampleRate, 0.03);
    gain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pVolume->load()));
    lastA = lastD = lastR = -1.0f;
    updateEnvelopes();
}

void BitEraProcessor::updateEnvelopes()
{
    const float a = pAttack->load(), d = pDecay->load(), r = pRelease->load();
    if (juce::exactlyEqual (a, lastA) && juce::exactlyEqual (d, lastD) && juce::exactlyEqual (r, lastR))
        return;
    lastA = a; lastD = d; lastR = r;
    juce::ADSR::Parameters env { a / 1000.0f, d / 1000.0f, sustainLevel, r / 1000.0f };
    for (int i = 0; i < synth.getNumSounds(); ++i)
        if (auto* s = dynamic_cast<juce::SamplerSound*> (synth.getSound (i).get()))
            s->setEnvelopeParameters (env);
}

void BitEraProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    activePreset = getPreset();
    updateEnvelopes();

    // play buttons in the editor: a note on the root key at the start of this block
    if (previewStop.exchange (0) != 0 && previewNote >= 0)
    {
        midi.addEvent (juce::MidiMessage::noteOff (1, previewNote), 0);
        previewNote = -1;
    }
    const int req = previewRequest.exchange (-1);
    if (req >= 0)
    {
        if (previewNote >= 0)
            midi.addEvent (juce::MidiMessage::noteOff (1, previewNote), 0);
        previewNote = rootNote;
        midi.addEvent (juce::MidiMessage::noteOn (1, rootNote, (juce::uint8) 110), 0);
    }

    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    const float toneAmt = pTone->load();
    tone.setCutoffFrequency (200.0f * std::pow (100.0f, toneAmt));
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    if (toneAmt < 0.995f)
        tone.process (ctx);

    const float space = pSpace->load();
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = 0.78f; rp.damping = 0.45f; rp.width = 1.0f;
    rp.wetLevel = space * 0.55f; rp.dryLevel = 1.0f - space * 0.35f;
    reverb.setParameters (rp);
    if (space > 0.001f)
        reverb.process (ctx);

    gain.setTargetValue (juce::Decibels::decibelsToGain (pVolume->load()));
    gain.applyGain (buffer, buffer.getNumSamples());
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
    activePreset = getPreset();
}

#if ! BE_HEADLESS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BitEraProcessor(); }
#endif
