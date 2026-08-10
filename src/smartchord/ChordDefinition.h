#pragma once

#include <string>
#include <vector>

namespace smartchord
{

enum class ChordQuality
{
    Maj,
    Min,
    Dim,
    Aug,
    Sus2,
    Sus4,
    Maj7,
    Min7,
    Dom7,
    Min7b5,
    Dim7,
    Add9,
    Six,
    Nine
};

// Intervalli in semitoni dalla root, per ChordQuality (SPEC.md sezione 3).
std::vector<int> getChordTones (ChordQuality quality);

// Scala implicita di 7 gradi che contiene i chord tones della qualita' data (per gli
// accordi diminuiti di settima, simmetrici, li approssima: non esiste una scala diatonica
// di 7 gradi che li contenga esattamente - vedi ChordDefinition.cpp). Usata da
// PlayStripEngine (SPEC.md sezione 5.5) per popolare le tacche della barra con note della
// scala dell'accordo, non solo i suoi toni; non sa nulla della tonalita' del brano, e' solo
// un ventaglio di note musicalmente coerenti con l'accordo attivo preso da solo.
std::vector<int> getChordScaleTones (ChordQuality quality);

// Nome stabile per ChordQuality, usato per serializzare (es. ChordBankPresets) - non e'
// pensato per la UI (ChordLabel ha le sue abbreviazioni), ma per un round-trip esatto.
const char* toString (ChordQuality quality);

// Lancia std::runtime_error se name non corrisponde a nessuna qualita'.
ChordQuality chordQualityFromString (const std::string& name);

struct ChordDefinition
{
    int rootSemitone = 0;                     // 0-11, C=0
    ChordQuality quality = ChordQuality::Maj;
    int inversion = 0;                        // 0 = fondamentale, 1 = primo rivolto, ecc.
    int octaveOffset = 0;                     // ottava base del voicing
};

inline bool operator== (const ChordDefinition& a, const ChordDefinition& b)
{
    return a.rootSemitone == b.rootSemitone
        && a.quality == b.quality
        && a.inversion == b.inversion
        && a.octaveOffset == b.octaveOffset;
}

inline bool operator!= (const ChordDefinition& a, const ChordDefinition& b) { return ! (a == b); }

} // namespace smartchord
