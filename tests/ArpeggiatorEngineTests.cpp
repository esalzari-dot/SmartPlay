#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "smartchord/ArpeggiatorEngine.h"
#include "smartchord/MidiOutputManager.h"

#include <map>

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

// --- Fedelta' del flusso MIDI: nessuna nota appesa, nessun evento perso ---------------
//
// Cio' che l'host registra e' esattamente il flusso prodotto da scheduleEventsInWindow,
// quindi ogni NoteOff perso qui diventa una nota appesa nel MIDI registrato.

TEST_CASE("scheduleEventsInWindow non perde il NoteOff che cade sulla fine del loop", "[ArpeggiatorEngine]")
{
    // Gate pieno sull'ultimo step: il NoteOff cade esattamente su loopLengthBeats, cioe'
    // sull'inizio del passaggio successivo.
    const std::vector<NoteEvent> loopEvents = {
        {NoteEvent::Kind::NoteOn, 60, defaultVelocity, 3.0},
        {NoteEvent::Kind::NoteOff, 60, 0, 4.0},
    };

    const auto scheduled = scheduleEventsInWindow(loopEvents, 4.0, 3.0, 2.0, 100.0, 200);

    int noteOffs = 0;
    for (const auto& s : scheduled)
        if (s.event.kind == NoteEvent::Kind::NoteOff && s.event.midiNote == 60)
            ++noteOffs;

    CHECK(noteOffs == 1);
}

TEST_CASE("scheduleEventsInWindow non perde un NoteOff spinto oltre la fine del loop", "[ArpeggiatorEngine]")
{
    // Lo swing sull'ultimo step puo' portare il NoteOff oltre la lunghezza del loop.
    const std::vector<NoteEvent> loopEvents = {
        {NoteEvent::Kind::NoteOn, 62, defaultVelocity, 3.5},
        {NoteEvent::Kind::NoteOff, 62, 0, 4.2},
    };

    const auto scheduled = scheduleEventsInWindow(loopEvents, 4.0, 3.0, 2.0, 100.0, 200);

    bool sawNoteOff = false;
    for (const auto& s : scheduled)
        if (s.event.kind == NoteEvent::Kind::NoteOff && s.event.midiNote == 62)
            sawNoteOff = true;

    CHECK(sawNoteOff);
}

TEST_CASE("scheduleEventsInWindow emette il NoteOff prima del NoteOn a parita' di campione", "[ArpeggiatorEngine]")
{
    // Nota ribattuta senza stacco: se il NoteOn arrivasse per primo, il NoteOff che lo
    // segue spegnerebbe la nota appena accesa.
    const std::vector<NoteEvent> loopEvents = {
        {NoteEvent::Kind::NoteOn, 60, defaultVelocity, 0.0},
        {NoteEvent::Kind::NoteOff, 60, 0, 1.0},
        {NoteEvent::Kind::NoteOn, 60, defaultVelocity, 1.0},
        {NoteEvent::Kind::NoteOff, 60, 0, 2.0},
    };

    const auto scheduled = scheduleEventsInWindow(loopEvents, 2.0, 0.9, 0.2, 100.0, 20);

    REQUIRE(scheduled.size() == 2);
    CHECK(scheduled[0].sampleOffset == scheduled[1].sampleOffset);
    CHECK(scheduled[0].event.kind == NoteEvent::Kind::NoteOff);
    CHECK(scheduled[1].event.kind == NoteEvent::Kind::NoteOn);
}

TEST_CASE("un pattern completo si chiude su ogni nota che apre, loop dopo loop", "[ArpeggiatorEngine]")
{
    PatternDefinition fullGate;
    fullGate.noteOrderSequence = {0, 1, 2, 3};
    fullGate.rhythmGrid = {0.25f, 0.25f, 0.25f, 0.25f};
    fullGate.gateLength = {1.0f, 1.0f, 1.0f, 1.0f}; // legato: ogni NoteOff tocca lo step dopo

    const VoicingResult voicing{{60, 64, 67, 72}, 2};
    const auto loopEvents = generateSequence(fullGate, voicing);
    const double loopLength = patternLoopLengthBeats(fullGate);

    // Si riproduce la catena del plugin: lo scheduler propone gli eventi, MidiOutputManager
    // filtra i NoteOff che non chiudono nulla (il NoteOff dell'ultimo step cade sul
    // passaggio successivo, quindi al primo giro non ha corrispondenza).
    MidiOutputManager output;
    std::map<int, int> outstanding;
    const double blockBeats = 0.125;

    for (int block = 0; block < 24; ++block) // tre passaggi interi
    {
        const auto scheduled = scheduleEventsInWindow(loopEvents, loopLength,
                                                      block * blockBeats, blockBeats, 1000.0, 125);

        for (const auto& s : scheduled)
        {
            if (s.event.kind == NoteEvent::Kind::NoteOff && !output.isNoteActive(s.event.midiNote))
                continue;

            output.handleEvent(s.event);

            if (s.event.kind == NoteEvent::Kind::NoteOn)
                ++outstanding[s.event.midiNote];
            else
                --outstanding[s.event.midiNote];

            // Nessuna nota deve mai risultare spenta piu' volte di quante e' stata accesa.
            CHECK(outstanding[s.event.midiNote] >= 0);
        }
    }

    // Ogni nota ha suonato tre volte (tre passaggi) e si e' chiusa, tranne quella
    // dell'ultimo step: il suo NoteOff cade all'inizio del passaggio successivo, che
    // qui non viene reso.
    CHECK(output.getActiveNotes() == std::vector<int>{72});

    int stillSounding = 0;
    for (const auto& entry : outstanding)
        stillSounding += entry.second;

    CHECK(stillSounding == 1);
}

// --- Moltiplicatore globale di velocita' ------------------------------------------

TEST_CASE("rateMultiplierFor mappa i rate sui rispettivi fattori", "[ArpeggiatorEngine]")
{
    CHECK(rateMultiplierFor(PatternRate::Half) == Catch::Approx(2.0));
    CHECK(rateMultiplierFor(PatternRate::Normal) == Catch::Approx(1.0));
    CHECK(rateMultiplierFor(PatternRate::Triplet) == Catch::Approx(2.0 / 3.0));
    CHECK(rateMultiplierFor(PatternRate::Double) == Catch::Approx(0.5));
}

TEST_CASE("il rate globale scala le posizioni degli step e la lunghezza del loop", "[ArpeggiatorEngine]")
{
    PatternDefinition pattern;
    pattern.noteOrderSequence = {0, 1, 2, 0};
    pattern.rhythmGrid = {0.25f, 0.25f, 0.25f, 0.25f};
    pattern.gateLength = {0.5f, 0.5f, 0.5f, 0.5f};

    const VoicingResult voicing{{60, 64, 67}, 2};

    SyncClock doubleSpeed;
    doubleSpeed.rateMultiplier = rateMultiplierFor(PatternRate::Double);

    const auto events = generateSequence(pattern, voicing, doubleSpeed);

    CHECK(patternLoopLengthBeats(pattern, doubleSpeed.rateMultiplier) == Catch::Approx(0.5));
    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 64, 0.125, defaultVelocity));
    CHECK(hasNoteOn(events, 67, 0.25, defaultVelocity));
    CHECK(hasNoteOff(events, 60, 0.0625)); // gate 0.5 di uno step lungo 0.125
}

TEST_CASE("il rate a terzine comprime il loop di un terzo", "[ArpeggiatorEngine]")
{
    PatternDefinition pattern;
    pattern.noteOrderSequence = {0, 1, 2};
    pattern.rhythmGrid = {0.5f, 0.5f, 0.5f};

    CHECK(patternLoopLengthBeats(pattern, rateMultiplierFor(PatternRate::Triplet)) == Catch::Approx(1.0));
}

TEST_CASE("un rate non valido viene ignorato invece di azzerare il pattern", "[ArpeggiatorEngine]")
{
    PatternDefinition pattern;
    pattern.noteOrderSequence = {0, 1};
    pattern.rhythmGrid = {1.0f, 1.0f};

    const VoicingResult voicing{{60, 64}, 1};

    SyncClock broken;
    broken.rateMultiplier = 0.0;

    const auto events = generateSequence(pattern, voicing, broken);

    CHECK(hasNoteOn(events, 64, 1.0, defaultVelocity));
    CHECK(patternLoopLengthBeats(pattern, 0.0) == Catch::Approx(2.0));
}

// --- crescendoCurve ---------------------------------------------------------------

TEST_CASE("crescendoCurve emette una rampa di CC11 lungo tutto il loop", "[ArpeggiatorEngine]")
{
    PatternDefinition swell;
    swell.noteOrderSequence = {0, 1, 2, 3};
    swell.rhythmGrid = {4.0f}; // una sola nota tenuta: solo il CC puo' farla gonfiare
    swell.gateLength = {1.0f};
    swell.crescendoCurve = true;

    const VoicingResult voicing{{60, 64, 67, 72}, 2};
    const auto events = generateSequence(swell, voicing);

    std::vector<NoteEvent> ccEvents;
    for (const auto& e : events)
        if (e.kind == NoteEvent::Kind::ControlChange)
            ccEvents.push_back(e);

    REQUIRE_FALSE(ccEvents.empty());

    for (const auto& e : ccEvents)
    {
        CHECK(e.midiNote == expressionController);
        CHECK(e.velocity >= 0);
        CHECK(e.velocity <= 127);
        CHECK(e.beatPosition >= 0.0);
        CHECK(e.beatPosition < patternLoopLengthBeats(swell));
    }

    // Il crescendo parte piano e cresce senza mai tornare indietro.
    CHECK(ccEvents.front().velocity == crescendoStartValue);
    CHECK(ccEvents.back().velocity > ccEvents.front().velocity);

    for (size_t i = 1; i < ccEvents.size(); ++i)
    {
        INFO("punto " << i);
        CHECK(ccEvents[i].velocity >= ccEvents[i - 1].velocity);
        CHECK(ccEvents[i].beatPosition > ccEvents[i - 1].beatPosition);
    }
}

TEST_CASE("crescendoCurve scala anche le velocity, in crescita lungo il loop", "[ArpeggiatorEngine]")
{
    PatternDefinition swell;
    swell.noteOrderSequence = {0, 0, 0, 0};
    swell.rhythmGrid = {1.0f, 1.0f, 1.0f, 1.0f};
    swell.velocityCurve = {100, 100, 100, 100};
    swell.crescendoCurve = true;

    const VoicingResult voicing{{60}, 0};
    const auto events = generateSequence(swell, voicing);

    std::vector<int> velocities;
    for (const auto& e : events)
        if (e.kind == NoteEvent::Kind::NoteOn)
            velocities.push_back(e.velocity);

    REQUIRE(velocities.size() == 4);
    CHECK(velocities[0] < velocities[1]);
    CHECK(velocities[1] < velocities[2]);
    CHECK(velocities[2] < velocities[3]);
    CHECK(velocities[3] <= 100); // il crescendo non supera mai la velocity scritta nel pattern
}

TEST_CASE("senza crescendoCurve non viene emesso alcun control change", "[ArpeggiatorEngine]")
{
    PatternDefinition plain;
    plain.noteOrderSequence = {0, 1};
    plain.rhythmGrid = {1.0f, 1.0f};

    const VoicingResult voicing{{60, 64}, 1};
    const auto events = generateSequence(plain, voicing);

    for (const auto& e : events)
        CHECK(e.kind != NoteEvent::Kind::ControlChange);
}

TEST_CASE("i control change precedono le note che cadono sullo stesso campione", "[ArpeggiatorEngine]")
{
    const std::vector<NoteEvent> loopEvents = {
        {NoteEvent::Kind::NoteOn, 60, defaultVelocity, 0.0},
        {NoteEvent::Kind::NoteOff, 60, 0, 1.0},
        {NoteEvent::Kind::ControlChange, expressionController, 40, 0.0},
    };

    const auto scheduled = scheduleEventsInWindow(loopEvents, 2.0, 0.0, 0.5, 100.0, 50);

    REQUIRE(scheduled.size() == 2);
    CHECK(scheduled[0].event.kind == NoteEvent::Kind::ControlChange);
    CHECK(scheduled[1].event.kind == NoteEvent::Kind::NoteOn);
}

TEST_CASE("MidiOutputManager ignora i control change nel conteggio delle note", "[ArpeggiatorEngine]")
{
    MidiOutputManager output;

    output.handleEvent({NoteEvent::Kind::ControlChange, expressionController, 90, 0.0});
    CHECK(output.getActiveNotes().empty());

    output.handleEvent({NoteEvent::Kind::NoteOn, 60, defaultVelocity, 0.0});
    output.handleEvent({NoteEvent::Kind::ControlChange, expressionController, 120, 0.5});
    CHECK(output.getActiveNotes() == std::vector<int>{60});

    output.handleEvent({NoteEvent::Kind::NoteOff, 60, 0, 1.0});
    CHECK(output.getActiveNotes().empty());
}

// --- octaveSpread e loopLength ----------------------------------------------------

TEST_CASE("octaveSpread fa salire il pattern di ottava lungo il loop", "[ArpeggiatorEngine]")
{
    PatternDefinition rising;
    rising.noteOrderSequence = {0, 0, 0, 0};
    rising.rhythmGrid = {0.25f, 0.25f, 0.25f, 0.25f};
    rising.octaveSpread = 1;

    const VoicingResult voicing{{60}, 0};
    const auto events = generateSequence(rising, voicing);

    // Quattro step su due bande: i primi due nell'ottava di partenza, gli altri sopra.
    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 60, 0.25, defaultVelocity));
    CHECK(hasNoteOn(events, 72, 0.5, defaultVelocity));
    CHECK(hasNoteOn(events, 72, 0.75, defaultVelocity));
}

TEST_CASE("un octaveSpread negativo fa scendere il pattern", "[ArpeggiatorEngine]")
{
    PatternDefinition falling;
    falling.noteOrderSequence = {0, 0};
    falling.rhythmGrid = {0.5f, 0.5f};
    falling.octaveSpread = -1;

    const VoicingResult voicing{{60}, 0};
    const auto events = generateSequence(falling, voicing);

    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 48, 0.5, defaultVelocity));
}

TEST_CASE("octaveSpread a zero lascia il pattern dov'e'", "[ArpeggiatorEngine]")
{
    PatternDefinition flat;
    flat.noteOrderSequence = {0, 1, 2, 0};
    flat.rhythmGrid = {0.25f, 0.25f, 0.25f, 0.25f};

    const VoicingResult voicing{{60, 64, 67}, 2};
    const auto events = generateSequence(flat, voicing);

    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 64, 0.25, defaultVelocity));
    CHECK(hasNoteOn(events, 67, 0.5, defaultVelocity));
    CHECK(hasNoteOn(events, 60, 0.75, defaultVelocity));
}

TEST_CASE("loopLength ripete la griglia e ruota la sequenza a ogni passaggio", "[ArpeggiatorEngine]")
{
    PatternDefinition twoBars;
    twoBars.noteOrderSequence = {0, 1, 2};
    twoBars.rhythmGrid = {1.0f, 1.0f, 1.0f};
    twoBars.loopLength = 2;

    const VoicingResult voicing{{60, 64, 67}, 2};
    const auto events = generateSequence(twoBars, voicing);

    CHECK(patternLoopLengthBeats(twoBars) == Catch::Approx(6.0));

    // Primo passaggio: la sequenza cosi' com'e'.
    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 64, 1.0, defaultVelocity));
    CHECK(hasNoteOn(events, 67, 2.0, defaultVelocity));

    // Secondo passaggio: ruotata di uno, quindi non e' una copia del primo.
    CHECK(hasNoteOn(events, 64, 3.0, defaultVelocity));
    CHECK(hasNoteOn(events, 67, 4.0, defaultVelocity));
    CHECK(hasNoteOn(events, 60, 5.0, defaultVelocity));
}

TEST_CASE("loopLength a 0 o 1 lascia il pattern a un solo passaggio", "[ArpeggiatorEngine]")
{
    PatternDefinition pattern;
    pattern.noteOrderSequence = {0, 1};
    pattern.rhythmGrid = {1.0f, 1.0f};

    const VoicingResult voicing{{60, 64}, 1};
    const auto single = generateSequence(pattern, voicing);

    pattern.loopLength = 1;
    const auto explicitSingle = generateSequence(pattern, voicing);

    CHECK(patternLoopLengthBeats(pattern) == Catch::Approx(2.0));
    REQUIRE(single.size() == explicitSingle.size());
    for (size_t i = 0; i < single.size(); ++i)
    {
        CHECK(single[i].midiNote == explicitSingle[i].midiNote);
        CHECK(single[i].beatPosition == Catch::Approx(explicitSingle[i].beatPosition));
    }
}

TEST_CASE("loopLength e octaveSpread si distribuiscono sull'intero ciclo", "[ArpeggiatorEngine]")
{
    PatternDefinition twoBars;
    twoBars.noteOrderSequence = {0, 0};
    twoBars.rhythmGrid = {1.0f, 1.0f};
    twoBars.loopLength = 2;
    twoBars.octaveSpread = 1;

    const VoicingResult voicing{{60}, 0};
    const auto events = generateSequence(twoBars, voicing);

    // Quattro step in tutto: la banda di ottava cambia a meta' ciclo, non a meta' battuta.
    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 60, 1.0, defaultVelocity));
    CHECK(hasNoteOn(events, 72, 2.0, defaultVelocity));
    CHECK(hasNoteOn(events, 72, 3.0, defaultVelocity));
}

TEST_CASE("loopLength conserva i raggruppamenti di uno strum", "[ArpeggiatorEngine]")
{
    // Due note per step: la rotazione deve muoversi di un gruppo intero, altrimenti
    // spezzerebbe le coppie che formano la strimpellata.
    PatternDefinition strummed;
    strummed.noteOrderSequence = {0, 1, 2, 3};
    strummed.rhythmGrid = {1.0f, 1.0f};
    strummed.loopLength = 2;

    const VoicingResult voicing{{60, 64, 67, 72}, 3};
    const auto events = generateSequence(strummed, voicing);

    CHECK(hasNoteOn(events, 60, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 64, 0.0, defaultVelocity));
    CHECK(hasNoteOn(events, 67, 1.0, defaultVelocity));
    CHECK(hasNoteOn(events, 72, 1.0, defaultVelocity));

    // Secondo passaggio: i gruppi si scambiano, restando gruppi.
    CHECK(hasNoteOn(events, 67, 2.0, defaultVelocity));
    CHECK(hasNoteOn(events, 72, 2.0, defaultVelocity));
    CHECK(hasNoteOn(events, 60, 3.0, defaultVelocity));
    CHECK(hasNoteOn(events, 64, 3.0, defaultVelocity));
}

// --- Palm mute e strimpellata parziale ---------------------------------------------

TEST_CASE("palmMute accorcia e attenua solo gli step marcati", "[ArpeggiatorEngine]")
{
    PatternDefinition chugging;
    chugging.noteOrderSequence = {0, 0, 0, 0};
    chugging.rhythmGrid = {0.25f, 0.25f, 0.25f, 0.25f};
    chugging.gateLength = {0.8f, 0.8f, 0.8f, 0.8f};
    chugging.velocityCurve = {100, 100, 100, 100};
    chugging.palmMute = {true, false, true, false};

    const VoicingResult voicing{{60}, 0};
    const auto events = generateSequence(chugging, voicing);

    // Step 0 e 2 mutati: nota piu' corta (gate 0.8 * 0.35) e piu' piano.
    CHECK(hasNoteOff(events, 60, 0.25 * 0.8 * palmMuteGateScale));
    CHECK(hasNoteOff(events, 60, 0.25 + 0.25 * 0.8));

    std::vector<int> velocities;
    for (const auto& e : events)
        if (e.kind == NoteEvent::Kind::NoteOn)
            velocities.push_back(e.velocity);

    REQUIRE(velocities.size() == 4);
    CHECK(velocities[0] == 70);
    CHECK(velocities[1] == 100);
    CHECK(velocities[2] == 70);
    CHECK(velocities[3] == 100);
}

TEST_CASE("un array palmMute piu' corto della griglia lascia liberi gli step mancanti", "[ArpeggiatorEngine]")
{
    PatternDefinition pattern;
    pattern.noteOrderSequence = {0, 0, 0};
    pattern.rhythmGrid = {1.0f, 1.0f, 1.0f};
    pattern.velocityCurve = {100, 100, 100};
    pattern.palmMute = {true};

    const VoicingResult voicing{{60}, 0};
    const auto events = generateSequence(pattern, voicing);

    std::vector<int> velocities;
    for (const auto& e : events)
        if (e.kind == NoteEvent::Kind::NoteOn)
            velocities.push_back(e.velocity);

    REQUIRE(velocities.size() == 3);
    CHECK(velocities[0] == 70);
    CHECK(velocities[1] == 100);
    CHECK(velocities[2] == 100);
}

TEST_CASE("una strimpellata parziale si ottiene con le pause dentro lo step", "[ArpeggiatorEngine]")
{
    // Quattro indici su due step: il secondo step suona solo le corde gravi.
    PatternDefinition partial;
    partial.noteOrderSequence = {0, 1, 2, restNoteIndex};
    partial.rhythmGrid = {1.0f, 1.0f};
    partial.gateLength = {1.0f, 1.0f};

    const VoicingResult voicing{{60, 64, 67}, 2};
    const auto events = generateSequence(partial, voicing);

    // Primo step: due note. Secondo step: una sola, la pausa non suona.
    int firstStepNotes = 0;
    int secondStepNotes = 0;
    for (const auto& e : events)
    {
        if (e.kind != NoteEvent::Kind::NoteOn)
            continue;
        (e.beatPosition < 1.0 ? firstStepNotes : secondStepNotes)++;
    }

    CHECK(firstStepNotes == 2);
    CHECK(secondStepNotes == 1);
}

// --- Gate globale e swing globale (SPEC.md sezione 8) -------------------------------

TEST_CASE("gateLengthMultiplier scala il gate di ogni step", "[ArpeggiatorEngine]")
{
    PatternDefinition pattern;
    pattern.noteOrderSequence = {0, 1};
    pattern.rhythmGrid = {1.0f, 1.0f};
    pattern.gateLength = {0.5f, 0.5f};

    const VoicingResult voicing{{60, 64}, 1};

    SyncClock shortened;
    shortened.gateLengthMultiplier = 0.5;
    const auto shortEvents = generateSequence(pattern, voicing, shortened);
    CHECK(hasNoteOff(shortEvents, 60, 0.25));

    SyncClock lengthened;
    lengthened.gateLengthMultiplier = 1.5;
    const auto longEvents = generateSequence(pattern, voicing, lengthened);
    CHECK(hasNoteOff(longEvents, 60, 0.75));
}

TEST_CASE("un gateLengthMultiplier non valido lascia il gate del pattern invariato", "[ArpeggiatorEngine]")
{
    PatternDefinition pattern;
    pattern.noteOrderSequence = {0};
    pattern.rhythmGrid = {1.0f};
    pattern.gateLength = {0.6f};

    const VoicingResult voicing{{60}, 0};

    SyncClock broken;
    broken.gateLengthMultiplier = 0.0;

    const auto events = generateSequence(pattern, voicing, broken);
    CHECK(hasNoteOff(events, 60, 0.6));
}

TEST_CASE("gateLengthMultiplier non spegne mai la nota nell'istante in cui si accende", "[ArpeggiatorEngine]")
{
    PatternDefinition pattern;
    pattern.noteOrderSequence = {0};
    pattern.rhythmGrid = {1.0f};
    pattern.gateLength = {0.5f};

    const VoicingResult voicing{{60}, 0};

    SyncClock silenced;
    silenced.gateLengthMultiplier = 0.0001;

    const auto events = generateSequence(pattern, voicing, silenced);

    double onBeat = -1.0, offBeat = -1.0;
    for (const auto& e : events)
    {
        if (e.kind == NoteEvent::Kind::NoteOn) onBeat = e.beatPosition;
        if (e.kind == NoteEvent::Kind::NoteOff) offBeat = e.beatPosition;
    }
    CHECK(offBeat > onBeat);
}

TEST_CASE("globalSwingAmount di SyncClock si somma allo swing del pattern", "[ArpeggiatorEngine]")
{
    // Copre il percorso reale usato dal plugin (SPEC.md sezione 8: swing globale
    // automatizzabile), non solo la combinazione gia' testata altrove con swing di pattern.
    PatternDefinition pattern;
    pattern.noteOrderSequence = {0, 0};
    pattern.rhythmGrid = {1.0f, 1.0f};

    const VoicingResult voicing{{60}, 0};

    SyncClock swung;
    swung.globalSwingAmount = 0.4;

    const auto events = generateSequence(pattern, voicing, swung);
    CHECK(hasNoteOn(events, 60, 1.2, defaultVelocity));
}
