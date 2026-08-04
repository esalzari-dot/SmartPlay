#include "smartchord/VoicingEngine.h"

#include <algorithm>

namespace smartchord
{

namespace
{
    // MIDI note 60 (C4) e' la nota di riferimento per octaveOffset == 0.
    constexpr int referenceMiddleC = 60;

    int shiftIntoRange (int note, int low, int high)
    {
        while (note < low)
            note += 12;
        while (note > high)
            note -= 12;
        return note;
    }
}

VoicingProfile getVoicingProfile (InstrumentFamily family)
{
    switch (family)
    {
        case InstrumentFamily::Piano:
            return { InstrumentFamily::Piano, 36, 84, 8, 1, 0, false, VoicingStyle::Block };

        case InstrumentFamily::Bass:
            return { InstrumentFamily::Bass, 28, 55, 4, 1, 0, false, VoicingStyle::Monophonic };

        case InstrumentFamily::Guitar:
            return { InstrumentFamily::Guitar, 40, 76, 6, 1, 12, false, VoicingStyle::Block };

        case InstrumentFamily::Strings:
            return { InstrumentFamily::Strings, 36, 96, 6, 7, 24, true, VoicingStyle::Spread };
    }

    return {};
}

std::vector<int> applyInversion (const std::vector<int>& chordTones, int inversion)
{
    if (chordTones.empty())
        return chordTones;

    std::vector<int> result = chordTones;
    const int n = static_cast<int> (result.size());
    const int steps = ((inversion % n) + n) % n;

    for (int i = 0; i < steps; ++i)
    {
        const int lowest = result.front();
        result.erase (result.begin());
        result.push_back (lowest + 12);
    }

    return result;
}

VoicingResult mapToInstrumentRange (const std::vector<int>& invertedTones,
                                     int rootSemitone,
                                     int octaveOffset,
                                     const VoicingProfile& profile)
{
    VoicingResult out;
    if (invertedTones.empty())
        return out;

    const int baseMidiNote = referenceMiddleC + rootSemitone + 12 * octaveOffset;

    std::vector<int> absoluteNotes;
    absoluteNotes.reserve (invertedTones.size());
    for (int tone : invertedTones)
        absoluteNotes.push_back (baseMidiNote + tone);

    std::sort (absoluteNotes.begin(), absoluteNotes.end());

    if (profile.maxNotes > 0 && static_cast<int> (absoluteNotes.size()) > profile.maxNotes)
        absoluteNotes.resize (static_cast<size_t> (profile.maxNotes));

    std::vector<int> voiced;
    voiced.reserve (absoluteNotes.size());

    for (int note : absoluteNotes)
    {
        note = shiftIntoRange (note, profile.midiRangeLow, profile.midiRangeHigh);

        if (! voiced.empty())
        {
            const int previous = voiced.back();

            while (note - previous < profile.minSpacingSemitones && note + 12 <= profile.midiRangeHigh)
                note += 12;

            if (profile.maxSpacingSemitones > 0)
                while (note - previous > profile.maxSpacingSemitones && note - 12 > previous)
                    note -= 12;

            while (note <= previous && note + 12 <= profile.midiRangeHigh)
                note += 12;
        }

        voiced.push_back (note);
    }

    out.notes = voiced;
    out.topNoteIndex = static_cast<int> (voiced.size()) - 1;
    return out;
}

VoicingResult voiceChord (const ChordDefinition& chord, const VoicingProfile& profile)
{
    const auto invertedTones = applyInversion (getChordTones (chord.quality), chord.inversion);
    return mapToInstrumentRange (invertedTones, chord.rootSemitone, chord.octaveOffset, profile);
}

} // namespace smartchord
