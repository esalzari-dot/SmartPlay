#pragma once

#include "smartchord/ChordDefinition.h"

#include <vector>

namespace smartchord
{

enum class InstrumentFamily
{
    Piano,
    Bass,
    Guitar,
    Strings
};

enum class VoicingStyle
{
    Block,
    Spread,
    Monophonic
};

struct VoicingProfile
{
    InstrumentFamily instrumentFamily;
    int midiRangeLow;
    int midiRangeHigh;
    int maxNotes;              // quante note dell'accordo mantenere
    int minSpacingSemitones;   // spaziatura minima tra note adiacenti
    int maxSpacingSemitones;   // spaziatura massima (0 = nessun vincolo)
    bool allowDoubling;        // raddoppio di root/ottava per riempire maxNotes
    VoicingStyle voicingStyle;
};

struct VoicingResult
{
    std::vector<int> notes;   // note MIDI assolute, ordinate dal grave all'acuto
    int topNoteIndex = -1;    // indice della nota piu' acuta in notes
};

// Restituisce il VoicingProfile di riferimento (SPEC.md sezione 4) per la famiglia data.
VoicingProfile getVoicingProfile (InstrumentFamily family);

// Riordina/trasla gli intervalli di un accordo secondo l'inversione richiesta.
// Un ciclo completo (inversion == chordTones.size()) restituisce gli stessi
// intervalli traslati di un'ottava.
std::vector<int> applyInversion (const std::vector<int>& chordTones, int inversion);

// Applica i vincoli di un VoicingProfile a un set di intervalli gia' invertiti,
// producendo note MIDI assolute ordinate ed entro il range dello strumento.
VoicingResult mapToInstrumentRange (const std::vector<int>& invertedTones,
                                     int rootSemitone,
                                     int octaveOffset,
                                     const VoicingProfile& profile);

// Pipeline completa: getChordTones -> applyInversion -> mapToInstrumentRange.
VoicingResult voiceChord (const ChordDefinition& chord, const VoicingProfile& profile);

} // namespace smartchord
