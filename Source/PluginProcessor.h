#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include "Eras.h"
#include "Engine.h"

// BIT ERA: 4-layer chip / sampler engine (see Engine.h), the era sample bank and the era presets.
// Parameters: l1_..l4_ per layer, m_ for the master. The preset index lives in the state (not a parameter):
// loading a preset writes the layer parameters, so every preset stays fully editable and automatable.
class BitEraProcessor : public juce::AudioProcessor
{
public:
    BitEraProcessor();
    ~BitEraProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Bit Era"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return bitera::numPresets; }
    int getCurrentProgram() override { return getPreset(); }
    void setCurrentProgram (int index) override { setPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // presets
    int getPreset() const;
    void setPreset (int preset);          // loads the preset into the layer parameters
    void previewPreset (int preset);      // loads + plays it at its root key (the play buttons)
    void stopPreview();
    double getSoundLengthSeconds (int preset) const;
    int previewNote (int preset) const;
    bool getSoundWav (int preset, juce::MemoryBlock& out) const;   // the era sound as it is
    // renders what the instrument plays right now (current layers + master) at the preset's root key to a WAV
    bool renderCurrentWav (juce::MemoryBlock& out);

    bool introSeen() const { return state.state.getProperty ("introSeen", false); }
    void setIntroSeen() { state.state.setProperty ("introSeen", true, nullptr); }

    const bitera::chips::Bank& getBank() const { return bank; }
    juce::AudioProcessorValueTreeState state;

    static juce::String layerId (int layer, const char* name) { return "l" + juce::String (layer + 1) + "_" + name; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void loadBank();
    void readParams (bitera::EngineParams& p) const;
    void setParam (const juce::String& id, float value);

    bitera::chips::Bank bank;
    bitera::Engine engine;
    bitera::EngineParams params;

    std::atomic<int> previewRequest { -1 }, previewStop { 0 };
    int previewPlaying = -1;

    struct LayerPtrs
    {
        std::atomic<float> *on, *source, *wave, *level, *pan, *coarse, *fine, *character, *velSens, *keyLo, *keyHi,
                           *a, *d, *s, *r, *filter, *cutoff, *reso, *fltEnv, *keyTrack, *fa, *fd, *fs, *fr,
                           *pitchEnv, *pitchDecay, *vibDepth, *vibRate;
    } lp[bitera::numLayers] {};
    std::atomic<float> *pMode = nullptr, *pGlide = nullptr, *pBend = nullptr, *pVolume = nullptr, *pTone = nullptr, *pSpace = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BitEraProcessor)
};
