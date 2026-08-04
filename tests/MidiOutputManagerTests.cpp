#include <catch2/catch_test_macros.hpp>

#include "smartchord/MidiOutputManager.h"

#include <algorithm>

using namespace smartchord;

namespace
{
    NoteEvent noteOn (int midiNote, double beat = 0.0, int velocity = defaultVelocity)
    {
        return { NoteEvent::Kind::NoteOn, midiNote, velocity, beat };
    }

    NoteEvent noteOff (int midiNote, double beat = 0.0)
    {
        return { NoteEvent::Kind::NoteOff, midiNote, 0, beat };
    }
}

TEST_CASE("MidiOutputManager starts with no active notes", "[MidiOutputManager]")
{
    const MidiOutputManager manager;
    CHECK(manager.getActiveNotes().empty());
    CHECK_FALSE(manager.isNoteActive(60));
}

TEST_CASE("MidiOutputManager tracks a note as active between NoteOn and NoteOff", "[MidiOutputManager]")
{
    MidiOutputManager manager;

    manager.handleEvent(noteOn(60));
    CHECK(manager.isNoteActive(60));
    CHECK(manager.getActiveNotes() == std::vector<int>{60});

    manager.handleEvent(noteOff(60));
    CHECK_FALSE(manager.isNoteActive(60));
    CHECK(manager.getActiveNotes().empty());
}

TEST_CASE("MidiOutputManager keeps a repeated note active until every NoteOn has a matching NoteOff", "[MidiOutputManager]")
{
    MidiOutputManager manager;

    manager.handleEvent(noteOn(60));
    manager.handleEvent(noteOn(60));
    CHECK(manager.isNoteActive(60));

    manager.handleEvent(noteOff(60));
    CHECK(manager.isNoteActive(60)); // ancora un NoteOn "in sospeso"

    manager.handleEvent(noteOff(60));
    CHECK_FALSE(manager.isNoteActive(60));
}

TEST_CASE("MidiOutputManager ignores an unmatched NoteOff", "[MidiOutputManager]")
{
    MidiOutputManager manager;
    manager.handleEvent(noteOff(60));
    CHECK(manager.getActiveNotes().empty());
    CHECK_FALSE(manager.isNoteActive(60));
}

TEST_CASE("MidiOutputManager::allNotesOff closes every active note and resets the state", "[MidiOutputManager]")
{
    MidiOutputManager manager;
    manager.handleEvent(noteOn(60));
    manager.handleEvent(noteOn(64));
    manager.handleEvent(noteOn(67));

    const auto offEvents = manager.allNotesOff(2.5);

    REQUIRE(offEvents.size() == 3);
    for (const auto& e : offEvents)
    {
        CHECK(e.kind == NoteEvent::Kind::NoteOff);
        CHECK(e.beatPosition == 2.5);
    }

    std::vector<int> offNotes;
    for (const auto& e : offEvents)
        offNotes.push_back(e.midiNote);
    std::sort(offNotes.begin(), offNotes.end());
    CHECK(offNotes == std::vector<int>{60, 64, 67});

    CHECK(manager.getActiveNotes().empty());
    CHECK_FALSE(manager.isNoteActive(60));
}

TEST_CASE("MidiOutputManager::allNotesOff is a no-op when nothing is active", "[MidiOutputManager]")
{
    MidiOutputManager manager;
    CHECK(manager.allNotesOff().empty());
}

TEST_CASE("MidiOutputManager naturally clears itself after a full, uninterrupted ArpeggiatorEngine sequence", "[MidiOutputManager]")
{
    PatternDefinition asPlayed;
    asPlayed.noteOrderSequence = {0, 1, 2};
    asPlayed.rhythmGrid = {1.0f, 1.0f, 1.0f};
    asPlayed.gateLength = {0.95f, 0.95f, 0.95f};

    const VoicingResult voicing{{60, 64, 67}, 2};
    const auto events = generateSequence(asPlayed, voicing);

    MidiOutputManager manager;
    for (const auto& e : events)
        manager.handleEvent(e);

    CHECK(manager.getActiveNotes().empty());
}

TEST_CASE("MidiOutputManager::allNotesOff cleans up notes left hanging by a chord change mid-pattern", "[MidiOutputManager]")
{
    PatternDefinition asPlayed;
    asPlayed.noteOrderSequence = {0, 1, 2};
    asPlayed.rhythmGrid = {1.0f, 1.0f, 1.0f};
    asPlayed.gateLength = {0.95f, 0.95f, 0.95f};

    const VoicingResult voicing{{60, 64, 67}, 2};
    const auto events = generateSequence(asPlayed, voicing);

    MidiOutputManager manager;

    // Simula un'interruzione a meta' pattern: solo i primi due NoteOn sono arrivati
    // prima del cambio di accordo, i rispettivi NoteOff non hanno ancora suonato.
    manager.handleEvent(events[0]); // NoteOn 60
    manager.handleEvent(events[2]); // NoteOn 64

    REQUIRE(manager.getActiveNotes().size() == 2);

    const auto offEvents = manager.allNotesOff();
    REQUIRE(offEvents.size() == 2);
    CHECK(manager.getActiveNotes().empty());
}
