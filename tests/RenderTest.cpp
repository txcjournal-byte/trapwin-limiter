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

    // every chip source and wave: plays C4 for 0.8 s on layer 1 (with layer 2 detuned on top), must sound and stay sane
    auto setP = [&] (const juce::String& id, float v) { if (auto* p = proc.state.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    for (int src = 1; src < bitera::chips::numSources; ++src)
    {
        const int waves = juce::jmin (bitera::chips::numWaves (src), bitera::chips::isSampler (src) ? 6 : 64);
        for (int w = 0; w < waves; ++w)
        {
            proc.setPreset (0);
            const int wave = bitera::chips::isSampler (src) ? w * 12 : w;
            setP ("l1_src", (float) src); setP ("l1_wave", (float) wave);
            setP ("l1_filter", (float) (w % bitera::numFilterTypes)); setP ("l1_cutoff", 3000.0f); setP ("l1_reso", 0.7f); setP ("l1_fenv", 0.5f);
            setP ("l2_on", 1.0f); setP ("l2_src", (float) src); setP ("l2_wave", (float) wave); setP ("l2_coarse", 12.0f); setP ("l2_vibdepth", 30.0f);
            const int note = bitera::chips::isSampler (src) ? bitera::sound (wave).root : 60;
            float peak = 0.0f; bool finite = true;
            for (int b = 0; b < (int) (1.6 * sr / block); ++b)
            {
                juce::AudioBuffer<float> buf (2, block);
                juce::MidiBuffer midi;
                if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
                if (b == (int) (0.8 * sr / block)) midi.addEvent (juce::MidiMessage::noteOff (1, note), 0);
                proc.processBlock (buf, midi);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < block; ++i) { const float v = buf.getSample (ch, i); if (! std::isfinite (v)) finite = false; peak = juce::jmax (peak, std::abs (v)); }
            }
            for (int b = 0; b < (int) (2.5 * sr / block); ++b) { juce::AudioBuffer<float> buf (2, block); juce::MidiBuffer m; proc.processBlock (buf, m); }
            const bool ok = finite && peak > 0.005f && peak < 1.5f;
            if (! ok) ++failures;
            std::cout << (ok ? "ok   " : "FAIL ") << bitera::chips::sourceName (src) << " / " << bitera::chips::waveName (src, wave) << "  peak " << peak << "\n";
        }
    }

    // Drag WAV render of a chip patch
    {
        juce::MemoryBlock wav;
        const bool ok = proc.renderCurrentWav (wav) && wav.getSize() > 10000;
        if (! ok) ++failures;
        std::cout << (ok ? "ok   " : "FAIL ") << "render to WAV  " << wav.getSize() << " bytes\n";
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
