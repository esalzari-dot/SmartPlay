#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace smartchord::ui
{

// Slot (0-based) selezionato dal tasto premuto, sia sul tastierino numerico sia sulla
// riga di cifre in alto, oppure -1 se il tasto non corrisponde a nessuno slot o eccede
// maxSlots. Permette di suonare gli accordi dalla tastiera del computer quando la
// finestra del plugin ha il focus, senza bisogno di un controller MIDI (SPEC.md sezione
// 3: "selezionabili via UI pad, MIDI note trigger, o keyswitch" - questa e' la quarta via).
int chordSlotForKeyPress (const juce::KeyPress& key, int maxSlots);

} // namespace smartchord::ui
