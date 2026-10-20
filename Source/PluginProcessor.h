#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include "Eras.h"
#include "Engine.h"
#include "Presets.h"

// BIT ERA: 4-layer chip / sampler engine (see Engine.h), the era sample bank and the presets.
// Parameter ids: l1_..l4_ per layer (incl. arp and l1_fx1_..), lfo1_.., menv1_.., mod1_.., macro1.., xy1x.., mfx1_..,
// and the master (bend, volume, tone, space). The arp / sequencer step tables live in the state tree ("steps").
// The preset index lives in the state: loading a preset writes all parameters, so every preset stays editable.
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
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return bitera::numPresets; }
    int getCurrentProgram() override { return getPreset(); }
    void setCurrentProgram (int index) override { setPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // presets
    int getPreset() const;
    void setPreset (int preset);          // loads the preset into all parameters
    void previewPreset (int preset);      // loads + plays it at its root key (the play buttons)
    void stopPreview();
    double getSoundLengthSeconds (int preset) const;
    int previewNote (int preset) const;
    bool getSoundWav (int preset, juce::MemoryBlock& out) const;   // the era sound as it is
    // renders what the instrument plays right now (layers, arp, FX, master) at the preset's root key to a WAV
    bool renderCurrentWav (juce::MemoryBlock& out);

    // arp / sequencer step tables: field = "vel", "gate", "pan", "note", "wave", "on"
    float getStep (int layer, const char* field, int step) const;
    void setStep (int layer, const char* field, int step, float value);
    int currentStep (int layer) const { return engine.currentStep (layer); }

    bool introSeen() const { return state.state.getProperty ("introSeen", false); }
    void setIntroSeen() { state.state.setProperty ("introSeen", true, nullptr); }

    const bitera::chips::Bank& getBank() const { return bank; }
    juce::AudioProcessorValueTreeState state;

    static juce::String layerId (int layer, const char* name) { return "l" + juce::String (layer + 1) + "_" + name; }
    static juce::String fxId (int layer, int slot, const char* name)   // layer -1 = master
    {
        return (layer < 0 ? "mfx" + juce::String (slot + 1) : layerId (layer, "fx") + juce::String (slot + 1)) + "_" + name;
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void loadBank();
    void readParams (bitera::EngineParams& p, bool messageThread = false);
    int renderNote (bitera::EngineParams& p, int note, double holdSec, double maxTailSec, juce::AudioBuffer<float>& result);
    void levelPreset();
    void setParam (const juce::String& id, float value);
    void pushSteps();   // message thread: copy the step tables for the audio thread
    juce::ValueTree stepsTree (int layer);

    bitera::chips::Bank bank;
    bitera::Engine engine;
    bitera::EngineParams params;

    std::atomic<int> previewRequest { -1 }, previewStop { 0 };
    int previewPlaying = -1;

    // all parameter values, looked up once
    std::atomic<float>* get (const juce::String& id) const { return state.getRawParameterValue (id); }
    struct FxPtrs { std::atomic<float> *type, *p1, *p2, *p3, *mix; };
    struct LayerPtrs
    {
        std::atomic<float> *on, *source, *wave, *level, *pan, *coarse, *fine, *character, *velSens, *keyLo, *keyHi,
                           *a, *d, *s, *r, *filter, *cutoff, *reso, *fltEnv, *keyTrack, *fa, *fd, *fs, *fr,
                           *pitchEnv, *pitchDecay, *vibDepth, *vibRate, *mode, *glide,
                           *arp, *arpDir, *arpRate, *arpOct, *arpSteps, *arpGate, *arpSwing, *arpEnd;
        FxPtrs fx[bitera::layerFxSlots];
    } lp[bitera::numLayers] {};
    struct { std::atomic<float> *shape, *rate, *sync, *retrig, *fade, *uni; } lfo[bitera::numLfos] {};
    struct { std::atomic<float> *a, *d, *s, *r; } menv[bitera::numModEnvs] {};
    struct { std::atomic<float> *src, *dst, *layer, *amt; } slot[bitera::numModSlots] {};
    std::atomic<float>* macro[bitera::numMacros] {};
    std::atomic<float>* xy[4] {};
    FxPtrs mfx[bitera::masterFxSlots] {};
    std::atomic<float> *pBend = nullptr, *pVolume = nullptr, *pTrim = nullptr, *pTone = nullptr, *pSpace = nullptr;

    struct StepTable
    {
        float vel[bitera::maxSteps], gate[bitera::maxSteps], pan[bitera::maxSteps];
        int note[bitera::maxSteps], wave[bitera::maxSteps];
        bool on[bitera::maxSteps];
    };
    StepTable pendingSteps[bitera::numLayers] {}, audioSteps[bitera::numLayers] {};
    juce::SpinLock stepLock;
    std::atomic<int> stepsVersion { 1 };
    int audioStepsVersion = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BitEraProcessor)
};
