#pragma once

#include "smartchord/AutoplayGridState.h"
#include "smartchord/PatternLibrary.h"
#include "smartchord/VoicingEngine.h"

#include <random>
#include <vector>

namespace smartchord
{

// Moltiplicatore globale sulla durata degli step: riusa lo stesso pattern a velocita'
// diverse senza doverne scrivere una variante per ciascuna. Il valore moltiplica
// rhythmGrid, quindi un moltiplicatore piu' grande allunga gli step (pattern piu' lento).
enum class PatternRate
{
    Half,     // meta' velocita'
    Normal,
    Triplet,  // terzine: tre note nello spazio di due
    Double    // doppia velocita'
};

double rateMultiplierFor (PatternRate rate);

// Aggancio a SyncClock (SPEC.md sezione 6): informazioni minime di cui
// ArpeggiatorEngine ha bisogno per generare una sequenza. Il legame vero e
// proprio con l'AudioPlayHead dell'host arriva nell'integrazione JUCE
// (SPEC.md sezione 10, step 8); qui SyncClock e' un semplice value type,
// cosi' il motore resta testabile in isolamento.
struct SyncClock
{
    double bpm = 120.0;
    double globalSwingAmount = 0.0;   // 0-1, si combina con lo swingAmount del pattern
    double rateMultiplier = 1.0;      // vedi PatternRate/rateMultiplierFor
};

struct NoteEvent
{
    enum class Kind { NoteOn, NoteOff, ControlChange };

    Kind kind = Kind::NoteOn;

    // Per NoteOn/NoteOff e' il numero di nota MIDI; per ControlChange e' il numero di
    // controller (l'arpeggiatore usa solo expressionController).
    int midiNote = 0;

    // Per NoteOn e' la velocity, per ControlChange il valore del controller; ignorato
    // sui NoteOff.
    int velocity = 0;

    double beatPosition = 0.0; // posizione dell'evento in beat, relativa all'inizio del loop di pattern
};

constexpr int defaultVelocity = 100;

// CC11 (expression): e' il controller con cui i pattern marcati crescendoCurve fanno
// gonfiare il suono anche quando le note sono tenute e la velocity non basta piu'.
constexpr int expressionController = 11;

// Valore di partenza del crescendo, sia sul CC sia come fattore di scala sulle velocity
// (il valore di arrivo e' 127 / fattore 1.0).
constexpr int crescendoStartValue = 40;

// Effetto di uno step marcato palmMute: la nota dura una frazione del gate normale ed e'
// suonata piu' piano. Sono i due tratti che rendono riconoscibile il palm mute.
constexpr float palmMuteGateScale = 0.35f;
constexpr float palmMuteVelocityScale = 0.7f;

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
// - Un indice pari a restNoteIndex e' una pausa: lo step consuma il suo tempo senza
//   generare eventi.
// - humanizeTiming/humanizeVelocity vengono applicati solo se si passa un generatore
//   casuale; con rng == nullptr la funzione resta pura e deterministica (e i test
//   possono passarne uno con seme fisso).
// - clock.rateMultiplier scala la durata di ogni step (e con essa gate e swing, che vi
//   sono proporzionali); strumOffsetMs resta in millisecondi e non viene scalato.
// - se pattern.crescendoCurve e' attivo la sequenza gonfia lungo il loop: le velocity
//   vengono scalate e vengono inseriti eventi di CC11 (expression), necessari perche'
//   anche le note tenute - il caso degli archi - seguano la dinamica.
std::vector<NoteEvent> generateSequence (const PatternDefinition& pattern,
                                          const VoicingResult& voicing,
                                          const SyncClock& clock = {},
                                          std::mt19937* rng = nullptr);

// Somma delle durate di rhythmGrid: lunghezza totale, in beat, di un passaggio del
// pattern (utile per sapere quando un loop deve ripartire da capo). rateMultiplier va
// passato uguale a quello di SyncClock, altrimenti il loop non combacia con la sequenza.
double patternLoopLengthBeats (const PatternDefinition& pattern, double rateMultiplier = 1.0);

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
