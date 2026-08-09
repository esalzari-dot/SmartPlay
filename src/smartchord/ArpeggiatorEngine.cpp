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

    // Profilo del crescendo: 0 all'inizio del loop, 1 alla fine. La curva e' un coseno
    // rialzato invece di una rampa lineare perche' un gonfiare lineare suona meccanico:
    // parte piano, accelera al centro, si assesta sul finale.
    double crescendoAmountAt (double positionInLoop, double loopLength)
    {
        if (loopLength <= 0.0)
            return 1.0;

        const double t = std::clamp (positionInLoop / loopLength, 0.0, 1.0);
        return 0.5 - 0.5 * std::cos (t * 3.14159265358979323846);
    }

    // octaveSpread distribuisce gli step del pattern su piu' ottave nell'arco di un loop:
    // lo stesso arpeggio, invece di girare sempre nella stessa posizione, sale (o scende,
    // con valori negativi) di ottava man mano che il loop avanza.
    int spreadOctaveForStep (int step, int stepCount, int octaveSpread)
    {
        if (octaveSpread == 0 || stepCount <= 0)
            return 0;

        const int magnitude = std::abs (octaveSpread);
        const int band = (step * (magnitude + 1)) / stepCount;
        return octaveSpread > 0 ? band : -band;
    }

    // Numero di punti su cui campionare il CC di expression lungo un loop. Abbastanza
    // fitto da suonare continuo, abbastanza rado da non intasare la traccia registrata.
    constexpr int crescendoResolution = 32;
}

double rateMultiplierFor (PatternRate rate)
{
    switch (rate)
    {
        case PatternRate::Half:    return 2.0;
        case PatternRate::Triplet: return 2.0 / 3.0;
        case PatternRate::Double:  return 0.5;
        case PatternRate::Normal:  break;
    }

    return 1.0;
}

std::vector<NoteEvent> generateSequence (const PatternDefinition& pattern,
                                          const VoicingResult& voicing,
                                          const SyncClock& clock,
                                          std::mt19937* rng)
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

    const double rate = clock.rateMultiplier > 0.0 ? clock.rateMultiplier : 1.0;
    const double loopLength = patternLoopLengthBeats (pattern, rate);

    // loopLength: quante volte rhythmGrid si ripete prima che il ciclo ricominci. Le
    // ripetizioni non sono identiche - la sequenza degli indici ruota di un gruppo per
    // volta e octaveSpread si distribuisce sull'intero ciclo - cosi' un pattern puo'
    // durare piu' battute senza suonare come un anello incollato.
    const int repetitions = std::max (1, pattern.loopLength);
    const int totalSteps = stepCount * repetitions;
    const size_t sequenceSize = pattern.noteOrderSequence.size();

    double beatCursor = 0.0;

    for (int globalStep = 0; globalStep < totalSteps; ++globalStep)
    {
        const int repetition = globalStep / stepCount;
        const int step = globalStep % stepCount;
        const size_t rotation = static_cast<size_t> (repetition * notesPerStep) % sequenceSize;

        const double stepDuration = static_cast<double> (pattern.rhythmGrid[static_cast<size_t> (step)]) * rate;

        double stepStartBeat = beatCursor;
        if (effectiveSwing > 0.0 && (globalStep % 2) == 1)
            stepStartBeat += effectiveSwing * stepDuration * 0.5;

        const bool muted = static_cast<size_t> (step) < pattern.palmMute.size()
            && pattern.palmMute[static_cast<size_t> (step)];

        float gate = static_cast<size_t> (step) < pattern.gateLength.size()
            ? pattern.gateLength[static_cast<size_t> (step)] : 1.0f;
        int velocity = static_cast<size_t> (step) < pattern.velocityCurve.size()
            ? pattern.velocityCurve[static_cast<size_t> (step)] : defaultVelocity;

        // Gate globale (SPEC.md sezione 8): scala il gate di ogni step senza toccare i
        // dati del pattern, cosi' un host puo' automatizzare stondato/legato in tempo
        // reale. clamp a un minimo perche' un gate a 0 spegnerebbe la nota nell'istante
        // in cui si accende.
        gate = std::max (0.02f, gate * static_cast<float> (clock.gateLengthMultiplier > 0.0 ? clock.gateLengthMultiplier : 1.0));

        if (muted)
        {
            gate *= palmMuteGateScale;
            velocity = std::clamp (static_cast<int> (std::lround (velocity * palmMuteVelocityScale)), 1, 127);
        }

        const double noteOffBeat = stepStartBeat + stepDuration * gate;

        const size_t firstIndex = static_cast<size_t> (step) * static_cast<size_t> (notesPerStep);
        const size_t lastIndex = std::min (firstIndex + static_cast<size_t> (notesPerStep),
                                            pattern.noteOrderSequence.size());

        // La strimpellata verso il basso distanzia le note partendo dall'acuto: e'
        // l'ordine degli offset a invertirsi, non le note suonate.
        const bool strumAscending = pattern.strumDirection == StrumDirection::Up
                                  || (pattern.strumDirection == StrumDirection::Alternate && (globalStep % 2) == 0);
        const size_t notesInStep = lastIndex - firstIndex;
        const int stepOctave = spreadOctaveForStep (globalStep, totalSteps, pattern.octaveSpread);

        for (size_t positionInStep = 0; positionInStep < notesInStep; ++positionInStep)
        {
            const int noteIndex = pattern.noteOrderSequence[(firstIndex + rotation + positionInStep) % sequenceSize];
            if (noteIndex == restNoteIndex)
                continue; // pausa: lo step consuma il tempo ma non suona

            const auto resolved = resolveNoteIndex (noteIndex, voicingSize);
            const int midiNote = voicing.notes[static_cast<size_t> (resolved.noteIndex)]
                               + 12 * (resolved.octaveShift + stepOctave);

            const size_t strumSlot = strumAscending ? positionInStep : (notesInStep - 1 - positionInStep);

            const double strumOffsetBeats = msToBeats (pattern.strumOffsetMs * static_cast<double> (strumSlot), clock.bpm);
            double noteStartBeat = stepStartBeat + strumOffsetBeats;
            int noteVelocity = velocity;

            if (rng != nullptr)
            {
                if (pattern.humanizeTiming > 0.0f)
                {
                    std::uniform_real_distribution<double> jitter (-pattern.humanizeTiming, pattern.humanizeTiming);
                    noteStartBeat = std::max (0.0, noteStartBeat + msToBeats (jitter (*rng), clock.bpm));
                }

                if (pattern.humanizeVelocity > 0)
                {
                    std::uniform_int_distribution<int> jitter (-pattern.humanizeVelocity, pattern.humanizeVelocity);
                    noteVelocity = std::clamp (noteVelocity + jitter (*rng), 1, 127);
                }
            }

            if (pattern.crescendoCurve)
            {
                const double amount = crescendoAmountAt (noteStartBeat, loopLength);
                const double scale = (crescendoStartValue / 127.0) + (1.0 - crescendoStartValue / 127.0) * amount;
                noteVelocity = std::clamp (static_cast<int> (std::lround (noteVelocity * scale)), 1, 127);
            }

            events.push_back ({ NoteEvent::Kind::NoteOn, midiNote, noteVelocity, noteStartBeat });
            events.push_back ({ NoteEvent::Kind::NoteOff, midiNote, 0, noteOffBeat });
        }

        beatCursor += stepDuration;
    }

    // Il CC di expression va emesso a prescindere dagli step: un pad di archi puo' essere
    // una sola nota tenuta per l'intero loop, e la velocity da sola non la farebbe
    // gonfiare.
    if (pattern.crescendoCurve && loopLength > 0.0)
    {
        for (int i = 0; i < crescendoResolution; ++i)
        {
            const double position = loopLength * i / crescendoResolution;
            const double amount = crescendoAmountAt (position, loopLength);
            const int value = std::clamp (
                static_cast<int> (std::lround (crescendoStartValue + (127 - crescendoStartValue) * amount)), 0, 127);

            events.push_back ({ NoteEvent::Kind::ControlChange, expressionController, value, position });
        }
    }

    return events;
}

double patternLoopLengthBeats (const PatternDefinition& pattern, double rateMultiplier)
{
    const double rate = rateMultiplier > 0.0 ? rateMultiplier : 1.0;
    const int repetitions = std::max (1, pattern.loopLength);

    double total = 0.0;
    for (float duration : pattern.rhythmGrid)
        total += duration;
    return total * rate * repetitions;
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
            // La posizione dell'evento va riportata dentro [0, loopLengthBeats): un
            // NoteOff che cade esattamente sulla fine del loop (gate pieno sull'ultimo
            // step) o oltre (swing sull'ultimo step) appartiene al passaggio successivo.
            // Senza questa riduzione non verrebbe mai emesso e la nota resterebbe appesa.
            double eventPosition = std::fmod (event.beatPosition, loopLengthBeats);
            if (eventPosition < 0.0)
                eventPosition += loopLengthBeats;

            if (eventPosition >= localStart && eventPosition < localEnd)
            {
                const double beatOffset = eventPosition - localStart;
                int sampleOffset = static_cast<int> (std::round (sampleBase + beatOffset * samplesPerBeat));
                sampleOffset = std::clamp (sampleOffset, 0, std::max (0, blockNumSamples - 1));
                result.push_back ({ event, sampleOffset });
            }
        }

        sampleBase += consumedThisPass * samplesPerBeat;
        remaining -= consumedThisPass;
        localStart = 0.0; // dopo il primo giro riparte dall'inizio del loop (wrap-around)
    }

    // A parita' di campione conta l'ordine: il CC precede tutto (una nota deve partire
    // con l'expression gia' al valore giusto) e il NoteOff precede il NoteOn, altrimenti
    // una nota ribattuta senza stacco - gate pieno, o wrap-around del loop - verrebbe
    // spenta subito dopo essere stata accesa. stable_sort perche' l'ordine fra eventi
    // altrimenti equivalenti deve restare quello di generazione: due esecuzioni della
    // stessa battuta devono produrre lo stesso identico flusso MIDI.
    const auto rank = [] (NoteEvent::Kind kind)
    {
        switch (kind)
        {
            case NoteEvent::Kind::ControlChange: return 0;
            case NoteEvent::Kind::NoteOff:       return 1;
            case NoteEvent::Kind::NoteOn:        break;
        }
        return 2;
    };

    std::stable_sort (result.begin(), result.end(),
                      [&rank] (const ScheduledEvent& a, const ScheduledEvent& b)
                      {
                          if (a.sampleOffset != b.sampleOffset)
                              return a.sampleOffset < b.sampleOffset;
                          return rank (a.event.kind) < rank (b.event.kind);
                      });

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
