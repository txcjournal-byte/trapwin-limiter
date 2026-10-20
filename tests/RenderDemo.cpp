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
