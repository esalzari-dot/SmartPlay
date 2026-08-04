#pragma once

#include "smartchord/ArpeggiatorEngine.h"

#include <map>
#include <vector>

namespace smartchord
{

// Traccia le note MIDI "suonanti" generate da ArpeggiatorEngine e garantisce che non
// restino mai note appese quando l'accordo attivo, il pattern o l'intensita' cambiano
// a meta' esecuzione (SPEC.md sezione 7).
class MidiOutputManager
{
public:
    // Applica un singolo evento (un NoteOn incrementa, un NoteOff decrementa il
    // conteggio della nota). Piu' NoteOn sulla stessa nota prima del relativo NoteOff
    // sono supportati (es. lo stesso indice di voicing ripetuto in step ravvicinati).
    void handleEvent (const NoteEvent& event);

    bool isNoteActive (int midiNote) const;

    // Note attualmente suonanti, in ordine crescente di altezza.
    std::vector<int> getActiveNotes() const;

    // All-notes-off / panic (SPEC.md sezione 7): genera un NoteOff per ciascuna nota
    // attualmente attiva e azzera lo stato. Da chiamare al cambio di accordo, o quando
    // il pattern/l'intensita' cambiano a meta' esecuzione, per evitare note appese.
    // atBeatPosition e' riportato cosi' com'e' sugli eventi generati: il chiamante che
    // conosce la posizione reale nel tempo puo' passarla, altrimenti resta 0.0.
    std::vector<NoteEvent> allNotesOff (double atBeatPosition = 0.0);

private:
    std::map<int, int> activeNoteCounts;
};

} // namespace smartchord
