#include <catch2/catch_test_macros.hpp>

#include "smartchord/ChordDefinition.h"

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
