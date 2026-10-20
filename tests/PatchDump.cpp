// prints the parameter list of presets: BitEraPatchDump <preset>...
#include "../Source/Presets.h"
#include <iostream>
int main (int argc, char** argv)
{
    for (int a = 1; a < argc; ++a)
    {
        const juce::String arg (argv[a]);
        int p = arg.getIntValue();
        if (! arg.containsOnly ("0123456789"))
            for (int i = 0; i < bitera::numPresets; ++i) if (bitera::presetInfo (i).name == arg) { p = i; break; }
        std::cout << "== " << p << " " << bitera::presetInfo (p).name << "\n";
        auto patch = bitera::buildPatch (p);
        for (auto& pr : patch.params) if (pr.first.startsWith ("l") && ! pr.first.startsWith ("lfo")) std::cout << "  " << pr.first << " = " << pr.second << "\n";
        for (auto& st : patch.steps) std::cout << "  step L" << st.layer + 1 << " " << st.field << st.step << " = " << st.value << "\n";
    }
}
