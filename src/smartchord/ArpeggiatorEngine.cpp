#include "smartchord/ArpeggiatorEngine.h"

#include <algorithm>
#include <cmath>

namespace smartchord
{

namespace
{
    struct ResolvedNote
    {
        int noteIndex;
        int octaveShift;
    };

    ResolvedNote resolveNoteIndex (int idx, int voicingSize)
    {
        const int wrapped = ((idx % voicingSize) + voicingSize) % voicingSize;
        const int octaveShift = (idx - wrapped) / voicingSize;
        return { wrapped, octaveShift };
    }

    double msToBeats (double milliseconds, double bpm)
    {
        if (bpm <= 0.0)
            return 0.0;
        return milliseconds * (bpm / 60000.0);
    }
}

std::vector<NoteEvent> generateSequence (const PatternDefinition& pattern,
                                          const VoicingResult& voicing,
                                          const SyncClock& clock)
{
    std::vector<NoteEvent> events;

    const int voicingSize = static_cast<int> (voicing.notes.size());
    const int stepCount = static_cast<int> (pattern.rhythmGrid.size());

    if (voicingSize == 0 || stepCount == 0 || pattern.noteOrderSequence.empty())
        return events;

    // Se ci sono piu' indici che step, gli indici in eccesso condividono lo stesso step
    // (es. un accordo di chitarra strimpellato su un unico rhythmGrid slot).
    const int notesPerStep = std::max (1, static_cast<int> (pattern.noteOrderSequence.size()) / stepCount);

    const double effectiveSwing = std::min (1.0, std::max (0.0,
        static_cast<double> (pattern.swingAmount) + clock.globalSwingAmount));

    double beatCursor = 0.0;

    for (int step = 0; step < stepCount; ++step)
    {
        const float stepDuration = pattern.rhythmGrid[static_cast<size_t> (step)];

        double stepStartBeat = beatCursor;
        if (effectiveSwing > 0.0 && (step % 2) == 1)
            stepStartBeat += effectiveSwing * stepDuration * 0.5;

        const float gate = static_cast<size_t> (step) < pattern.gateLength.size()
            ? pattern.gateLength[static_cast<size_t> (step)] : 1.0f;
        const int velocity = static_cast<size_t> (step) < pattern.velocityCurve.size()
            ? pattern.velocityCurve[static_cast<size_t> (step)] : defaultVelocity;
        const double noteOffBeat = stepStartBeat + stepDuration * gate;

        const size_t firstIndex = static_cast<size_t> (step) * static_cast<size_t> (notesPerStep);
        const size_t lastIndex = std::min (firstIndex + static_cast<size_t> (notesPerStep),
                                            pattern.noteOrderSequence.size());

        for (size_t i = firstIndex; i < lastIndex; ++i)
        {
            const auto resolved = resolveNoteIndex (pattern.noteOrderSequence[i], voicingSize);
            const int midiNote = voicing.notes[static_cast<size_t> (resolved.noteIndex)] + 12 * resolved.octaveShift;

            const double strumOffsetBeats = msToBeats (pattern.strumOffsetMs * static_cast<double> (i - firstIndex), clock.bpm);
            const double noteStartBeat = stepStartBeat + strumOffsetBeats;

            events.push_back ({ NoteEvent::Kind::NoteOn, midiNote, velocity, noteStartBeat });
            events.push_back ({ NoteEvent::Kind::NoteOff, midiNote, 0, noteOffBeat });
        }

        beatCursor += stepDuration;
    }

    return events;
}

double patternLoopLengthBeats (const PatternDefinition& pattern)
{
    double total = 0.0;
    for (float duration : pattern.rhythmGrid)
        total += duration;
    return total;
}

std::vector<ScheduledEvent> scheduleEventsInWindow (const std::vector<NoteEvent>& loopEvents,
                                                     double loopLengthBeats,
                                                     double windowStartBeat,
                                                     double windowLengthBeats,
                                                     double samplesPerBeat,
                                                     int blockNumSamples)
{
    std::vector<ScheduledEvent> result;

    if (loopEvents.empty() || loopLengthBeats <= 0.0 || windowLengthBeats <= 0.0)
        return result;

    double localStart = std::fmod (windowStartBeat, loopLengthBeats);
    if (localStart < 0.0)
        localStart += loopLengthBeats;

    double remaining = windowLengthBeats;
    double sampleBase = 0.0;

    while (remaining > 0.0)
    {
        const double localEnd = std::min (localStart + remaining, loopLengthBeats);
        const double consumedThisPass = localEnd - localStart;

        for (const auto& event : loopEvents)
        {
            if (event.beatPosition >= localStart && event.beatPosition < localEnd)
            {
                const double beatOffset = event.beatPosition - localStart;
                int sampleOffset = static_cast<int> (std::round (sampleBase + beatOffset * samplesPerBeat));
                sampleOffset = std::clamp (sampleOffset, 0, std::max (0, blockNumSamples - 1));
                result.push_back ({ event, sampleOffset });
            }
        }

        sampleBase += consumedThisPass * samplesPerBeat;
        remaining -= consumedThisPass;
        localStart = 0.0; // dopo il primo giro riparte dall'inizio del loop (wrap-around)
    }

    std::sort (result.begin(), result.end(),
               [] (const ScheduledEvent& a, const ScheduledEvent& b) { return a.sampleOffset < b.sampleOffset; });

    return result;
}

ArpeggiatorEngine::ArpeggiatorEngine (const PatternLibrary& patternLibraryIn, const AutoplayGridState& gridStateIn)
    : patternLibrary (patternLibraryIn), gridState (gridStateIn)
{
}

std::vector<NoteEvent> ArpeggiatorEngine::renderChordLoop (const ChordDefinition& chord,
                                                             InstrumentFamily family,
                                                             int chordSlot,
                                                             const SyncClock& clock) const
{
    const int intensityLevel = gridState.getIntensity (family, chordSlot);
    const auto* pattern = resolvePattern (patternLibrary, family, intensityLevel);
    if (pattern == nullptr)
        return {};

    const auto voicing = voiceChord (chord, getVoicingProfile (family));
    return generateSequence (*pattern, voicing, clock);
}

} // namespace smartchord
