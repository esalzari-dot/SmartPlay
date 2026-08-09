#include "smartchord/ChordRecognizer.h"

#include <algorithm>
#include <array>

namespace smartchord
{

namespace
{
    constexpr std::array<ChordQuality, 14> allQualities {
        ChordQuality::Maj, ChordQuality::Min, ChordQuality::Dim, ChordQuality::Aug,
        ChordQuality::Sus2, ChordQuality::Sus4, ChordQuality::Maj7, ChordQuality::Min7,
        ChordQuality::Dom7, ChordQuality::Min7b5, ChordQuality::Dim7, ChordQuality::Add9,
        ChordQuality::Six, ChordQuality::Nine
    };

    using PitchClassSet = unsigned int; // un bit per classe di altezza, 0 = C

    PitchClassSet pitchClassSetOf (const std::vector<int>& notes)
    {
        PitchClassSet set = 0;
        for (int note : notes)
            set |= 1u << (((note % 12) + 12) % 12);
        return set;
    }

    PitchClassSet chordPitchClassSet (int rootSemitone, ChordQuality quality)
    {
        PitchClassSet set = 0;
        for (int interval : getChordTones (quality))
            set |= 1u << (((rootSemitone + interval) % 12 + 12) % 12);
        return set;
    }

    int countBits (PitchClassSet set)
    {
        int count = 0;
        for (; set != 0; set >>= 1)
            count += static_cast<int> (set & 1u);
        return count;
    }

    // Quante volte il basso deve pesare rispetto a una nota qualsiasi. Con due accordi
    // ugualmente compatibili (Do maggiore e La minore settima condividono tre note su
    // quattro) e' la nota piu' grave a dire quale dei due si sta suonando.
    constexpr int bassBonus = 2;

    int inversionForBass (int rootSemitone, ChordQuality quality, int bassPitchClass)
    {
        const auto tones = getChordTones (quality);
        for (size_t i = 0; i < tones.size(); ++i)
            if (((rootSemitone + tones[i]) % 12 + 12) % 12 == bassPitchClass)
                return static_cast<int> (i);

        return 0; // basso estraneo all'accordo: si resta in posizione fondamentale
    }
}

std::optional<ChordDefinition> recognizeChord (const std::vector<int>& heldNotes)
{
    const auto played = pitchClassSetOf (heldNotes);
    if (static_cast<size_t> (countBits (played)) < minimumNotesForRecognition)
        return std::nullopt;

    const int bassPitchClass = ((*std::min_element (heldNotes.begin(), heldNotes.end()) % 12) + 12) % 12;

    std::optional<ChordDefinition> best;
    int bestScore = 0;

    for (int root = 0; root < 12; ++root)
    {
        for (ChordQuality quality : allQualities)
        {
            const auto candidate = chordPitchClassSet (root, quality);

            const int matched = countBits (candidate & played);
            const int missing = countBits (candidate & ~played);   // note dell'accordo non suonate
            const int extra = countBits (played & ~candidate);     // note suonate estranee all'accordo

            // Un accordo e' tanto piu' probabile quanto piu' copre cio' che si sta
            // suonando senza aggiungere note assenti; la fondamentale al basso conferma.
            int score = matched * 2 - missing - extra * 2;
            if (root == bassPitchClass)
                score += bassBonus;

            if (score > bestScore)
            {
                bestScore = score;
                best = ChordDefinition { root, quality, inversionForBass (root, quality, bassPitchClass), 0 };
            }
        }
    }

    return best;
}

} // namespace smartchord
