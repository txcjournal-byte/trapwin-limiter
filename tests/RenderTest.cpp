// Bit Era render test: every preset must load, sound, stay finite and below clipping through the full chain.
#include "../Source/PluginProcessor.h"
#include <iostream>

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    BitEraProcessor proc;
    const double sr = 48000.0;
    const int block = 512;
    proc.setPlayConfigDetails (0, 2, sr, block);
    proc.prepareToPlay (sr, block);

    int failures = 0;
    for (int preset = 0; preset < bitera::numPresets; ++preset)
    {
        proc.setPreset (preset);
        const double len = proc.getSoundLengthSeconds (preset);
        juce::MemoryBlock wav;
        if (len <= 0.0 || ! proc.getSoundWav (preset, wav) || wav.getSize() < 1000)
        {
            std::cout << "FAIL preset " << preset << ": sound missing\n";
            ++failures;
            continue;
        }
        float peak = 0.0f; double energy = 0.0; bool finite = true;
        const int blocks = (int) std::ceil (juce::jmin (len, 3.0) * sr / block);
        for (int b = 0; b < blocks; ++b)
        {
            juce::AudioBuffer<float> buf (2, block);
            juce::MidiBuffer midi;
            if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, bitera::sound (preset).root, (juce::uint8) 110), 0);
            if (b == blocks - 1) midi.addEvent (juce::MidiMessage::noteOff (1, bitera::sound (preset).root), 0);
            proc.processBlock (buf, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < block; ++i)
                {
                    const float v = buf.getSample (ch, i);
                    if (! std::isfinite (v)) finite = false;
                    peak = juce::jmax (peak, std::abs (v));
                    energy += (double) v * v;
                }
        }
        // let the voice and reverb tail die before the next preset
        for (int b = 0; b < (int) (2.5 * sr / block); ++b) { juce::AudioBuffer<float> buf (2, block); juce::MidiBuffer m; proc.processBlock (buf, m); }

        const bool ok = finite && peak > 0.01f && peak < 1.2f;
        std::cout << (ok ? "ok   " : "FAIL ") << bitera::era (bitera::eraOfPreset (preset)).year << " " << bitera::sound (preset).name
                  << "  len " << len << "s  peak " << peak << "\n";
        if (! ok) ++failures;
    }

    // state round trip
    proc.setPreset (7);
    juce::MemoryBlock state;
    proc.getStateInformation (state);
    proc.setPreset (0);
    proc.setStateInformation (state.getData(), (int) state.getSize());
    if (proc.getPreset() != 7) { std::cout << "FAIL state round trip\n"; ++failures; }

    std::cout << (failures == 0 ? "ALL PASSED" : "FAILURES: " + std::to_string (failures)) << "\n";
    return failures == 0 ? 0 : 1;
}
