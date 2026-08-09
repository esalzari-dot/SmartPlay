#pragma once

namespace smartchord
{

// Cio' che il plugin sa del trasporto dell'host all'inizio di un blocco audio.
struct TransportState
{
    double bpm = 120.0;
    double hostPpq = 0.0;        // posizione in beat, valida solo se hostProvidesPpq
    bool hostProvidesPpq = false;
    bool isPlaying = true;
};

// Decide, blocco per blocco, se l'arpeggiatore deve suonare e a che punto del loop si
// trova. Vive nel core e non dipende da JUCE, cosi' la logica di temporizzazione del
// plugin - che altrimenti sarebbe sepolta in processBlock() e verificabile solo dentro una
// DAW - resta testabile in isolamento.
//
// Due sorgenti di tempo: la posizione PPQ dell'host mentre il trasporto gira, e un clock
// interno quando e' fermo (usato solo in free run). Le due non hanno alcuna relazione fra
// loro, quindi al passaggio dall'una all'altra la fase del loop va ri-ancorata. La regola:
//
//  - free run attivo: si conserva la continuita' di fase, perche' il pattern e' gia'
//    udibile e un salto in corrispondenza del play sarebbe sgradevole;
//  - free run spento: alla partenza del trasporto il loop si aggancia alla griglia
//    dell'host (fase zero a PPQ zero). E' la scelta che rende la resa ripetibile: la
//    stessa battuta produce sempre lo stesso MIDI, take dopo take.
//
// In entrambi i casi un restartLoop() esplicito (cambio di accordo o di intensita') ha la
// precedenza e riporta il loop a zero sulla posizione corrente.
class LoopClock
{
public:
    struct Frame
    {
        bool shouldPlay = false;
        bool usingHostClock = false;
        double loopPosition = 0.0; // posizione nel loop, al netto dell'ancoraggio di fase
    };

    // Da chiamare una volta per blocco audio, anche quando il trasporto e' fermo: il clock
    // interno deve avanzare comunque. blockLengthBeats e' la durata del blocco.
    Frame advance (const TransportState& transport, bool freeRunWhenStopped, double blockLengthBeats);

    // Chiede che il loop riparta da zero: l'ancoraggio avviene al prossimo advance(), che
    // e' l'unico momento in cui la posizione corrente e' nota. Da usare al cambio di
    // accordo o di intensita', quando il pattern deve rispondere subito invece di entrare
    // a meta'.
    void restartLoop();

    // Riporta il clock allo stato iniziale (prepareToPlay).
    void reset();

private:
    double internalPosition = 0.0;
    double phaseAnchor = 0.0;
    double nextExpectedLoopPosition = 0.0;
    bool wasUsingHostClock = false;
    bool havePreviousFrame = false;
    bool restartPending = false;
};

} // namespace smartchord
