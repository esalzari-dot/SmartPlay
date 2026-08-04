#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "smartchord/ArpeggiatorEngine.h"

using namespace smartchord;

namespace
{
    bool hasNoteOn (const std::vector<NoteEvent>& events, int midiNote, double beatPosition, int velocity)
    {
        for (const auto& e : events)
            if (e.kind == NoteEvent::Kind::NoteOn && e.midiNote == midiNote
                && e.velocity == velocity && e.beatPosition == Catch::Approx (beatPosition))
                return true;
        return false;
    }

    bool hasNoteOff (const std::vector<NoteEvent>& events, int midiNote, double beatPosition)
    {
        for (const auto& e : events)
            if (e.kind == NoteEvent::Kind::NoteOff && e.midiNote == midiNote
                && e.beatPosition == Catch::Approx (beatPosition))
                return true;
        return false;
    }
}

TEST_CASE("generateSequence plays one note per step for a simple 1:1 pattern", "[ArpeggiatorEngine]")
{
    PatternDefinition asPlayed;
    asPlayed.noteOrderSequence = {0, 1, 2};
    asPlayed.rhythmGrid = {1.0f, 1.0f, 1.0f};
    asPlayed.gateLength = {0.95f, 0.95f, 0.95f};

    const VoicingResult voicing{{60, 64, 67}, 2};

    const auto events = generateSequence(asPlayed, voicing);

    REQUIRE(events.size() == 6);
    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOff(events, 60, 0.95));
    CHECK(hasNoteOn(events, 64, 1.0, defaultVelocity));
    CHECK(hasNoteOff(events, 64, 1.95));
    CHECK(hasNoteOn(events, 67, 2.0, defaultVelocity));
    CHECK(hasNoteOff(events, 67, 2.95));
}

TEST_CASE("generateSequence resolves negative and overflowing indices as octave shifts", "[ArpeggiatorEngine]")
{
    PatternDefinition walkingPop;
    walkingPop.noteOrderSequence = {0, 0, 2, -1};
    walkingPop.rhythmGrid = {0.5f, 0.25f, 0.25f, 0.5f};
    walkingPop.gateLength = {0.7f, 0.5f, 0.5f, 0.7f};

    const VoicingResult voicing{{48, 52, 55}, 2};

    const auto events = generateSequence(walkingPop, voicing);

    REQUIRE(events.size() == 8);
    CHECK(hasNoteOn(events, 48, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 48, 0.5, defaultVelocity));
    CHECK(hasNoteOn(events, 55, 0.75, defaultVelocity));
    // indice -1: wrap all'ultima nota del voicing (55), un'ottava sotto
    CHECK(hasNoteOn(events, 43, 1.0, defaultVelocity));
    CHECK(hasNoteOff(events, 43, 1.35));
}

TEST_CASE("generateSequence groups multiple indices into one strummed step using strumOffsetMs", "[ArpeggiatorEngine]")
{
    PatternDefinition padSostenuto;
    padSostenuto.noteOrderSequence = {0, 1, 2, 3};
    padSostenuto.rhythmGrid = {1.0f};
    padSostenuto.gateLength = {1.0f};
    padSostenuto.strumOffsetMs = 4.0f;

    const VoicingResult voicing{{60, 64, 67}, 2};
    const SyncClock clock{120.0, 0.0};

    const auto events = generateSequence(padSostenuto, voicing, clock);

    REQUIRE(events.size() == 8);
    // index 3 sconfina oltre il voicing (size 3): wrap sul root, un'ottava sopra
    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 64, 0.008, defaultVelocity));
    CHECK(hasNoteOn(events, 67, 0.016, defaultVelocity));
    CHECK(hasNoteOn(events, 72, 0.024, defaultVelocity));

    // tutte le note dello strum si chiudono insieme, a fine step
    for (int note : {60, 64, 67, 72})
        CHECK(hasNoteOff(events, note, 1.0));
}

TEST_CASE("generateSequence delays off-beat steps according to the combined swing amount", "[ArpeggiatorEngine]")
{
    PatternDefinition swung;
    swung.noteOrderSequence = {0, 0, 0, 0};
    swung.rhythmGrid = {0.25f, 0.25f, 0.25f, 0.25f};
    swung.swingAmount = 0.5f;

    const VoicingResult voicing{{60}, 0};

    const auto events = generateSequence(swung, voicing);

    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 60, 0.3125, defaultVelocity));
    CHECK(hasNoteOn(events, 60, 0.5, defaultVelocity));
    CHECK(hasNoteOn(events, 60, 0.8125, defaultVelocity));
}

TEST_CASE("generateSequence combines the pattern's swingAmount with the clock's globalSwingAmount", "[ArpeggiatorEngine]")
{
    PatternDefinition pattern;
    pattern.noteOrderSequence = {0, 0};
    pattern.rhythmGrid = {1.0f, 1.0f};
    pattern.swingAmount = 0.2f;

    const VoicingResult voicing{{60}, 0};
    const SyncClock clock{120.0, 0.3};

    const auto events = generateSequence(pattern, voicing, clock);

    // effectiveSwing = min(1, 0.2 + 0.3) = 0.5 -> delay = 0.5 * 1.0 * 0.5 = 0.25
    CHECK(hasNoteOn(events, 60, 1.25, defaultVelocity));
}

TEST_CASE("generateSequence uses velocityCurve when provided and a default velocity otherwise", "[ArpeggiatorEngine]")
{
    PatternDefinition brokenChord;
    brokenChord.noteOrderSequence = {0, 1, 2, 1};
    brokenChord.rhythmGrid = {0.25f, 0.25f, 0.25f, 0.25f};
    brokenChord.gateLength = {0.9f, 0.9f, 0.9f, 0.9f};
    brokenChord.velocityCurve = {100, 75, 85, 75};

    const VoicingResult voicing{{60, 64, 67}, 2};

    const auto events = generateSequence(brokenChord, voicing);

    CHECK(hasNoteOn(events, 60, 0.0, 100));
    CHECK(hasNoteOn(events, 64, 0.25, 75));
    CHECK(hasNoteOn(events, 67, 0.5, 85));
    CHECK(hasNoteOn(events, 64, 0.75, 75));
}

TEST_CASE("generateSequence returns no events when the voicing or the pattern grid is empty", "[ArpeggiatorEngine]")
{
    PatternDefinition pattern;
    pattern.noteOrderSequence = {0};
    pattern.rhythmGrid = {1.0f};

    CHECK(generateSequence(pattern, VoicingResult{}).empty());
    CHECK(generateSequence(PatternDefinition{}, VoicingResult{{60}, 0}).empty());
}

TEST_CASE("ArpeggiatorEngine resolves the pattern selected on the grid and renders it for the given chord", "[ArpeggiatorEngine]")
{
    const auto library = PatternLibrary::fromJsonFile(std::string(SMARTCHORD_DATA_DIR) + "/patterns.json");

    AutoplayGridState grid;
    grid.setIntensity(InstrumentFamily::Piano, 0, 0); // "As Played"

    const ArpeggiatorEngine engine(library, grid);
    const ChordDefinition cMajor{0, ChordQuality::Maj, 0, 0};

    const auto events = engine.renderChordLoop(cMajor, InstrumentFamily::Piano, 0);

    REQUIRE(events.size() == 6);
    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 64, 1.0, defaultVelocity));
    CHECK(hasNoteOn(events, 67, 2.0, defaultVelocity));
}

TEST_CASE("ArpeggiatorEngine renders a non-empty sequence for all 16 reference patterns", "[ArpeggiatorEngine]")
{
    const auto library = PatternLibrary::fromJsonFile(std::string(SMARTCHORD_DATA_DIR) + "/patterns.json");
    const ChordDefinition cMajor{0, ChordQuality::Maj, 0, 0};

    for (const auto& pattern : library.getAllPatterns())
    {
        AutoplayGridState grid;
        grid.setIntensity(pattern.instrumentFamily, 0, pattern.intensityLevel);

        const ArpeggiatorEngine engine(library, grid);
        const auto events = engine.renderChordLoop(cMajor, pattern.instrumentFamily, 0);

        INFO("pattern id: " << pattern.id);
        CHECK_FALSE(events.empty());

        // Ogni step deve produrre esattamente una coppia NoteOn/NoteOff.
        CHECK(events.size() % 2 == 0);
    }
}

TEST_CASE("ArpeggiatorEngine::renderChordLoop returns no events when no pattern matches the cell", "[ArpeggiatorEngine]")
{
    const std::string json = R"({
        "patterns": [
            {
                "id": "piano_only_level_0",
                "displayName": "Only level 0",
                "instrumentFamily": "Piano",
                "intensityLevel": 0,
                "noteOrderSequence": [0],
                "rhythmGrid": [1.0],
                "gateLength": [0.9]
            }
        ]
    })";
    const auto library = PatternLibrary::fromJson(json);

    AutoplayGridState grid;
    grid.setIntensity(InstrumentFamily::Piano, 0, 2); // nessun pattern per Piano/intensita' 2

    const ArpeggiatorEngine engine(library, grid);
    const ChordDefinition cMajor{0, ChordQuality::Maj, 0, 0};

    CHECK(engine.renderChordLoop(cMajor, InstrumentFamily::Piano, 0).empty());
}
