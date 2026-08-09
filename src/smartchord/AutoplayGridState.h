#pragma once

#include "smartchord/PatternLibrary.h"
#include "smartchord/VoicingEngine.h"

#include <array>

namespace smartchord
{

// Deve combaciare con numChordBankSlots (ChordBankModule.h): stesso numero di slot, letto
// da due punti di vista diversi (banco accordi vs griglia intensita').
constexpr int numChordSlots = 9;
constexpr int minIntensityLevel = 0;
constexpr int maxIntensityLevel = 3;

// Stato della griglia Autoplay (SPEC.md sezione 5.1): per ogni combinazione
// (famiglia strumentale, slot accordo) memorizza il livello di intensita'
// selezionato (0 = piu' semplice, 3 = piu' complesso).
class AutoplayGridState
{
public:
    // Tutte le celle iniziano a minIntensityLevel.
    AutoplayGridState();

    int getIntensity (InstrumentFamily family, int chordSlot) const;

    // Lancia std::out_of_range se chordSlot non e' in [0, numChordSlots) o
    // intensityLevel non e' in [minIntensityLevel, maxIntensityLevel].
    void setIntensity (InstrumentFamily family, int chordSlot, int intensityLevel);

private:
    static int familyIndex (InstrumentFamily family);
    static void validateChordSlot (int chordSlot);

    static constexpr int numFamilies = 4;
    std::array<std::array<int, numChordSlots>, numFamilies> intensityByChordSlot{};
};

// Risolve il PatternDefinition corrispondente a (family, intensityLevel) nella
// libreria data (SPEC.md sezione 5.1: "resolvePattern(family, intensityLevel)").
// Restituisce nullptr se non trovato.
const PatternDefinition* resolvePattern (const PatternLibrary& library,
                                          InstrumentFamily family,
                                          int intensityLevel);

} // namespace smartchord
