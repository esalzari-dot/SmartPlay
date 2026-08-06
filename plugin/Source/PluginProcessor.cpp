#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <BinaryData.h>

namespace smartchord
{

namespace
{
    // v1: famiglia + slot attivo + 8 accordi + griglia intensita'.
    // v2: aggiunge il flag "free run a trasporto fermo" in coda.
    constexpr int32_t stateFormatVersion = 2;

    std::string embeddedPatternJson()
    {
        return std::string (BinaryData::patterns_json, static_cast<size_t> (BinaryData::patterns_jsonSize));
    }

    // File modificabile dall'utente. Sta nei Documenti (non in AppData) perche' e' fatto
    // per essere aperto e modificato a mano.
    juce::File userPatternFile()
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                 .getChildFile ("SmartChordArp")
                 .getChildFile ("patterns.json");
    }

    // SPEC.md sezione 5.4 vuole il dataset espandibile "senza ricompilare": i pattern di
    // default sono embeddati nel binario (un plugin distribuito non puo' dipendere da un
    // percorso della macchina che lo ha compilato), ma al primo avvio vengono scritti su
    // un file nei Documenti dell'utente, che da quel momento ha la precedenza.
    //
    // Nessun errore qui deve impedire il caricamento del plugin nell'host: un file utente
    // illeggibile ricade sui pattern embeddati, e un dataset embeddato illeggibile ricade
    // su una libreria vuota.
    PatternLibrary loadPatternLibrary()
    {
        const auto file = userPatternFile();

        if (file.existsAsFile())
        {
            try
            {
                auto library = PatternLibrary::fromJson (file.loadFileAsString().toStdString());
                if (! library.getAllPatterns().empty())
                    return library;
            }
            catch (const std::exception&)
            {
                // JSON dell'utente non valido: si prosegue con i default embeddati.
            }
        }
        else if (file.getParentDirectory().createDirectory())
        {
            file.replaceWithText (embeddedPatternJson());
        }

        try
        {
            return PatternLibrary::fromJson (embeddedPatternJson());
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
      patternLibrary (loadPatternLibrary()),
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

    // Keyswitch: una nota nella fascia dedicata seleziona lo slot accordo (SPEC.md
    // sezione 3). Va letto prima di svuotare il buffer, che poi viene riempito solo con
    // gli eventi generati - il MIDI in ingresso non passa oltre.
    int keyswitchSlot = -1;
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        if (! message.isNoteOn())
            continue;

        const int slot = keyswitchSlotForNote (message.getNoteNumber());
        if (slot >= 0)
            keyswitchSlot = slot;
    }

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

        // Il keyswitch aggiorna lo stato autorevole, cosi' la scelta sopravvive alla
        // chiusura dell'editor e finisce nello stato salvato dall'host.
        if (keyswitchSlot >= 0)
            chordBank.setActiveSlot (keyswitchSlot);

        audioChordBank = chordBank;
        audioGridState = gridState;
        audioFamily = activeFamily;
    }

    if (keyswitchSlot >= 0)
        slotChangedByMidi.store (true, std::memory_order_release);

    const int activeSlot = audioChordBank.getActiveSlot();
    const int currentIntensity = audioGridState.getIntensity (audioFamily, activeSlot);

    const bool selectionChanged = ! havePreviousSelection
                                || activeSlot != previousActiveSlot
                                || audioFamily != previousFamily
                                || currentIntensity != previousIntensity;

    const bool freeRun = freeRunWhenStopped.load (std::memory_order_relaxed);
    const bool shouldPlay = isPlaying || freeRun;

    // Si segue la posizione dell'host finche' il trasporto gira; a trasporto fermo la
    // PPQ dell'host e' congelata, quindi in free run si passa al clock interno. Tenerlo
    // allineato mentre l'host comanda rende il passaggio continuo, senza salti di fase.
    const bool useHostPosition = hostProvidesPpq && isPlaying;
    if (useHostPosition)
        internalBeatPosition = hostPpq;

    const double rawPosition = useHostPosition ? hostPpq : internalBeatPosition;

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

    if (! shouldPlay || currentLoopEvents.empty() || currentLoopLengthBeats <= 0.0)
    {
        // Allo stop le note ancora suonanti vanno chiuse, altrimenti restano appese
        // nello strumento a valle (SPEC.md sezione 7).
        for (const auto& off : midiOutputManager.allNotesOff())
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, off.midiNote), 0);

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

    if (! useHostPosition)
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

    stream.writeBool (freeRunWhenStopped.load (std::memory_order_relaxed));
}

void SmartChordAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);

    const int version = stream.readInt();
    if (version < 1 || version > stateFormatVersion)
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

    // Assente negli stati salvati dalla v1: resta al default.
    if (version >= 2)
        freeRunWhenStopped.store (stream.readBool(), std::memory_order_relaxed);
}

} // namespace smartchord

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new smartchord::SmartChordAudioProcessor();
}
