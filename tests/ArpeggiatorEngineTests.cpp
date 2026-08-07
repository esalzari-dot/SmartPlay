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

TEST_CASE("patternLoopLengthBeats sums the rhythmGrid durations", "[ArpeggiatorEngine]")
{
    PatternDefinition walkingPop;
    walkingPop.rhythmGrid = {0.5f, 0.25f, 0.25f, 0.5f};
    CHECK(patternLoopLengthBeats(walkingPop) == Catch::Approx(1.5));

    CHECK(patternLoopLengthBeats(PatternDefinition{}) == Catch::Approx(0.0));
}

TEST_CASE("scheduleEventsInWindow schedules events that fall within a non-wrapping window", "[ArpeggiatorEngine]")
{
    const std::vector<NoteEvent> loopEvents = {
        {NoteEvent::Kind::NoteOn, 60, defaultVelocity, 0.0},
        {NoteEvent::Kind::NoteOn, 61, defaultVelocity, 1.0},
        {NoteEvent::Kind::NoteOn, 62, defaultVelocity, 2.0},
        {NoteEvent::Kind::NoteOn, 63, defaultVelocity, 3.0},
    };

    const auto scheduled = scheduleEventsInWindow(loopEvents, 4.0, 0.0, 2.0, 100.0, 200);

    REQUIRE(scheduled.size() == 2);
    CHECK(scheduled[0].event.midiNote == 60);
    CHECK(scheduled[0].sampleOffset == 0);
    CHECK(scheduled[1].event.midiNote == 61);
    CHECK(scheduled[1].sampleOffset == 100);
}

TEST_CASE("scheduleEventsInWindow wraps around the end of the loop", "[ArpeggiatorEngine]")
{
    const std::vector<NoteEvent> loopEvents = {
        {NoteEvent::Kind::NoteOn, 60, defaultVelocity, 0.0},
        {NoteEvent::Kind::NoteOn, 61, defaultVelocity, 1.0},
        {NoteEvent::Kind::NoteOn, 62, defaultVelocity, 2.0},
        {NoteEvent::Kind::NoteOn, 63, defaultVelocity, 3.0},
    };

    // Finestra [3, 5) su un loop di 4 beat: copre beat 3 (fine giro) e beat 0 (giro successivo).
    const auto scheduled = scheduleEventsInWindow(loopEvents, 4.0, 3.0, 2.0, 100.0, 200);

    REQUIRE(scheduled.size() == 2);
    CHECK(scheduled[0].event.midiNote == 63);
    CHECK(scheduled[0].sampleOffset == 0);
    CHECK(scheduled[1].event.midiNote == 60);
    CHECK(scheduled[1].sampleOffset == 100);
}

TEST_CASE("scheduleEventsInWindow reduces a windowStartBeat far beyond the loop length modulo loopLengthBeats", "[ArpeggiatorEngine]")
{
    const std::vector<NoteEvent> loopEvents = {
        {NoteEvent::Kind::NoteOn, 60, defaultVelocity, 0.0},
        {NoteEvent::Kind::NoteOn, 61, defaultVelocity, 1.0},
    };

    const auto atZero = scheduleEventsInWindow(loopEvents, 4.0, 0.0, 2.0, 100.0, 200);
    const auto farBeyond = scheduleEventsInWindow(loopEvents, 4.0, 8.0, 2.0, 100.0, 200); // 8 mod 4 == 0

    REQUIRE(atZero.size() == farBeyond.size());
    for (size_t i = 0; i < atZero.size(); ++i)
    {
        CHECK(atZero[i].event.midiNote == farBeyond[i].event.midiNote);
        CHECK(atZero[i].sampleOffset == farBeyond[i].sampleOffset);
    }
}

TEST_CASE("scheduleEventsInWindow handles a window spanning multiple loop repetitions", "[ArpeggiatorEngine]")
{
    const std::vector<NoteEvent> loopEvents = {
        {NoteEvent::Kind::NoteOn, 60, defaultVelocity, 0.0},
    };

    // Loop di 0.5 beat, finestra di 2 beat: il loop si ripete 4 volte nello stesso blocco.
    const auto scheduled = scheduleEventsInWindow(loopEvents, 0.5, 0.0, 2.0, 10.0, 20);

    REQUIRE(scheduled.size() == 4);
    CHECK(scheduled[0].sampleOffset == 0);
    CHECK(scheduled[1].sampleOffset == 5);
    CHECK(scheduled[2].sampleOffset == 10);
    CHECK(scheduled[3].sampleOffset == 15);
}

TEST_CASE("scheduleEventsInWindow clamps a sample offset that rounds past the end of the block", "[ArpeggiatorEngine]")
{
    const std::vector<NoteEvent> loopEvents = {
        {NoteEvent::Kind::NoteOn, 60, defaultVelocity, 0.999},
    };

    const auto scheduled = scheduleEventsInWindow(loopEvents, 1.0, 0.0, 1.0, 100.0, 100);

    REQUIRE(scheduled.size() == 1);
    CHECK(scheduled[0].sampleOffset == 99); // blockNumSamples - 1
}

TEST_CASE("scheduleEventsInWindow returns no events for invalid or empty inputs", "[ArpeggiatorEngine]")
{
    const std::vector<NoteEvent> loopEvents = {
        {NoteEvent::Kind::NoteOn, 60, defaultVelocity, 0.0},
    };

    CHECK(scheduleEventsInWindow({}, 4.0, 0.0, 2.0, 100.0, 200).empty());
    CHECK(scheduleEventsInWindow(loopEvents, 0.0, 0.0, 2.0, 100.0, 200).empty());
    CHECK(scheduleEventsInWindow(loopEvents, 4.0, 0.0, 0.0, 100.0, 200).empty());
}

TEST_CASE("generateSequence consumes a rest's time without emitting events", "[ArpeggiatorEngine]")
{
    PatternDefinition withRest;
    withRest.noteOrderSequence = {0, restNoteIndex, 2, restNoteIndex};
    withRest.rhythmGrid = {0.25f, 0.25f, 0.25f, 0.25f};
    withRest.gateLength = {0.9f, 0.9f, 0.9f, 0.9f};

    const VoicingResult voicing{{60, 64, 67}, 2};
    const auto events = generateSequence(withRest, voicing);

    // Due note suonate su quattro step: una coppia NoteOn/NoteOff ciascuna.
    REQUIRE(events.size() == 4);
    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 67, 0.5, defaultVelocity)); // il tempo delle pause e' comunque scorso

    // Il loop dura comunque quattro step.
    CHECK(patternLoopLengthBeats(withRest) == Catch::Approx(1.0));
}

TEST_CASE("generateSequence reverses the strum offsets for a downward strum", "[ArpeggiatorEngine]")
{
    PatternDefinition strum;
    strum.noteOrderSequence = {0, 1, 2};
    strum.rhythmGrid = {1.0f};
    strum.gateLength = {1.0f};
    strum.strumOffsetMs = 8.0f;
    strum.strumDirection = StrumDirection::Down;

    const VoicingResult voicing{{60, 64, 67}, 2};
    const auto events = generateSequence(strum, voicing, SyncClock{120.0, 0.0});

    // Verso il basso: la nota piu' acuta parte per prima, la fondamentale per ultima.
    CHECK(hasNoteOn(events, 67, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 64, 0.016, defaultVelocity));
    CHECK(hasNoteOn(events, 60, 0.032, defaultVelocity));
}

TEST_CASE("generateSequence alternates the strum direction between steps", "[ArpeggiatorEngine]")
{
    PatternDefinition strum;
    strum.noteOrderSequence = {0, 1, 0, 1};
    strum.rhythmGrid = {1.0f, 1.0f};
    strum.gateLength = {1.0f, 1.0f};
    strum.strumOffsetMs = 8.0f;
    strum.strumDirection = StrumDirection::Alternate;

    const VoicingResult voicing{{60, 64}, 1};
    const auto events = generateSequence(strum, voicing, SyncClock{120.0, 0.0});

    // Primo step in su: 60 per prima. Secondo step in giu': 64 per prima.
    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 64, 0.016, defaultVelocity));
    CHECK(hasNoteOn(events, 64, 1.0, defaultVelocity));
    CHECK(hasNoteOn(events, 60, 1.016, defaultVelocity));
}

TEST_CASE("generateSequence applies humanization only when given a generator", "[ArpeggiatorEngine]")
{
    PatternDefinition humanized;
    humanized.noteOrderSequence = {0, 0, 0, 0};
    humanized.rhythmGrid = {0.25f, 0.25f, 0.25f, 0.25f};
    humanized.gateLength = {0.5f, 0.5f, 0.5f, 0.5f};
    humanized.humanizeTiming = 5.0f;
    humanized.humanizeVelocity = 10;

    const VoicingResult voicing{{60}, 0};

    // Senza generatore resta deterministica: tutte le velocity al default, tempi esatti.
    const auto plain = generateSequence(humanized, voicing);
    for (const auto& e : plain)
        if (e.kind == NoteEvent::Kind::NoteOn)
            CHECK(e.velocity == defaultVelocity);
    CHECK(hasNoteOn(plain, 60, 0.25, defaultVelocity));

    // Con un seme fisso il risultato e' riproducibile ma non piu' uguale al nominale.
    std::mt19937 rngA{12345}, rngB{12345};
    const auto humanA = generateSequence(humanized, voicing, SyncClock{}, &rngA);
    const auto humanB = generateSequence(humanized, voicing, SyncClock{}, &rngB);

    REQUIRE(humanA.size() == humanB.size());
    bool anyDifference = false;
    for (size_t i = 0; i < humanA.size(); ++i)
    {
        CHECK(humanA[i].velocity == humanB[i].velocity);
        CHECK(humanA[i].beatPosition == Catch::Approx(humanB[i].beatPosition));
        if (humanA[i].velocity != defaultVelocity)
            anyDifference = true;
    }
    CHECK(anyDifference);
}
