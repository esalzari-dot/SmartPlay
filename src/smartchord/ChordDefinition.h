#pragma once

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
