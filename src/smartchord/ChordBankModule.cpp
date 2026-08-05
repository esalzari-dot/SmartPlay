#include "smartchord/ChordBankModule.h"

#include <stdexcept>

namespace smartchord
{

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
