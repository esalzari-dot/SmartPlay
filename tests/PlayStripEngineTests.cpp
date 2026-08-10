#include <catch2/catch_test_macros.hpp>

#include "smartchord/PlayStripEngine.h"

#include <algorithm>

using namespace smartchord;

namespace
{
    bool hasNoteOn (const std::vector<StripNoteEvent>& events, int midiNote, int velocity)
    {
        for (const auto& e : events)
            if (e.kind == StripNoteEvent::Kind::NoteOn && e.midiNote == midiNote && e.velocity == velocity)
                return true;
        return false;
    }

    bool hasNoteOff (const std::vector<StripNoteEvent>& events, int midiNote)
    {
        for (const auto& e : events)
            if (e.kind == StripNoteEvent::Kind::NoteOff && e.midiNote == midiNote)
                return true;
        return false;
    }

    int countKind (const std::vector<StripNoteEvent>& events, StripNoteEvent::Kind kind)
    {
        int n = 0;
        for (const auto& e : events)
            if (e.kind == kind)
                ++n;
        return n;
    }
}

TEST_CASE("notchCountForFamily returns the fixed notch count per family", "[PlayStripEngine]")
{
    CHECK(notchCountForFamily(InstrumentFamily::Piano) == 7);
    CHECK(notchCountForFamily(InstrumentFamily::Guitar) == 6);
    CHECK(notchCountForFamily(InstrumentFamily::Bass) == 4);
    CHECK(notchCountForFamily(InstrumentFamily::Strings) == 4);
}

TEST_CASE("notesForStrip returns notchCountForFamily notes from the chord's scale, sorted and within tessitura", "[PlayStripEngine]")
{
    const ChordDefinition cMajor{0, ChordQuality::Maj, 0, 0};

    for (auto family : { InstrumentFamily::Piano, InstrumentFamily::Guitar,
                          InstrumentFamily::Bass, InstrumentFamily::Strings })
    {
        INFO("family index " << static_cast<int>(family));
        const auto notes = notesForStrip(cMajor, family);
        const auto profile = getVoicingProfile(family);
        const auto scale = getChordScaleTones(ChordQuality::Maj);

        // La tessitura di ciascuna famiglia (VoicingEngine sezione 4) e' ampia abbastanza
        // da contenere sempre almeno notchCountForFamily note della scala: il campionamento
        // non deve mai restituire meno tacche del numero fisso previsto.
        CHECK(notes.size() == static_cast<size_t>(notchCountForFamily(family)));

        for (int n : notes)
        {
            CHECK(n >= profile.midiRangeLow);
            CHECK(n <= profile.midiRangeHigh);
            const int pitchClass = ((n % 12) + 12) % 12;
            CHECK(std::find(scale.begin(), scale.end(), pitchClass) != scale.end());
        }

        CHECK(std::is_sorted(notes.begin(), notes.end()));
    }
}

TEST_CASE("PlayStripEngine Down emits a single NoteOn at the tapped notch", "[PlayStripEngine]")
{
    PlayStripEngine engine({60, 64, 67, 72, 76, 79});

    const auto events = engine.processGesture({StripGesturePhase::Down, 0.0f, 0.0});

    REQUIRE(events.size() == 1);
    CHECK(hasNoteOn(events, 60, defaultVelocity));
}

TEST_CASE("PlayStripEngine Up without movement closes the same note it opened", "[PlayStripEngine]")
{
    PlayStripEngine engine({60, 64, 67, 72, 76, 79});

    engine.processGesture({StripGesturePhase::Down, 0.0f, 0.0});
    const auto events = engine.processGesture({StripGesturePhase::Up, 0.0f, 0.3});

    REQUIRE(events.size() == 1);
    CHECK(hasNoteOff(events, 60));
}

TEST_CASE("PlayStripEngine Move within the same notch produces no events", "[PlayStripEngine]")
{
    PlayStripEngine engine({60, 64, 67, 72, 76, 79});

    engine.processGesture({StripGesturePhase::Down, 0.0f, 0.0});
    const auto events = engine.processGesture({StripGesturePhase::Move, 0.05f, 0.05});

    CHECK(events.empty());
}

TEST_CASE("PlayStripEngine Move crossing one notch retriggers NoteOff+NoteOn", "[PlayStripEngine]")
{
    PlayStripEngine engine({60, 64, 67, 72, 76, 79});

    engine.processGesture({StripGesturePhase::Down, 0.0f, 0.0}); // notch 0 -> 60
    const auto events = engine.processGesture({StripGesturePhase::Move, 0.2f, 0.1}); // notch round(0.2*5)=1 -> 64

    REQUIRE(events.size() == 2);
    CHECK(hasNoteOff(events, 60));
    CHECK(hasNoteOn(events, 64, stripMaxVelocity)); // 0.1s per crossing = 10 crossing/s > soglia massima
}

TEST_CASE("PlayStripEngine Move crossing multiple notches at once retriggers each intermediate note", "[PlayStripEngine]")
{
    PlayStripEngine engine({60, 64, 67, 72, 76, 79});

    engine.processGesture({StripGesturePhase::Down, 0.0f, 0.0}); // notch 0 -> 60
    // notch 5 -> 79, 5 attraversamenti in 0.1s: 50 crossing/s, ben oltre la soglia di
    // velocity massima (8/s), cosi' il test non dipende dall'interpolazione lineare.
    const auto events = engine.processGesture({StripGesturePhase::Move, 1.0f, 0.1});

    REQUIRE(events.size() == 10); // 5 NoteOff + 5 NoteOn
    CHECK(countKind(events, StripNoteEvent::Kind::NoteOff) == 5);
    CHECK(countKind(events, StripNoteEvent::Kind::NoteOn) == 5);

    CHECK(hasNoteOff(events, 60));
    CHECK(hasNoteOff(events, 64));
    CHECK(hasNoteOff(events, 67));
    CHECK(hasNoteOff(events, 72));
    CHECK(hasNoteOff(events, 76));
    CHECK(hasNoteOn(events, 64, stripMaxVelocity));
    CHECK(hasNoteOn(events, 67, stripMaxVelocity));
    CHECK(hasNoteOn(events, 72, stripMaxVelocity));
    CHECK(hasNoteOn(events, 76, stripMaxVelocity));
    CHECK(hasNoteOn(events, 79, stripMaxVelocity));

    // Le note intermedie non restano "appese": ogni NoteOn intermedio e' seguito dal suo
    // NoteOff prima della fine del gesto, tranne l'ultima nota (79), che resta accesa finche'
    // non arriva Up.
    const auto afterUp = engine.processGesture({StripGesturePhase::Up, 1.0f, 0.5});
    REQUIRE(afterUp.size() == 1);
    CHECK(hasNoteOff(afterUp, 79));
}

TEST_CASE("PlayStripEngine crossing velocity scales down for a slow drag", "[PlayStripEngine]")
{
    PlayStripEngine engine({60, 64, 67, 72, 76, 79});

    engine.processGesture({StripGesturePhase::Down, 0.0f, 0.0});
    // 1 attraversamento in 1s = 1 crossing/s, ben sotto la soglia di velocity massima (8/s).
    const auto events = engine.processGesture({StripGesturePhase::Move, 0.2f, 1.0});

    REQUIRE(hasNoteOff(events, 60));
    bool foundScaledDown = false;
    for (const auto& e : events)
        if (e.kind == StripNoteEvent::Kind::NoteOn && e.midiNote == 64)
        {
            CHECK(e.velocity > stripMinVelocity);
            CHECK(e.velocity < stripMaxVelocity);
            foundScaledDown = true;
        }
    CHECK(foundScaledDown);
}

TEST_CASE("PlayStripEngine ignores Move/Up without a preceding Down", "[PlayStripEngine]")
{
    PlayStripEngine engine({60, 64, 67});

    CHECK(engine.processGesture({StripGesturePhase::Move, 0.5f, 0.0}).empty());
    CHECK(engine.processGesture({StripGesturePhase::Up, 0.5f, 0.1}).empty());
}

TEST_CASE("PlayStripEngine with empty notes never produces events", "[PlayStripEngine]")
{
    PlayStripEngine engine({});

    CHECK(engine.processGesture({StripGesturePhase::Down, 0.5f, 0.0}).empty());
    CHECK(engine.processGesture({StripGesturePhase::Move, 0.8f, 0.1}).empty());
    CHECK(engine.processGesture({StripGesturePhase::Up, 0.8f, 0.2}).empty());
}
