#include "smartchord/ChordProgressions.h"

#include <algorithm>

namespace smartchord
{

namespace
{
    // Gradi della scala maggiore, in semitoni sopra la tonica: I=0, ii=2, iii=4, IV=5,
    // V=7, vi=9, vii=11.
    constexpr int I = 0, ii = 2, iii = 4, IV = 5, V = 7, vi = 9, bVII = 10;

    const std::vector<ChordProgression> progressions {
        { "pop_i_v_vi_iv", "Pop I-V-vi-IV", {
            { I, ChordQuality::Maj }, { V, ChordQuality::Maj },
            { vi, ChordQuality::Min }, { IV, ChordQuality::Maj } } },

        { "doo_wop_i_vi_iv_v", "Doo-wop I-vi-IV-V", {
            { I, ChordQuality::Maj }, { vi, ChordQuality::Min },
            { IV, ChordQuality::Maj }, { V, ChordQuality::Maj } } },

        { "canone_i_v_vi_iii_iv", "Canone I-V-vi-iii-IV-I-IV-V", {
            { I, ChordQuality::Maj }, { V, ChordQuality::Maj },
            { vi, ChordQuality::Min }, { iii, ChordQuality::Min },
            { IV, ChordQuality::Maj }, { I, ChordQuality::Maj },
            { IV, ChordQuality::Maj }, { V, ChordQuality::Maj } } },

        { "jazz_ii_v_i", "Jazz ii-V-I", {
            { ii, ChordQuality::Min7 }, { V, ChordQuality::Dom7 },
            { I, ChordQuality::Maj7 }, { I, ChordQuality::Maj7 } } },

        { "blues_12_bar", "Blues 12 battute", {
            { I, ChordQuality::Dom7 }, { IV, ChordQuality::Dom7 },
            { I, ChordQuality::Dom7 }, { I, ChordQuality::Dom7 },
            { IV, ChordQuality::Dom7 }, { I, ChordQuality::Dom7 },
            { V, ChordQuality::Dom7 }, { IV, ChordQuality::Dom7 } } },

        { "minore_vi_iv_i_v", "Minore vi-IV-I-V", {
            { vi, ChordQuality::Min }, { IV, ChordQuality::Maj },
            { I, ChordQuality::Maj }, { V, ChordQuality::Maj } } },

        { "modale_i_bvii_iv", "Modale I-bVII-IV", {
            { I, ChordQuality::Maj }, { bVII, ChordQuality::Maj },
            { IV, ChordQuality::Maj }, { I, ChordQuality::Maj } } },

        { "ballad_i_iii_vi_iv", "Ballad I-iii-vi-IV", {
            { I, ChordQuality::Maj }, { iii, ChordQuality::Min },
            { vi, ChordQuality::Min }, { IV, ChordQuality::Maj } } },
    };
}

const std::vector<ChordProgression>& getChordProgressions()
{
    return progressions;
}

const ChordProgression* findChordProgression (const std::string& id)
{
    const auto it = std::find_if (progressions.begin(), progressions.end(),
                                   [&id] (const ChordProgression& p) { return p.id == id; });

    return it != progressions.end() ? &(*it) : nullptr;
}

ChordBankModule buildChordBank (const ChordProgression& progression, int tonicSemitone)
{
    ChordBankModule bank;
    if (progression.degrees.empty())
        return bank;

    const int tonic = ((tonicSemitone % 12) + 12) % 12;

    for (int slot = 0; slot < numChordBankSlots; ++slot)
    {
        const auto& degree = progression.degrees[static_cast<size_t> (slot) % progression.degrees.size()];

        ChordDefinition chord;
        chord.rootSemitone = (tonic + degree.semitonesAboveTonic) % 12;
        chord.quality = degree.quality;
        bank.setChord (slot, chord);
    }

    return bank;
}

} // namespace smartchord
