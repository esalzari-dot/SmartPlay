#pragma once

#include "smartchord/ChordDefinition.h"

#include <array>

namespace smartchord
{

constexpr int numChordBankSlots = 8;

// Banco di accordi (SPEC.md sezione 3): minimo 8 slot, selezionabili in tempo reale.
// Ogni slot e' un ChordDefinition; uno slot alla volta e' "attivo" (quello che
// ArpeggiatorEngine deve eseguire).
class ChordBankModule
{
public:
    // Tutti gli slot iniziano a ChordDefinition di default (C maggiore, fondamentale);
    // lo slot attivo iniziale e' 0.
    ChordBankModule();

    const ChordDefinition& getChord (int slot) const;
    void setChord (int slot, const ChordDefinition& chord);

    int getActiveSlot() const noexcept { return activeSlot; }
    void setActiveSlot (int slot);

    const ChordDefinition& getActiveChord() const;

private:
    static void validateSlot (int slot);

    std::array<ChordDefinition, numChordBankSlots> slots{};
    int activeSlot = 0;
};

} // namespace smartchord
