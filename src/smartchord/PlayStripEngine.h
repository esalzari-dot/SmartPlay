#pragma once

#include "smartchord/ArpeggiatorEngine.h"
#include "smartchord/ChordDefinition.h"
#include "smartchord/VoicingEngine.h"

#include <vector>

namespace smartchord
{

enum class StripGesturePhase { Down, Move, Up };

struct StripGesture
{
    StripGesturePhase phase = StripGesturePhase::Down;
    float position = 0.0f;        // 0.0-1.0 lungo la barra
    double timestampSeconds = 0.0;
};

struct StripNoteEvent
{
    enum class Kind { NoteOn, NoteOff };

    Kind kind = Kind::NoteOn;
    int midiNote = 0;
    int velocity = 0;              // ignorato sui NoteOff
    double timestampSeconds = 0.0;
};

// Numero di tacche della barra per famiglia (SPEC.md sezione 5.5): fisso, non dipende da
// quante note dell'accordo entrano nella tessitura - il Piano usa la scala intera (7
// gradi), Chitarra e Basso/Archi replicano il numero di corde dei rispettivi strumenti
// reali (6 e 4).
int notchCountForFamily (InstrumentFamily family);

// Note della barra per un accordo (SPEC.md sezione 5.5): la scala implicita dell'accordo
// (getChordScaleTones), ripetuta su piu' ottave e filtrata alla tessitura della famiglia
// data (VoicingEngine sezione 4), poi campionata a notchCountForFamily(family) valori
// equidistanti dal grave all'acuto - cosi' un basso a 4 tacche copre la stessa estensione
// di un piano a 7, solo con meno fermate. L'indice nel vettore restituito e' la tacca
// sulla barra.
std::vector<int> notesForStrip (const ChordDefinition& chord, InstrumentFamily family);

constexpr int stripMinVelocity = 60;
constexpr int stripMaxVelocity = 120;

// Oltre questa velocita' di attraversamento (tacche al secondo) lo streaming usa gia' la
// velocity massima; sotto scala linearmente verso stripMinVelocity.
constexpr double stripCrossingsPerSecondForMaxVelocity = 8.0;

// Motore della modalita' Play (SPEC.md sezione 5.5): traduce un flusso di StripGesture in
// eventi nota in tempo reale. A differenza di ArpeggiatorEngine non e' agganciato a
// SyncClock e non schedula nulla in anticipo - reagisce gesto per gesto, senza timer
// interno (vedi processGesture). Stateful (tiene traccia della tacca premuta), quindi
// un'istanza per pad/voce attualmente in esecuzione.
class PlayStripEngine
{
public:
    // notes deve essere gia' ordinato dal grave all'acuto (vedi notesForStrip); un vettore
    // vuoto disabilita l'engine (processGesture non produce mai eventi).
    explicit PlayStripEngine (std::vector<int> notesIn = {});

    // Non chiamarla mentre un gesto e' in corso (fra un Down e il suo Up): il prossimo
    // evento userebbe le nuove note con un indice di tacca calcolato sulle vecchie.
    void setNotes (std::vector<int> notesIn);

    // Consuma un evento gesto e restituisce gli eventi nota che ne conseguono (zero, uno o
    // piu': un solo Move che attraversa piu' tacche produce un NoteOff+NoteOn per ciascuna
    // tacca intermedia, non solo per quella finale, cosi' un trascinamento veloce non salta
    // note). Le fasi vanno passate nell'ordine Down, zero o piu' Move, Up; un Move o un Up
    // senza un Down precedente non produce eventi.
    std::vector<StripNoteEvent> processGesture (const StripGesture& gesture);

private:
    std::vector<int> notes;

    bool isDown = false;
    int currentNotchIndex = -1;
    double lastEventTimestampSeconds = 0.0;

    int notchIndexForPosition (float position) const;
    static int velocityForCrossing (double intervalSeconds);
};

} // namespace smartchord
