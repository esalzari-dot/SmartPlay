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

// Note della barra per un accordo (SPEC.md sezione 5.5): i chord tones dell'accordo,
// ripetuti su piu' ottave e filtrati alla tessitura della famiglia data (VoicingEngine
// sezione 4) - non l'intera scala diatonica, per riusare la conoscenza armonica gia'
// presente nel progetto invece di introdurre un concetto di scala parallelo. Ordinate dal
// grave all'acuto: l'indice in questo vettore e' la tacca sulla barra.
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
