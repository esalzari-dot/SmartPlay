#pragma once

#include "smartchord/AutoplayGridState.h"
#include "smartchord/PatternLibrary.h"
#include "smartchord/VoicingEngine.h"

#include <vector>

namespace smartchord
{

// Aggancio a SyncClock (SPEC.md sezione 6): informazioni minime di cui
// ArpeggiatorEngine ha bisogno per generare una sequenza. Il legame vero e
// proprio con l'AudioPlayHead dell'host arriva nell'integrazione JUCE
// (SPEC.md sezione 10, step 8); qui SyncClock e' un semplice value type,
// cosi' il motore resta testabile in isolamento.
struct SyncClock
{
    double bpm = 120.0;
    double globalSwingAmount = 0.0;   // 0-1, si combina con lo swingAmount del pattern
};

struct NoteEvent
{
    enum class Kind { NoteOn, NoteOff };

    Kind kind = Kind::NoteOn;
    int midiNote = 0;
    int velocity = 0;          // usato solo per NoteOn
    double beatPosition = 0.0; // posizione dell'evento in beat, relativa all'inizio del loop di pattern
};

constexpr int defaultVelocity = 100;

// Genera la sequenza di eventi MIDI note on/off per un singolo passaggio del pattern,
// applicato al voicing dato (SPEC.md sezione 5.4).
//
// - noteOrderSequence viene raggruppato in step secondo rhythmGrid: se ci sono piu'
//   indici che step (es. un accordo "strimpellato" di chitarra), gli indici in eccesso
//   suonano nello stesso step, distanziati da strumOffsetMs (convertito in beat tramite
//   clock.bpm).
// - Un indice negativo o >= al numero di note del voicing trasla la nota di un'ottava
//   (sotto o sopra), ciclando con modulo (SPEC.md sezione 5.2).
// - clock.globalSwingAmount si somma a pattern.swingAmount e ritarda gli step in levare.
//
// humanizeTiming/humanizeVelocity fanno parte dello schema di PatternDefinition ma non
// sono applicati qui: restano compito del livello che pianifica i MIDI event reali
// (step 8), per mantenere questa funzione pura e deterministica.
std::vector<NoteEvent> generateSequence (const PatternDefinition& pattern,
                                          const VoicingResult& voicing,
                                          const SyncClock& clock = {});

// Somma delle durate di rhythmGrid: lunghezza totale, in beat, di un passaggio del
// pattern (utile per sapere quando un loop deve ripartire da capo).
double patternLoopLengthBeats (const PatternDefinition& pattern);

struct ScheduledEvent
{
    NoteEvent event;
    int sampleOffset = 0; // offset in campioni rispetto all'inizio del blocco audio
};

// Determina quali eventi del loop cadono nella finestra [windowStartBeat, windowStartBeat +
// windowLengthBeats), gestendo il wrap-around quando la finestra supera loopLengthBeats
// (anche piu' volte, se il loop e' piu' corto della finestra). windowStartBeat e' una
// posizione assoluta (es. la PPQ dell'host): viene ridotta modulo loopLengthBeats
// internamente, quindi puo' essere arbitrariamente grande. Pensata per essere chiamata una
// volta per blocco audio dal wrapper del plugin (SPEC.md sezione 6, aggancio a SyncClock/
// AudioPlayHead), qui come funzione pura e testabile senza dipendenze da JUCE.
std::vector<ScheduledEvent> scheduleEventsInWindow (const std::vector<NoteEvent>& loopEvents,
                                                     double loopLengthBeats,
                                                     double windowStartBeat,
                                                     double windowLengthBeats,
                                                     double samplesPerBeat,
                                                     int blockNumSamples);

// Motore step-based: lega insieme AutoplayGridState, PatternLibrary e VoicingEngine per
// risolvere ed eseguire il pattern attivo per un dato accordo (SPEC.md sezione 5.4).
class ArpeggiatorEngine
{
public:
    ArpeggiatorEngine (const PatternLibrary& patternLibrary, const AutoplayGridState& gridState);

    // Legge l'intensita' per (family, chordSlot) da AutoplayGridState, risolve il
    // PatternDefinition tramite resolvePattern(), applica il voicing di VoicingEngine
    // per l'accordo dato e genera la sequenza di eventi. Restituisce un vettore vuoto
    // se non esiste un pattern per l'intensita' corrente della cella.
    std::vector<NoteEvent> renderChordLoop (const ChordDefinition& chord,
                                             InstrumentFamily family,
                                             int chordSlot,
                                             const SyncClock& clock = {}) const;

private:
    const PatternLibrary& patternLibrary;
    const AutoplayGridState& gridState;
};

} // namespace smartchord
