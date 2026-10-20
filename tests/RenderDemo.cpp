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
        const int preset = juce::String (argv[a]).getIntValue();
        proc.setPreset (preset);
        juce::MemoryBlock wav;
        if (! proc.renderCurrentWav (wav)) continue;
        const auto& info = bitera::presetInfo (preset);
        auto f = out.getChildFile (juce::File::createLegalFileName (juce::String (bitera::era (info.era).year) + " - " + bitera::categoryName (info.category)
                                                                     + " - " + info.name + ".wav"));
        f.replaceWithData (wav.getData(), wav.getSize());
        std::cout << f.getFileName() << "\n";
    }
    return 0;
}
