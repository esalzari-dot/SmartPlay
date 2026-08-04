#include "smartchord/AutoplayGridState.h"

#include <stdexcept>

namespace smartchord
{

AutoplayGridState::AutoplayGridState()
{
    for (auto& familySlots : intensityByChordSlot)
        familySlots.fill (minIntensityLevel);
}

int AutoplayGridState::familyIndex (InstrumentFamily family)
{
    switch (family)
    {
        case InstrumentFamily::Piano:   return 0;
        case InstrumentFamily::Bass:    return 1;
        case InstrumentFamily::Guitar:  return 2;
        case InstrumentFamily::Strings: return 3;
    }

    throw std::out_of_range ("AutoplayGridState: InstrumentFamily sconosciuta");
}

void AutoplayGridState::validateChordSlot (int chordSlot)
{
    if (chordSlot < 0 || chordSlot >= numChordSlots)
        throw std::out_of_range ("AutoplayGridState: chordSlot fuori range [0, " + std::to_string (numChordSlots) + ")");
}

int AutoplayGridState::getIntensity (InstrumentFamily family, int chordSlot) const
{
    validateChordSlot (chordSlot);
    return intensityByChordSlot[static_cast<size_t> (familyIndex (family))][static_cast<size_t> (chordSlot)];
}

void AutoplayGridState::setIntensity (InstrumentFamily family, int chordSlot, int intensityLevel)
{
    validateChordSlot (chordSlot);

    if (intensityLevel < minIntensityLevel || intensityLevel > maxIntensityLevel)
        throw std::out_of_range ("AutoplayGridState: intensityLevel fuori range ["
            + std::to_string (minIntensityLevel) + ", " + std::to_string (maxIntensityLevel) + "]");

    intensityByChordSlot[static_cast<size_t> (familyIndex (family))][static_cast<size_t> (chordSlot)] = intensityLevel;
}

const PatternDefinition* resolvePattern (const PatternLibrary& library, InstrumentFamily family, int intensityLevel)
{
    return library.findPattern (family, intensityLevel);
}

} // namespace smartchord
