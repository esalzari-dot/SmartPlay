#include "smartchord/ChordBankModule.h"

#include <stdexcept>
#include <string>

namespace smartchord
{

int keyswitchSlotForNote (int midiNoteNumber)
{
    const int slot = midiNoteNumber - keyswitchBaseNote;
    return (slot >= 0 && slot < numChordBankSlots) ? slot : -1;
}

ChordBankModule::ChordBankModule()
{
    slots.fill (ChordDefinition{});
}

void ChordBankModule::validateSlot (int slot)
{
    if (slot < 0 || slot >= numChordBankSlots)
        throw std::out_of_range ("ChordBankModule: slot fuori range [0, " + std::to_string (numChordBankSlots) + ")");
}

const ChordDefinition& ChordBankModule::getChord (int slot) const
{
    validateSlot (slot);
    return slots[static_cast<size_t> (slot)];
}

void ChordBankModule::setChord (int slot, const ChordDefinition& chord)
{
    validateSlot (slot);
    slots[static_cast<size_t> (slot)] = chord;
}

void ChordBankModule::setActiveSlot (int slot)
{
    validateSlot (slot);
    activeSlot = slot;
}

const ChordDefinition& ChordBankModule::getActiveChord() const
{
    return slots[static_cast<size_t> (activeSlot)];
}

} // namespace smartchord
