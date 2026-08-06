#pragma once

#include "smartchord/VoicingEngine.h"

#include <limits>
#include <string>
#include <vector>

namespace smartchord
{

// Valore di noteOrderSequence che indica una pausa: lo step consuma il suo tempo senza
// suonare nulla. Nel JSON si scrive "null" fra gli indici, es. [0, null, 2, null].
constexpr int restNoteIndex = std::numeric_limits<int>::min();

// Ordine con cui le note di uno stesso step vengono distanziate da strumOffsetMs.
enum class StrumDirection
{
    Up,        // dal grave all'acuto (pennata in giu' su chitarra)
    Down,      // dall'acuto al grave (pennata in su')
    Alternate  // alterna a ogni step: e' cio' che rende una strimpellata credibile
};

struct PatternDefinition
{
    std::string id;
    std::string displayName;
    InstrumentFamily instrumentFamily = InstrumentFamily::Piano;
    int intensityLevel = 0;              // 0 (piu' semplice) - 3 (piu' complesso)

    std::vector<int> noteOrderSequence;  // indice della nota nel voicing (0=root, 1=terza, 2=quinta...)
                                          // negativo = ottava sotto, oltre size = ottava sopra
    std::vector<float> rhythmGrid;       // durata step in frazioni di beat (0.25 = 1/16)
    std::vector<float> gateLength;       // % della durata effettivamente suonata (staccato/legato)
    std::vector<int> velocityCurve;      // velocity MIDI per step (accenti)

    int octaveSpread = 0;
    int loopLength = 0;
    float humanizeTiming = 0.0f;         // ms variazione random
    int humanizeVelocity = 0;
    float swingAmount = 0.0f;            // 0-1
    float strumOffsetMs = 0.0f;          // opzionale, solo chitarra
    StrumDirection strumDirection = StrumDirection::Up;
    bool crescendoCurve = false;         // opzionale, solo archi
};

class PatternLibrary
{
public:
    // Analizza le pattern definitions da una stringa JSON (SPEC.md sezione 5.2/5.3).
    // Lancia std::runtime_error se il JSON non e' valido.
    static PatternLibrary fromJson (const std::string& json);

    // Legge un file JSON da disco e lo analizza tramite fromJson().
    static PatternLibrary fromJsonFile (const std::string& path);

    const std::vector<PatternDefinition>& getAllPatterns() const noexcept { return patterns; }

    // Restituisce il pattern per (famiglia, intensita'), oppure nullptr se non trovato.
    const PatternDefinition* findPattern (InstrumentFamily family, int intensityLevel) const;

private:
    std::vector<PatternDefinition> patterns;
};

} // namespace smartchord
