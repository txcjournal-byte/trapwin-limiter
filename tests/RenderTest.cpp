// Bit Era render test: every preset must load, sound, stay finite and below clipping through the full chain.
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <set>

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    BitEraProcessor proc;
    const double sr = 48000.0;
    const int block = 512;
    proc.setPlayConfigDetails (0, 2, sr, block);
    proc.prepareToPlay (sr, block);

    int failures = 0;
    // the whole factory bank: every preset must load, sound, stay finite and below clipping - and sound different
    // from every other preset (fingerprint = RMS envelope + zero crossings + stereo balance)
    const bool quick = juce::SystemStats::getEnvironmentVariable ("BE_QUICK", {}).isNotEmpty();
    std::vector<std::vector<float>> prints;
    std::vector<int> printPreset;
    std::set<juce::String> names;
    int quiet = 0, hot = 0;
    for (int preset = 0; preset < bitera::numPresets; preset += (quick ? 37 : 1))
    {
        const auto& info = bitera::presetInfo (preset);
        if (! names.insert (info.name).second) { std::cout << "FAIL duplicate name " << info.name << "\n"; ++failures; }
        proc.setPreset (preset);
        proc.prepareToPlay (sr, block);   // fresh engine
        const double hold = juce::jlimit (0.4, 1.6, proc.getSoundLengthSeconds (preset));
        const int blocks = (int) ((hold + 0.4) * sr / block), offAt = (int) (hold * sr / block);
        float peak = 0.0f; bool finite = true;
        std::vector<float> fp;
        double zc = 0, lr = 0;
        float last = 0.0f;
        for (int b = 0; b < blocks; ++b)
        {
            juce::AudioBuffer<float> buf (2, block);
            juce::MidiBuffer midi;
            if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, info.root, (juce::uint8) 110), 0);
            if (b == offAt) midi.addEvent (juce::MidiMessage::noteOff (1, info.root), 0);
            proc.processBlock (buf, midi);
            double e = 0;
            for (int i = 0; i < block; ++i)
            {
                const float l = buf.getSample (0, i), r = buf.getSample (1, i);
                if (! std::isfinite (l) || ! std::isfinite (r)) finite = false;
                peak = juce::jmax (peak, std::abs (l), std::abs (r));
                e += (double) l * l + (double) r * r;
                if ((l >= 0) != (last >= 0)) zc += 1; last = l;
                lr += (double) l * l - (double) r * r;
            }
            fp.push_back ((float) std::sqrt (e / (2 * block)));
        }
        fp.push_back ((float) (zc / (blocks * block)) * 10.0f);
        fp.push_back ((float) (lr / (blocks * block)));
        const bool ok = finite && peak > 0.003f && peak < 1.6f;
        if (peak < 0.02f) ++quiet;
        if (peak > 1.0f) ++hot;
        if (! ok)
        {
            ++failures;
            std::cout << "FAIL " << preset << " " << bitera::era (info.era).year << " " << bitera::categoryName (info.category) << " " << info.name << "  peak " << peak << "\n";
        }
        prints.push_back (std::move (fp));
        printPreset.push_back (preset);
    }
    int dups = 0;
    for (size_t a = 0; a < prints.size(); ++a)
        for (size_t b = a + 1; b < prints.size(); ++b)
        {
            if (prints[a].size() != prints[b].size()) continue;
            float d = 0;
            for (size_t k = 0; k < prints[a].size() && d < 1.0e-4f; ++k) d = juce::jmax (d, std::abs (prints[a][k] - prints[b][k]));
            if (d < 1.0e-4f)
            {
                ++dups;
                if (dups < 20) std::cout << "FAIL same sound: " << bitera::presetInfo (printPreset[a]).name << " = " << bitera::presetInfo (printPreset[b]).name << "\n";
            }
        }
    failures += dups;
    std::cout << "bank: " << prints.size() << " presets rendered, " << names.size() << " unique names, " << dups << " duplicate sounds, "
              << quiet << " quiet (<0.02), " << hot << " hot (>1.0)\n";

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

    // helper: play notes for a while, return peak (and finite flag)
    auto playFor = [&] (std::initializer_list<int> notes, double seconds, bool& finite)
    {
        float peak = 0.0f; finite = true;
        const int blocks = (int) (seconds * sr / block);
        for (int b = 0; b < blocks + (int) (2.0 * sr / block); ++b)
        {
            juce::AudioBuffer<float> buf (2, block);
            juce::MidiBuffer midi;
            if (b == 0) for (int nn : notes) midi.addEvent (juce::MidiMessage::noteOn (1, nn, (juce::uint8) 100), 0);
            if (b == blocks) for (int nn : notes) midi.addEvent (juce::MidiMessage::noteOff (1, nn), 0);
            proc.processBlock (buf, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < block; ++i) { const float v = buf.getSample (ch, i); if (! std::isfinite (v)) finite = false; peak = juce::jmax (peak, std::abs (v)); }
        }
        return peak;
    };

    // every FX type on a layer slot and a master slot
    for (int t = 1; t < bitera::numFxTypes; ++t)
    {
        proc.setPreset (5);
        setP ("l1_src", 1.0f); setP ("l1_wave", 2.0f);
        setP (BitEraProcessor::fxId (0, 0, "type"), (float) t); setP (BitEraProcessor::fxId (0, 0, "mix"), 1.0f);
        setP (BitEraProcessor::fxId (-1, 0, "type"), (float) t); setP (BitEraProcessor::fxId (-1, 0, "p1"), 0.9f);
        bool finite = true;
        const float peak = playFor ({ 60, 64, 67 }, 0.8, finite);
        const bool ok = finite && peak > 0.002f && peak < 2.0f;
        if (! ok) ++failures;
        std::cout << (ok ? "ok   " : "FAIL ") << "FX " << bitera::fxName (t) << "  peak " << peak << "\n";
    }

    // arpeggiator + sequencer with swing, octaves, rests and per-step pitch
    for (int mode = 1; mode <= 2; ++mode)
        for (int dir = 0; dir < bitera::numArpDirs; ++dir)
        {
            proc.setPreset (60);
            setP ("l1_src", 2.0f); setP ("l1_wave", 1.0f);
            setP ("l1_arp", (float) mode); setP ("l1_arpdir", (float) dir); setP ("l1_arpoct", 2.0f); setP ("l1_arpswing", 0.4f);
            proc.setStep (0, "on", 2, 0.0f); proc.setStep (0, "note", 1, 7.0f); proc.setStep (0, "vel", 3, 0.5f);
            bool finite = true;
            const float peak = playFor ({ 60, 63, 67 }, 1.2, finite);
            const bool ok = finite && peak > 0.005f && peak < 1.5f;
            if (! ok) ++failures;
            std::cout << (ok ? "ok   " : "FAIL ") << (mode == 1 ? "ARP " : "SEQ ") << bitera::arpDirName (dir) << "  peak " << peak << "\n";
        }

    // modulation: every source onto cutoff / pitch / level, plus macros onto master
    for (int src = 1; src < bitera::numModSources; ++src)
    {
        proc.setPreset (30);
        setP ("l1_src", 3.0f); setP ("l1_filter", 1.0f); setP ("l1_cutoff", 1500.0f);
        setP ("mod1_src", (float) src); setP ("mod1_dst", (float) bitera::mdCutoff); setP ("mod1_amt", 0.8f);
        setP ("mod2_src", (float) src); setP ("mod2_dst", (float) bitera::mdPitch); setP ("mod2_amt", 0.2f);
        setP ("mod3_src", (float) src); setP ("mod3_dst", (float) bitera::mdTone); setP ("mod3_amt", -0.5f);
        setP ("macro1", 0.7f); setP ("lfo1_sync", 6.0f); setP ("lfo2_shape", 5.0f); setP ("lfo3_shape", 6.0f);
        bool finite = true;
        const float peak = playFor ({ 48, 60 }, 0.6, finite);
        const bool ok = finite && peak > 0.005f && peak < 1.5f;
        if (! ok) ++failures;
        std::cout << (ok ? "ok   " : "FAIL ") << "MOD " << bitera::modSourceName (src) << "  peak " << peak << "\n";
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
