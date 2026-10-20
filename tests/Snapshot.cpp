// Renders the Bit Era window for every era (and the first-launch screen) to PNG files: BitEraSnapshot <outdir>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::File out (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    out.createDirectory();
    BitEraProcessor proc;
    int page = 0;
    auto snap = [&] (const juce::String& name)
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        if (auto* be = dynamic_cast<BitEraEditor*> (ed.get())) be->showPage (page);
        ed->setSize (BitEraEditor::designWidth, BitEraEditor::designHeight);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat png;
        auto f = out.getChildFile (name + ".png");
        f.deleteFile();
        juce::FileOutputStream s (f);
        png.writeImageToStream (img, s);
    };
    snap ("00-intro");
    proc.setIntroSeen();
    for (int e = 0; e < bitera::numEras; ++e)
    {
        proc.setPreset (e * bitera::soundsPerEra);
        snap (juce::String (e + 1).paddedLeft ('0', 2) + "-" + bitera::era (e).year);
    }
    // engine pages
    page = 1;
    for (int e : { 0, 5, 8, 11 })
    {
        proc.setPreset (e * bitera::soundsPerEra);
        if (auto* p = proc.state.getParameter ("l1_filter")) p->setValueNotifyingHost (p->convertTo0to1 (2.0f));
        snap ("layers-" + juce::String (e + 1).paddedLeft ('0', 2));
    }
    return 0;
}
