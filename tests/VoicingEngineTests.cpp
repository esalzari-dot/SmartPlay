#include <catch2/catch_approx.hpp>
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

    SECTION("Bass keeps the full chord (for pattern indexing), shifted into its low range")
    {
        const auto result = voiceChord(cMajor, getVoicingProfile(InstrumentFamily::Bass));
        CHECK(result.notes == std::vector<int>{48, 52, 55});
        CHECK(result.topNoteIndex == 2);
    }

    SECTION("Guitar keeps the block chord within its playable range")
    {
        const auto result = voiceChord(cMajor, getVoicingProfile(InstrumentFamily::Guitar));
        CHECK(result.notes == std::vector<int>{60, 64, 67});
        CHECK(result.topNoteIndex == 2);
    }

    SECTION("Strings spread the chord across octaves and double the voices")
    {
        // Il profilo archi ha allowDoubling: le tre note dell'accordo, spaziate, vengono
        // raddoppiate all'ottava superiore fin dove la tessitura lo consente (91 + 12
        // uscirebbe da midiRangeHigh, quindi quella voce resta singola).
        const auto result = voiceChord(cMajor, getVoicingProfile(InstrumentFamily::Strings));
        CHECK(result.notes == std::vector<int>{60, 72, 76, 88, 91});
        CHECK(result.topNoteIndex == 4);
    }
}

TEST_CASE("voiceChord keeps notes within the instrument range regardless of octaveOffset", "[VoicingEngine]")
{
    const ChordDefinition highBassChord{0, ChordQuality::Min, 0, 3};

    const auto result = voiceChord(highBassChord, getVoicingProfile(InstrumentFamily::Bass));

    REQUIRE(result.notes.size() == 3);
    for (int note : result.notes)
    {
        CHECK(note >= 28);
        CHECK(note <= 55);
    }
}

TEST_CASE("voicingDistance averages the movement of the corresponding voices", "[VoicingEngine]")
{
    CHECK(voicingDistance({60, 64, 67}, {60, 64, 67}) == Catch::Approx(0.0));
    CHECK(voicingDistance({62, 65, 69}, {60, 64, 67}) == Catch::Approx((2 + 1 + 2) / 3.0));

    // Confronta solo tante voci quante ne ha il voicing piu' piccolo.
    CHECK(voicingDistance({60, 64}, {60, 64, 67}) == Catch::Approx(0.0));
    CHECK(voicingDistance({}, {60}) == Catch::Approx(0.0));
}

TEST_CASE("voiceChordWithLeading picks the inversion closest to the previous voicing", "[VoicingEngine]")
{
    const auto profile = getVoicingProfile(InstrumentFamily::Piano);
    const ChordDefinition fMajor{5, ChordQuality::Maj, 0, 0};

    const auto previous = voiceChord({0, ChordQuality::Maj, 0, 0}, profile).notes; // C: 60,64,67
    const auto led = voiceChordWithLeading(fMajor, profile, previous);
    const auto plain = voiceChord(fMajor, profile);

    REQUIRE_FALSE(led.notes.empty());
    CHECK(voicingDistance(led.notes, previous) <= voicingDistance(plain.notes, previous));
}

TEST_CASE("voiceChordWithLeading falls back to the requested inversion without a previous voicing", "[VoicingEngine]")
{
    const auto profile = getVoicingProfile(InstrumentFamily::Piano);
    const ChordDefinition chord{0, ChordQuality::Maj, 1, 0};

    CHECK(voiceChordWithLeading(chord, profile, {}).notes == voiceChord(chord, profile).notes);
}

TEST_CASE("allowDoubling riempie le voci mancanti raddoppiando all'ottava", "[VoicingEngine]")
{
    VoicingProfile profile{InstrumentFamily::Strings, 36, 96, 6, 1, 0, true, VoicingStyle::Block};
    const ChordDefinition cMajor{0, ChordQuality::Maj, 0, 0};

    const auto result = voiceChord(cMajor, profile);

    // 60/64/67 raddoppiate all'ottava sopra, in ordine crescente.
    CHECK(result.notes == std::vector<int>{60, 64, 67, 72, 76, 79});
    CHECK(result.topNoteIndex == 5);
}

TEST_CASE("allowDoubling non supera mai maxNotes", "[VoicingEngine]")
{
    VoicingProfile profile{InstrumentFamily::Strings, 36, 96, 4, 1, 0, true, VoicingStyle::Block};
    const ChordDefinition cMajor{0, ChordQuality::Maj, 0, 0};

    const auto result = voiceChord(cMajor, profile);

    CHECK(result.notes == std::vector<int>{60, 64, 67, 72});
}

TEST_CASE("allowDoubling non esce dalla tessitura dello strumento", "[VoicingEngine]")
{
    // midiRangeHigh troppo basso perche' un raddoppio ci stia: il voicing resta com'e'.
    VoicingProfile profile{InstrumentFamily::Guitar, 60, 70, 6, 1, 0, true, VoicingStyle::Block};
    const ChordDefinition cMajor{0, ChordQuality::Maj, 0, 0};

    const auto result = voiceChord(cMajor, profile);

    for (int note : result.notes)
        CHECK(note <= 70);
}

TEST_CASE("senza allowDoubling il voicing resta alle sole note dell'accordo", "[VoicingEngine]")
{
    VoicingProfile profile{InstrumentFamily::Piano, 36, 96, 6, 1, 0, false, VoicingStyle::Block};
    const ChordDefinition cMajor{0, ChordQuality::Maj, 0, 0};

    CHECK(voiceChord(cMajor, profile).notes == std::vector<int>{60, 64, 67});
}
