#include <catch2/catch_test_macros.hpp>

#include "smartchord/ChordDefinition.h"

#include <algorithm>

using namespace smartchord;

TEST_CASE("getChordTones returns correct intervals for triads", "[ChordDefinition]")
{
    CHECK(getChordTones(ChordQuality::Maj) == std::vector<int>{0, 4, 7});
    CHECK(getChordTones(ChordQuality::Min) == std::vector<int>{0, 3, 7});
    CHECK(getChordTones(ChordQuality::Dim) == std::vector<int>{0, 3, 6});
    CHECK(getChordTones(ChordQuality::Aug) == std::vector<int>{0, 4, 8});
    CHECK(getChordTones(ChordQuality::Sus2) == std::vector<int>{0, 2, 7});
    CHECK(getChordTones(ChordQuality::Sus4) == std::vector<int>{0, 5, 7});
}

TEST_CASE("getChordTones returns correct intervals for seventh chords", "[ChordDefinition]")
{
    CHECK(getChordTones(ChordQuality::Maj7) == std::vector<int>{0, 4, 7, 11});
    CHECK(getChordTones(ChordQuality::Min7) == std::vector<int>{0, 3, 7, 10});
    CHECK(getChordTones(ChordQuality::Dom7) == std::vector<int>{0, 4, 7, 10});
    CHECK(getChordTones(ChordQuality::Min7b5) == std::vector<int>{0, 3, 6, 10});
    CHECK(getChordTones(ChordQuality::Dim7) == std::vector<int>{0, 3, 6, 9});
}

TEST_CASE("getChordTones returns correct intervals for extended chords", "[ChordDefinition]")
{
    CHECK(getChordTones(ChordQuality::Add9) == std::vector<int>{0, 4, 7, 14});
    CHECK(getChordTones(ChordQuality::Six) == std::vector<int>{0, 4, 7, 9});
    CHECK(getChordTones(ChordQuality::Nine) == std::vector<int>{0, 4, 7, 10, 14});
}

TEST_CASE("ChordDefinition default state is C major root position", "[ChordDefinition]")
{
    ChordDefinition chord;
    CHECK(chord.rootSemitone == 0);
    CHECK(chord.quality == ChordQuality::Maj);
    CHECK(chord.inversion == 0);
    CHECK(chord.octaveOffset == 0);
}

TEST_CASE("toString/chordQualityFromString fanno un round-trip per ogni qualita'", "[ChordDefinition]")
{
    const ChordQuality allQualities[] = {
        ChordQuality::Maj, ChordQuality::Min, ChordQuality::Dim, ChordQuality::Aug,
        ChordQuality::Sus2, ChordQuality::Sus4, ChordQuality::Maj7, ChordQuality::Min7,
        ChordQuality::Dom7, ChordQuality::Min7b5, ChordQuality::Dim7, ChordQuality::Add9,
        ChordQuality::Six, ChordQuality::Nine
    };

    for (auto quality : allQualities)
    {
        INFO("quality index " << static_cast<int>(quality));
        CHECK(chordQualityFromString(toString(quality)) == quality);
    }
}

TEST_CASE("chordQualityFromString lancia su un nome sconosciuto", "[ChordDefinition]")
{
    CHECK_THROWS_AS(chordQualityFromString("NotAQuality"), std::runtime_error);
}

TEST_CASE("getChordScaleTones e' una scala di 7 gradi che contiene i chord tones della stessa qualita'", "[ChordDefinition]")
{
    const ChordQuality allQualities[] = {
        ChordQuality::Maj, ChordQuality::Min, ChordQuality::Dim, ChordQuality::Aug,
        ChordQuality::Sus2, ChordQuality::Sus4, ChordQuality::Maj7, ChordQuality::Min7,
        ChordQuality::Dom7, ChordQuality::Min7b5, ChordQuality::Dim7, ChordQuality::Add9,
        ChordQuality::Six, ChordQuality::Nine
    };

    for (auto quality : allQualities)
    {
        INFO("quality index " << static_cast<int>(quality));
        const auto scale = getChordScaleTones(quality);
        CHECK(scale.size() == 7);

        for (int tone : getChordTones(quality))
        {
            const int pitchClass = ((tone % 12) + 12) % 12;

            // Dim7 e' l'unica approssimazione documentata (SPEC.md sezione 5.5): un
            // accordo diminuito di settima e' simmetrico e non entra esattamente in una
            // scala diatonica di 7 gradi.
            if (quality == ChordQuality::Dim7 && pitchClass == 9)
                continue;

            INFO("pitch class " << pitchClass);
            CHECK(std::find(scale.begin(), scale.end(), pitchClass) != scale.end());
        }
    }
}
