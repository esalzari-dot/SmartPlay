#include <catch2/catch_test_macros.hpp>

#include "smartchord/VoicingEngine.h"

using namespace smartchord;

TEST_CASE("applyInversion rotates chord tones and transposes them up an octave", "[VoicingEngine]")
{
    const std::vector<int> triad{0, 4, 7};

    CHECK(applyInversion(triad, 0) == std::vector<int>{0, 4, 7});
    CHECK(applyInversion(triad, 1) == std::vector<int>{4, 7, 12});
    CHECK(applyInversion(triad, 2) == std::vector<int>{7, 12, 16});
}

TEST_CASE("applyInversion wraps modulo the tone count", "[VoicingEngine]")
{
    const std::vector<int> triad{0, 4, 7};
    CHECK(applyInversion(triad, 3) == applyInversion(triad, 0));
    CHECK(applyInversion(triad, 4) == applyInversion(triad, 1));
}

TEST_CASE("applyInversion leaves an empty tone list untouched", "[VoicingEngine]")
{
    CHECK(applyInversion({}, 2).empty());
}

TEST_CASE("mapToInstrumentRange pushes notes apart to satisfy minSpacingSemitones", "[VoicingEngine]")
{
    const VoicingProfile profile{InstrumentFamily::Strings, 40, 100, 8, 7, 24, false, VoicingStyle::Spread};

    const auto result = mapToInstrumentRange({0, 1}, 0, 0, profile);

    REQUIRE(result.notes.size() == 2);
    CHECK(result.notes[0] == 60);
    CHECK(result.notes[1] == 73);
}

TEST_CASE("mapToInstrumentRange compresses excessive spacing to respect maxSpacingSemitones", "[VoicingEngine]")
{
    const VoicingProfile profile{InstrumentFamily::Guitar, 40, 100, 8, 1, 12, false, VoicingStyle::Block};

    const auto result = mapToInstrumentRange({0, 30}, 0, 0, profile);

    REQUIRE(result.notes.size() == 2);
    CHECK(result.notes[0] == 60);
    CHECK(result.notes[1] == 66);
}

TEST_CASE("voiceChord generates the expected notes per instrument family for a C major triad", "[VoicingEngine]")
{
    const ChordDefinition cMajor{0, ChordQuality::Maj, 0, 0};

    SECTION("Piano keeps the full block chord")
    {
        const auto result = voiceChord(cMajor, getVoicingProfile(InstrumentFamily::Piano));
        CHECK(result.notes == std::vector<int>{60, 64, 67});
        CHECK(result.topNoteIndex == 2);
    }

    SECTION("Bass reduces to the root, shifted into its low range")
    {
        const auto result = voiceChord(cMajor, getVoicingProfile(InstrumentFamily::Bass));
        CHECK(result.notes == std::vector<int>{48});
        CHECK(result.topNoteIndex == 0);
    }

    SECTION("Guitar keeps the block chord within its playable range")
    {
        const auto result = voiceChord(cMajor, getVoicingProfile(InstrumentFamily::Guitar));
        CHECK(result.notes == std::vector<int>{60, 64, 67});
        CHECK(result.topNoteIndex == 2);
    }

    SECTION("Strings spread the chord across octaves")
    {
        const auto result = voiceChord(cMajor, getVoicingProfile(InstrumentFamily::Strings));
        CHECK(result.notes == std::vector<int>{60, 76, 91});
        CHECK(result.topNoteIndex == 2);
    }
}

TEST_CASE("voiceChord keeps notes within the instrument range regardless of octaveOffset", "[VoicingEngine]")
{
    const ChordDefinition highBassChord{0, ChordQuality::Min, 0, 3};

    const auto result = voiceChord(highBassChord, getVoicingProfile(InstrumentFamily::Bass));

    REQUIRE(result.notes.size() == 1);
    CHECK(result.notes[0] >= 28);
    CHECK(result.notes[0] <= 55);
}
