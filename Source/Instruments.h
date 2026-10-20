#pragma once
#include "Chips.h"

// BIT ERA instrument bank: the "ROM samples" the 16/32-bit consoles, trackers and 90s sound cards played.
// Synthesized at load (additive, FM, plucked-string and modal models) at 32 kHz; sustained instruments carry a
// seamless loop like the consoles' samples did. Played through the sampler chips they get each console's sound.
namespace bitera
{
enum Instrument
{
    insPiano = 0, insEPiano, insStrings, insChoirAah, insChoirOoh, insBrass, insTrumpet, insFlute, insClarinet, insSax, insOrgan,
    insFingerBass, insSlapBass, insNylonGuitar, insSteelGuitar, insDriveGuitar, insHarp, insPizzicato, insMarimba, insVibraphone,
    insGlock, insMusicBox, insSteelDrum, insOrchHit, insTimpani, insKick, insSnare, insClosedHat, insOpenHat, insCrash, insTom,
    insClap, insRim, insSquareLead, insSawLead, insSinePad, numInstruments
};

const char* instrumentName (int i);
void addInstruments (chips::Bank& bank);   // appends numInstruments samples
} // namespace bitera
