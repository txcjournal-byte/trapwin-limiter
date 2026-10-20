#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include "Eras.h"

// BIT ERA engine: 36 embedded sounds (3 per era), one active preset at a time, played from MIDI on a pitched
// sampler (root C3 = MIDI 60). Volume / ADSR / Tone (low-pass) / Space (reverb) on the output.
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
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return bitera::numPresets; }
    int getCurrentProgram() override { return getPreset(); }
    void setCurrentProgram (int index) override { setPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // presets
    int getPreset() const;
    void setPreset (int preset);
    // plays the active preset at its root note (the play buttons); length in seconds
    void previewPreset (int preset);
    void stopPreview();
    double getSoundLengthSeconds (int preset) const;
    // the raw WAV of a preset (for Drag WAV)
    bool getSoundWav (int preset, juce::MemoryBlock& out) const;

    bool introSeen() const { return state.state.getProperty ("introSeen", false); }
    void setIntroSeen() { state.state.setProperty ("introSeen", true, nullptr); }

    juce::AudioProcessorValueTreeState state;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void loadSounds();
    void updateEnvelopes();

    struct EraSound;
    juce::Synthesiser synth;
    std::atomic<int> activePreset { bitera::numPresets - 3 };
    std::atomic<int> previewRequest { -1 }, previewStop { 0 };
    int previewNote = -1;
    double soundLength[bitera::numPresets] {};

    juce::dsp::StateVariableTPTFilter<float> tone;
    juce::dsp::Reverb reverb;
    juce::SmoothedValue<float> gain;
    float lastA = -1, lastD = -1, lastR = -1;

    std::atomic<float>* pVolume = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pTone = nullptr;
    std::atomic<float>* pSpace = nullptr;
    std::atomic<float>* pPreset = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BitEraProcessor)
};
