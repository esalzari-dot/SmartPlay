#include "smartchord/PlayStripEngine.h"

#include <algorithm>
#include <cmath>

namespace smartchord
{

int notchCountForFamily (InstrumentFamily family)
{
    switch (family)
    {
        case InstrumentFamily::Piano:   return 7;
        case InstrumentFamily::Guitar:  return 6;
        case InstrumentFamily::Bass:    return 4;
        case InstrumentFamily::Strings: return 4;
    }

    return 7;
}

std::vector<int> notesForStrip (const ChordDefinition& chord, InstrumentFamily family)
{
    // MIDI 60 (C4) e' la nota di riferimento per octaveOffset == 0, stessa convenzione di
    // VoicingEngine.
    constexpr int referenceMiddleC = 60;

    const auto profile = getVoicingProfile (family);
    const auto scaleTones = getChordScaleTones (chord.quality);
    if (scaleTones.empty())
        return {};

    const int baseRoot = referenceMiddleC + chord.rootSemitone + 12 * chord.octaveOffset;

    std::vector<int> allNotes;
    for (int octave = -8; octave <= 8; ++octave)
    {
        for (int interval : scaleTones)
        {
            const int note = baseRoot + interval + 12 * octave;
            if (note >= profile.midiRangeLow && note <= profile.midiRangeHigh)
                allNotes.push_back (note);
        }
    }

    std::sort (allNotes.begin(), allNotes.end());
    allNotes.erase (std::unique (allNotes.begin(), allNotes.end()), allNotes.end());

    const int notchCount = notchCountForFamily (family);
    if (static_cast<int> (allNotes.size()) <= notchCount)
        return allNotes;

    std::vector<int> picked;
    picked.reserve (static_cast<size_t> (notchCount));
    for (int i = 0; i < notchCount; ++i)
    {
        const auto idx = static_cast<size_t> (std::lround (
            i * static_cast<double> (allNotes.size() - 1) / static_cast<double> (notchCount - 1)));
        picked.push_back (allNotes[idx]);
    }
    return picked;
}

PlayStripEngine::PlayStripEngine (std::vector<int> notesIn)
    : notes (std::move (notesIn))
{
}

void PlayStripEngine::setNotes (std::vector<int> notesIn)
{
    notes = std::move (notesIn);
}

int PlayStripEngine::notchIndexForPosition (float position) const
{
    const float clamped = std::clamp (position, 0.0f, 1.0f);
    const int lastIndex = static_cast<int> (notes.size()) - 1;
    return static_cast<int> (std::lround (clamped * static_cast<float> (lastIndex)));
}

int PlayStripEngine::velocityForCrossing (double intervalSeconds)
{
    if (intervalSeconds <= 0.0)
        return stripMaxVelocity;

    const double crossingsPerSecond = 1.0 / intervalSeconds;
    const double ratio = std::clamp (crossingsPerSecond / stripCrossingsPerSecondForMaxVelocity, 0.0, 1.0);

    return static_cast<int> (std::lround (stripMinVelocity + ratio * (stripMaxVelocity - stripMinVelocity)));
}

std::vector<StripNoteEvent> PlayStripEngine::processGesture (const StripGesture& gesture)
{
    std::vector<StripNoteEvent> events;

    if (notes.empty())
        return events;

    switch (gesture.phase)
    {
        case StripGesturePhase::Down:
        {
            isDown = true;
            currentNotchIndex = notchIndexForPosition (gesture.position);
            lastEventTimestampSeconds = gesture.timestampSeconds;

            events.push_back ({ StripNoteEvent::Kind::NoteOn,
                                 notes[static_cast<size_t> (currentNotchIndex)],
                                 defaultVelocity, gesture.timestampSeconds });
            break;
        }

        case StripGesturePhase::Move:
        {
            if (! isDown)
                break;

            const int targetNotchIndex = notchIndexForPosition (gesture.position);
            if (targetNotchIndex == currentNotchIndex)
                break;

            const int numCrossings = std::abs (targetNotchIndex - currentNotchIndex);
            const int step = targetNotchIndex > currentNotchIndex ? 1 : -1;
            const double intervalSeconds = (gesture.timestampSeconds - lastEventTimestampSeconds)
                                          / static_cast<double> (numCrossings);
            const int velocity = velocityForCrossing (intervalSeconds);

            double crossingTimestamp = lastEventTimestampSeconds;
            for (int i = 0; i < numCrossings; ++i)
            {
                crossingTimestamp += intervalSeconds;

                events.push_back ({ StripNoteEvent::Kind::NoteOff,
                                     notes[static_cast<size_t> (currentNotchIndex)],
                                     0, crossingTimestamp });

                currentNotchIndex += step;

                events.push_back ({ StripNoteEvent::Kind::NoteOn,
                                     notes[static_cast<size_t> (currentNotchIndex)],
                                     velocity, crossingTimestamp });
            }

            lastEventTimestampSeconds = gesture.timestampSeconds;
            break;
        }

        case StripGesturePhase::Up:
        {
            if (! isDown)
                break;

            events.push_back ({ StripNoteEvent::Kind::NoteOff,
                                 notes[static_cast<size_t> (currentNotchIndex)],
                                 0, gesture.timestampSeconds });

            isDown = false;
            currentNotchIndex = -1;
            break;
        }
    }

    return events;
}

} // namespace smartchord
