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
        proc.setPreset (e * bitera::presetsPerEra);
        snap (juce::String (e + 1).paddedLeft ('0', 2) + "-" + bitera::era (e).year);
    }
    // engine pages
    page = 1;
    for (int e : { 0, 5, 8, 11 })
    {
        proc.setPreset (e * bitera::presetsPerEra);
        if (auto* p = proc.state.getParameter ("l1_filter")) p->setValueNotifyingHost (p->convertTo0to1 (2.0f));
        snap ("layers-" + juce::String (e + 1).paddedLeft ('0', 2));
    }
    // MOD / FX / ARP / MACRO pages with something set up
    proc.setPreset (6 * bitera::presetsPerEra);
    auto setP = [&] (const juce::String& id, float v) { if (auto* p = proc.state.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    setP ("mod1_src", 1); setP ("mod1_dst", 3); setP ("mod1_amt", 0.6f); setP ("mod2_src", 12); setP ("mod2_dst", 5); setP ("mod2_amt", -0.4f);
    setP ("mfx1_type", 1); setP ("mfx2_type", 7); setP ("mfx3_type", 8); setP ("l1_arp", 1); setP ("l1_arpsteps", 16);
    proc.setStep (0, "vel", 2, 0.4f); proc.setStep (0, "on", 5, 0.0f); proc.setStep (0, "note", 3, 7.0f);
    const char* pageNames[] = { "mod", "fx", "arp", "macro" };
    for (int pg = 2; pg < 6; ++pg) { page = pg; snap (juce::String ("page-") + pageNames[pg - 2]); }
    setP ("l1_arp", 2);
    page = 4; snap ("page-seq");
    proc.setPreset (5 * bitera::presetsPerEra + 130);
    page = 6; snap ("page-browse");
    page = 0; snap ("play-bank");
    return 0;
}
