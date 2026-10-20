// Renders demo WAVs of chosen presets: BitEraRenderDemo <outdir> <preset> [<preset> ...]
#include "../Source/PluginProcessor.h"
#include <iostream>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 3) { std::cout << "usage: BitEraRenderDemo <outdir> <preset>...\n"; return 1; }
    const juce::File out { juce::String (argv[1]) };
    out.createDirectory();
    BitEraProcessor proc;
    proc.setPlayConfigDetails (0, 2, 44100.0, 512);
    // BitEraRenderDemo <outdir> --all [firstEra lastEra]: the whole bank as <outdir>/<era>/<category>/<nnnn> <name>.wav
    if (juce::String (argv[2]) == "--all")
    {
        const int e0 = argc > 3 ? juce::String (argv[3]).getIntValue() : 0, e1 = argc > 4 ? juce::String (argv[4]).getIntValue() : bitera::numEras - 1;
        for (int preset = e0 * bitera::presetsPerEra; preset < (e1 + 1) * bitera::presetsPerEra; ++preset)
        {
            proc.setPreset (preset);
            juce::MemoryBlock wav;
            if (! proc.renderCurrentWav (wav)) { std::cout << "FAIL " << preset << "\n"; continue; }
            const auto& info = bitera::presetInfo (preset);
            const auto& E = bitera::era (info.era);
            auto dir = out.getChildFile (juce::String (info.era + 1).paddedLeft ('0', 2) + " " + E.year + " " + E.shortName)
                          .getChildFile (bitera::categoryName (info.category));
            dir.createDirectory();
            auto f = dir.getChildFile (juce::File::createLegalFileName ("Bit Era " + juce::String (E.year) + " - " + info.name + ".wav"));
            f.replaceWithData (wav.getData(), wav.getSize());
            if (preset % 100 == 0) std::cout << preset << "\n";
        }
        return 0;
    }
    for (int a = 2; a < argc; ++a)
    {
        const juce::String arg (argv[a]);
        const int preset = arg.upToFirstOccurrenceOf ("@", false, false).getIntValue();
        const int note = arg.contains ("@") ? arg.fromFirstOccurrenceOf ("@", false, false).getIntValue() : -1;
        proc.setPreset (preset);
        juce::MemoryBlock wav;
        if (! proc.renderCurrentWav (wav, note)) continue;
        const auto& info = bitera::presetInfo (preset);
        auto f = out.getChildFile (juce::File::createLegalFileName (juce::String (bitera::era (info.era).year) + " - " + bitera::categoryName (info.category)
                                                                     + " - " + info.name + (note >= 0 ? " @" + juce::String (note) : juce::String()) + ".wav"));
        f.replaceWithData (wav.getData(), wav.getSize());
        std::cout << f.getFileName() << "\n";
    }
    return 0;
}
