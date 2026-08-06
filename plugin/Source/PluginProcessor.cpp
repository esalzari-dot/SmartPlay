#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <BinaryData.h>

namespace smartchord
{

namespace
{
    constexpr int32_t stateFormatVersion = 1;

    // Il dataset e' embeddato nel binario: un plugin distribuito non puo' leggere un
    // percorso dell'albero sorgente della macchina che lo ha compilato. Un dataset
    // illeggibile non deve comunque impedire il caricamento del plugin nell'host, quindi
    // in caso di errore si degrada a una libreria vuota (la griglia non risolvera'
    // pattern, ma l'editor si apre e l'host resta stabile).
    PatternLibrary loadEmbeddedPatternLibrary()
    {
        try
        {
            return PatternLibrary::fromJson (std::string (BinaryData::patterns_json,
                                                           static_cast<size_t> (BinaryData::patterns_jsonSize)));
        }
        catch (const std::exception&)
        {
            return {};
        }
    }

    // Accordi dimostrativi di default (come l'harness standalone), sovrascrivibili
    // dall'utente e comunque persistiti da getStateInformation/setStateInformation.
    ChordBankModule makeDemoChordBank()
    {
        static const ChordDefinition demoChords[numChordBankSlots] = {
            { 0, ChordQuality::Maj,  0, 0 },  // C
            { 7, ChordQuality::Maj,  0, 0 },  // G
            { 9, ChordQuality::Min,  0, 0 },  // A min
            { 5, ChordQuality::Maj,  0, 0 },  // F
            { 2, ChordQuality::Min7, 0, 0 },  // D min7
            { 4, ChordQuality::Min,  0, 0 },  // E min
            { 11, ChordQuality::Dim, 0, 0 },  // B dim
            { 0, ChordQuality::Dom7, 0, 0 },  // C7
        };

        ChordBankModule bank;
        for (int slot = 0; slot < numChordBankSlots; ++slot)
            bank.setChord (slot, demoChords[static_cast<size_t> (slot)]);
        return bank;
    }
}

SmartChordAudioProcessor::SmartChordAudioProcessor()
    : AudioProcessor (
       #if JucePlugin_IsMidiEffect
        BusesProperties() // MIDI effect puro: nessun bus audio
       #else
        // Variante strumento: un'uscita audio (sempre silenziosa) e' necessaria perche'
        // gli host che non ospitano i MIDI FX VST3 accettino il plugin.
        BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)
       #endif
      ),
      patternLibrary (loadEmbeddedPatternLibrary()),
      chordBank (makeDemoChordBank())
{
}

#if ! JucePlugin_IsMidiEffect
bool SmartChordAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    return mainOutput == juce::AudioChannelSet::mono() || mainOutput == juce::AudioChannelSet::stereo();
}
#endif

void SmartChordAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    midiOutputManager.allNotesOff();
    currentLoopEvents.clear();
    currentLoopLengthBeats = 0.0;
    havePreviousSelection = false;
    internalBeatPosition = 0.0;
    loopPhaseOffsetBeats = 0.0;
}

void SmartChordAudioProcessor::releaseResources()
{
    midiOutputManager.allNotesOff();
}

void SmartChordAudioProcessor::regenerateAudioThreadLoop (double bpm)
{
    const int activeSlot = audioChordBank.getActiveSlot();
    const int intensity = audioGridState.getIntensity (audioFamily, activeSlot);

    const auto* pattern = resolvePattern (patternLibrary, audioFamily, intensity);
    if (pattern == nullptr)
    {
        currentLoopEvents.clear();
        currentLoopLengthBeats = 0.0;
        return;
    }

    const auto voicing = voiceChord (audioChordBank.getActiveChord(), getVoicingProfile (audioFamily));
    currentLoopEvents = generateSequence (*pattern, voicing, SyncClock { bpm, 0.0 });
    currentLoopLengthBeats = patternLoopLengthBeats (*pattern);
}

void SmartChordAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear(); // MIDI effect: nessun segnale audio in uscita

    const int numSamples = buffer.getNumSamples();
    midiMessages.clear();

    if (numSamples <= 0)
        return;

    double bpm = 120.0;
    double hostPpq = 0.0;
    bool hostProvidesPpq = false;
    bool isPlaying = true;

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpmOpt = position->getBpm())
                bpm = *bpmOpt;

            if (auto ppqOpt = position->getPpqPosition())
            {
                hostPpq = *ppqOpt;
                hostProvidesPpq = true;
            }

            isPlaying = position->getIsPlaying();
        }
    }

    // Copia breve, sotto lock, dello stato condiviso con la UI verso le variabili di
    // lavoro del thread audio (SPEC.md sezione 8).
    {
        const juce::ScopedLock lock (stateLock);
        audioChordBank = chordBank;
        audioGridState = gridState;
        audioFamily = activeFamily;
    }

    const int activeSlot = audioChordBank.getActiveSlot();
    const int currentIntensity = audioGridState.getIntensity (audioFamily, activeSlot);

    const bool selectionChanged = ! havePreviousSelection
                                || activeSlot != previousActiveSlot
                                || audioFamily != previousFamily
                                || currentIntensity != previousIntensity;

    const double rawPosition = hostProvidesPpq ? hostPpq : internalBeatPosition;

    if (selectionChanged)
    {
        // All-notes-off / panic al cambio di accordo o intensita' (SPEC.md sezione 7).
        for (const auto& off : midiOutputManager.allNotesOff())
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, off.midiNote), 0);

        regenerateAudioThreadLoop (bpm);

        // Fa ripartire il pattern dall'inizio, indipendentemente dalla posizione
        // assoluta dell'host: risponde subito al cambio, invece di "entrare" a meta'.
        loopPhaseOffsetBeats = rawPosition;

        previousActiveSlot = activeSlot;
        previousFamily = audioFamily;
        previousIntensity = currentIntensity;
        havePreviousSelection = true;
    }

    if (! isPlaying || currentLoopEvents.empty() || currentLoopLengthBeats <= 0.0)
    {
        if (! hostProvidesPpq)
            internalBeatPosition += 0.0; // trasporto fermo: non avanzare
        return;
    }

    const double samplesPerBeat = (60.0 / bpm) * currentSampleRate;
    const double blockLengthBeats = numSamples / samplesPerBeat;
    const double windowStartBeat = rawPosition - loopPhaseOffsetBeats;

    const auto scheduled = scheduleEventsInWindow (currentLoopEvents, currentLoopLengthBeats,
                                                    windowStartBeat, blockLengthBeats,
                                                    samplesPerBeat, numSamples);

    for (const auto& s : scheduled)
    {
        if (s.event.kind == NoteEvent::Kind::NoteOn)
            midiMessages.addEvent (juce::MidiMessage::noteOn (1, s.event.midiNote, static_cast<juce::uint8> (s.event.velocity)),
                                    s.sampleOffset);
        else
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, s.event.midiNote), s.sampleOffset);

        midiOutputManager.handleEvent (s.event);
    }

    if (! hostProvidesPpq)
        internalBeatPosition += blockLengthBeats;
}

juce::AudioProcessorEditor* SmartChordAudioProcessor::createEditor()
{
    return new SmartChordAudioProcessorEditor (*this);
}

void SmartChordAudioProcessor::setActiveSlot (int slot)
{
    const juce::ScopedLock lock (stateLock);
    chordBank.setActiveSlot (slot);
}

void SmartChordAudioProcessor::setChordAt (int slot, const ChordDefinition& chord)
{
    const juce::ScopedLock lock (stateLock);
    chordBank.setChord (slot, chord);
}

void SmartChordAudioProcessor::setIntensityAt (InstrumentFamily family, int chordSlot, int intensityLevel)
{
    const juce::ScopedLock lock (stateLock);
    gridState.setIntensity (family, chordSlot, intensityLevel);
}

void SmartChordAudioProcessor::setActiveFamily (InstrumentFamily family)
{
    const juce::ScopedLock lock (stateLock);
    activeFamily = family;
}

ChordBankModule SmartChordAudioProcessor::getChordBankSnapshot() const
{
    const juce::ScopedLock lock (stateLock);
    return chordBank;
}

AutoplayGridState SmartChordAudioProcessor::getGridStateSnapshot() const
{
    const juce::ScopedLock lock (stateLock);
    return gridState;
}

InstrumentFamily SmartChordAudioProcessor::getActiveFamilySnapshot() const
{
    const juce::ScopedLock lock (stateLock);
    return activeFamily;
}

void SmartChordAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const juce::ScopedLock lock (stateLock);

    juce::MemoryOutputStream stream (destData, false);
    stream.writeInt (stateFormatVersion);
    stream.writeInt (static_cast<int> (activeFamily));
    stream.writeInt (chordBank.getActiveSlot());

    for (int slot = 0; slot < numChordBankSlots; ++slot)
    {
        const auto& chord = chordBank.getChord (slot);
        stream.writeInt (chord.rootSemitone);
        stream.writeInt (static_cast<int> (chord.quality));
        stream.writeInt (chord.inversion);
        stream.writeInt (chord.octaveOffset);
    }

    const InstrumentFamily families[] = {
        InstrumentFamily::Piano, InstrumentFamily::Bass, InstrumentFamily::Guitar, InstrumentFamily::Strings
    };
    for (auto family : families)
        for (int slot = 0; slot < numChordSlots; ++slot)
            stream.writeInt (gridState.getIntensity (family, slot));
}

void SmartChordAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);

    if (stream.readInt() != stateFormatVersion)
        return; // formato sconosciuto: mantiene lo stato di default piuttosto che corromperlo

    const juce::ScopedLock lock (stateLock);

    activeFamily = static_cast<InstrumentFamily> (stream.readInt());
    const int activeSlot = stream.readInt();

    for (int slot = 0; slot < numChordBankSlots; ++slot)
    {
        ChordDefinition chord;
        chord.rootSemitone = stream.readInt();
        chord.quality = static_cast<ChordQuality> (stream.readInt());
        chord.inversion = stream.readInt();
        chord.octaveOffset = stream.readInt();
        chordBank.setChord (slot, chord);
    }
    chordBank.setActiveSlot (activeSlot);

    const InstrumentFamily families[] = {
        InstrumentFamily::Piano, InstrumentFamily::Bass, InstrumentFamily::Guitar, InstrumentFamily::Strings
    };
    for (auto family : families)
        for (int slot = 0; slot < numChordSlots; ++slot)
            gridState.setIntensity (family, slot, stream.readInt());
}

} // namespace smartchord

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new smartchord::SmartChordAudioProcessor();
}
