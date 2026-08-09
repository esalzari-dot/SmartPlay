#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <BinaryData.h>

#include <cmath>

namespace smartchord
{

namespace
{
    // v1: famiglia + slot attivo + 8 accordi + griglia intensita'.
    // v2: aggiunge il flag "free run a trasporto fermo" in coda.
    // v3: aggiunge il flag "voice leading".
    // v4: aggiunge il moltiplicatore globale di velocita' dei pattern.
    // v5: aggiunge swing globale, gate globale e range d'ottava (SPEC.md sezione 8: gli
    //     stessi valori ora vivono in apvts, ma il formato binario resta lo stesso tipo di
    //     blob versionato delle versioni precedenti, per compatibilita' diretta.
    constexpr int32_t stateFormatVersion = 5;

    juce::String familyParamKey (InstrumentFamily family)
    {
        switch (family)
        {
            case InstrumentFamily::Piano:   return "piano";
            case InstrumentFamily::Bass:    return "bass";
            case InstrumentFamily::Guitar:  return "guitar";
            case InstrumentFamily::Strings: return "strings";
        }
        return "piano";
    }

    juce::String intensityParamID (InstrumentFamily family, int slot)
    {
        return "intensity_" + familyParamKey (family) + "_" + juce::String (slot + 1);
    }

    constexpr InstrumentFamily allFamilies[] = {
        InstrumentFamily::Piano, InstrumentFamily::Bass, InstrumentFamily::Guitar, InstrumentFamily::Strings
    };

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

juce::AudioProcessorValueTreeState::ParameterLayout SmartChordAudioProcessor::createParameterLayout()
{
    using namespace juce;

    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // SPEC.md sezione 8: "accordo attivo, intensita'/pattern per (accordo, famiglia),
    // rate, gate length globale, swing, octave range, instrument family attivo - tutti
    // automatizzabili dall'host".
    params.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { "activeFamily", 1 }, "Famiglia attiva",
        StringArray { "Piano", "Bass", "Guitar", "Strings" }, static_cast<int> (InstrumentFamily::Guitar)));

    params.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { "activeChordSlot", 1 }, "Accordo attivo",
        StringArray { "1", "2", "3", "4", "5", "6", "7", "8" }, 0));

    params.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { "rate", 1 }, "Rate",
        StringArray { "1/2x", "1x", "Terzine", "2x" }, static_cast<int> (PatternRate::Normal)));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "globalSwing", 1 }, "Swing globale",
        NormalisableRange<float> (0.0f, 1.0f), 0.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "globalGate", 1 }, "Gate globale",
        NormalisableRange<float> (0.25f, 1.5f), 1.0f));

    params.push_back (std::make_unique<AudioParameterInt> (
        ParameterID { "octaveRange", 1 }, "Range ottava", -2, 2, 0));

    for (auto family : allFamilies)
        for (int slot = 0; slot < numChordSlots; ++slot)
            params.push_back (std::make_unique<AudioParameterChoice> (
                ParameterID { intensityParamID (family, slot), 1 },
                "Intensita' " + familyParamKey (family) + " " + String (slot + 1),
                StringArray { "0", "1", "2", "3" }, 0));

    return { params.begin(), params.end() };
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
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout()),
      chordBankContent (makeDemoChordBank())
{
    // Risolti una volta sola: processBlock() li legge ogni blocco senza cercarli per
    // stringa e senza toccare la ValueTree (SPEC.md sezione 8, thread audio lock-free).
    activeChordSlotParam = apvts.getParameter ("activeChordSlot");

    activeFamilyRaw = apvts.getRawParameterValue ("activeFamily");
    activeChordSlotRaw = apvts.getRawParameterValue ("activeChordSlot");
    rateRaw = apvts.getRawParameterValue ("rate");
    globalSwingRaw = apvts.getRawParameterValue ("globalSwing");
    globalGateRaw = apvts.getRawParameterValue ("globalGate");
    octaveRangeRaw = apvts.getRawParameterValue ("octaveRange");

    for (auto family : allFamilies)
        for (int slot = 0; slot < numChordSlots; ++slot)
            intensityRaw[static_cast<size_t> (family)][static_cast<size_t> (slot)] =
                apvts.getRawParameterValue (intensityParamID (family, slot));
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
    heldKeyboardNotes.clear();
    keyboardChord.reset();
    loopClock.reset();
    loopPositionNormalized.store (0.0f, std::memory_order_relaxed);
}

void SmartChordAudioProcessor::releaseResources()
{
    midiOutputManager.allNotesOff();
}

void SmartChordAudioProcessor::regenerateAudioThreadLoop (const ChordDefinition& chord, InstrumentFamily family,
                                                            int intensityLevel, const SyncClock& clock,
                                                            int octaveRange)
{
    const auto* pattern = resolvePattern (patternLibrary, family, intensityLevel);
    if (pattern == nullptr)
    {
        currentLoopEvents.clear();
        currentLoopLengthBeats = 0.0;
        return;
    }

    const auto profile = getVoicingProfile (family);

    // octaveRange (SPEC.md sezione 8): trasla l'intero voicing, sommandosi all'ottava
    // impostata a mano su ogni pad invece di sostituirla.
    ChordDefinition shiftedChord = chord;
    shiftedChord.octaveOffset += octaveRange;

    const auto voicing = voiceLeadingEnabled.load (std::memory_order_relaxed)
        ? voiceChordWithLeading (shiftedChord, profile, previousVoicing)
        : voiceChord (shiftedChord, profile);

    previousVoicing = voicing.notes;

    currentLoopEvents = generateSequence (*pattern, voicing, clock, &humanizeRng);
    currentLoopLengthBeats = patternLoopLengthBeats (*pattern, clock.rateMultiplier);
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

        if (message.isNoteOn())
        {
            const int slot = keyswitchSlotForNote (message.getNoteNumber());
            if (slot >= 0)
            {
                keyswitchSlot = slot;
                continue;
            }

            // Sopra la fascia dei keyswitch: nota d'accordo suonata sulla tastiera.
            heldKeyboardNotes.push_back (message.getNoteNumber());
        }
        else if (message.isNoteOff())
        {
            const auto it = std::find (heldKeyboardNotes.begin(), heldKeyboardNotes.end(), message.getNoteNumber());
            if (it != heldKeyboardNotes.end())
                heldKeyboardNotes.erase (it);
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            heldKeyboardNotes.clear();
        }
    }

    midiMessages.clear();

    if (numSamples <= 0)
        return;

    TransportState transport;

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpmOpt = position->getBpm())
                transport.bpm = *bpmOpt;

            if (auto ppqOpt = position->getPpqPosition())
            {
                transport.hostPpq = *ppqOpt;
                transport.hostProvidesPpq = true;
            }

            transport.isPlaying = position->getIsPlaying();
        }
    }

    // Un BPM non valido riportato dall'host renderebbe NaN tutta la temporizzazione.
    if (! (transport.bpm > 0.0))
        transport.bpm = 120.0;

    const double bpm = transport.bpm;

    // Contenuto dei pad: copia breve sotto lock (SPEC.md sezione 8). Non dipende dallo
    // slot attivo, che ora e' un parametro apvts letto piu' sotto senza lock.
    {
        const juce::ScopedLock lock (chordContentLock);
        audioChordBankContent = chordBankContent;
    }

    // Il keyswitch scrive direttamente il parametro "accordo attivo": e' la stessa
    // grandezza automatizzabile da SPEC.md sezione 8, la nota di keyswitch e' solo
    // un'altra sorgente che la imposta. setValueNotifyingHost() e' pensato apposta per
    // essere chiamato dal thread audio quando e' il processor stesso a decidere il nuovo
    // valore: aggiorna in modo sincrono l'atomico letto da getRawParameterValue(), senza
    // toccare la ValueTree (quella sincronizzazione, per la UI e per il salvataggio di
    // stato, resta sul thread messaggi). Cosi' la scelta sopravvive alla chiusura
    // dell'editor e finisce nello stato salvato dall'host, come prima.
    if (keyswitchSlot >= 0)
    {
        activeChordSlotParam->setValueNotifyingHost (activeChordSlotParam->convertTo0to1 (static_cast<float> (keyswitchSlot)));
        slotChangedByMidi.store (true, std::memory_order_release);
    }

    const auto family = static_cast<InstrumentFamily> (juce::jlimit (0, 3,
        static_cast<int> (std::lround (activeFamilyRaw->load (std::memory_order_relaxed)))));

    // Se e' appena arrivato un keyswitch si usa subito il suo valore: leggere di nuovo
    // l'atomico non sarebbe scorretto (setValueNotifyingHost lo aggiorna in modo
    // sincrono) ma cosi' il comportamento non dipende da quel dettaglio implementativo.
    const int activeSlot = keyswitchSlot >= 0 ? keyswitchSlot
        : juce::jlimit (0, numChordBankSlots - 1,
            static_cast<int> (std::lround (activeChordSlotRaw->load (std::memory_order_relaxed))));

    const int intensityLevel = juce::jlimit (minIntensityLevel, maxIntensityLevel,
        static_cast<int> (std::lround (intensityRaw[static_cast<size_t> (family)][static_cast<size_t> (activeSlot)]
                                            ->load (std::memory_order_relaxed))));

    // L'accordo suonato sulla tastiera, finche' resta premuto, prende il posto di quello
    // selezionato sul banco: e' l'alternativa agli 8 pad per chi preferisce suonare le
    // armonie invece di sceglierle.
    keyboardChord.reset();
    if (chordFromKeyboard.load (std::memory_order_relaxed) && ! heldKeyboardNotes.empty())
        keyboardChord = recognizeChord (heldKeyboardNotes);

    const ChordDefinition padChord = audioChordBankContent.getChord (activeSlot);
    const ChordDefinition currentChord = keyboardChord.has_value() ? *keyboardChord : padChord;

    const auto currentRate = static_cast<PatternRate> (juce::jlimit (0, 3,
        static_cast<int> (std::lround (rateRaw->load (std::memory_order_relaxed)))));
    const float currentSwing = juce::jlimit (0.0f, 1.0f, globalSwingRaw->load (std::memory_order_relaxed));
    const float currentGate = globalGateRaw->load (std::memory_order_relaxed);
    const int currentOctaveRange = juce::jlimit (-2, 2,
        static_cast<int> (std::lround (octaveRangeRaw->load (std::memory_order_relaxed))));

    const bool selectionChanged = ! havePreviousSelection
                                || activeSlot != previousActiveSlot
                                || family != previousFamily
                                || intensityLevel != previousIntensity
                                || currentRate != previousRate
                                || currentChord != previousChord
                                || currentSwing != previousSwing
                                || currentGate != previousGate
                                || currentOctaveRange != previousOctaveRange;

    const bool freeRun = freeRunWhenStopped.load (std::memory_order_relaxed);

    if (selectionChanged)
    {
        // All-notes-off / panic al cambio di accordo o intensita' (SPEC.md sezione 7).
        for (const auto& off : midiOutputManager.allNotesOff())
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, off.midiNote), 0);

        const SyncClock clock { bpm, static_cast<double> (currentSwing),
                                 rateMultiplierFor (currentRate), static_cast<double> (currentGate) };
        regenerateAudioThreadLoop (currentChord, family, intensityLevel, clock, currentOctaveRange);

        // Fa ripartire il pattern dall'inizio, indipendentemente dalla posizione
        // assoluta dell'host: risponde subito al cambio, invece di "entrare" a meta'.
        loopClock.restartLoop();

        previousActiveSlot = activeSlot;
        previousFamily = family;
        previousIntensity = intensityLevel;
        previousRate = currentRate;
        previousChord = currentChord;
        previousSwing = currentSwing;
        previousGate = currentGate;
        previousOctaveRange = currentOctaveRange;
        havePreviousSelection = true;
    }

    const double samplesPerBeat = (60.0 / bpm) * currentSampleRate;
    const double blockLengthBeats = numSamples / samplesPerBeat;

    // Il clock va fatto avanzare a ogni blocco, anche quando non si suona: e' lui a
    // tenere il conto della fase e a ri-ancorarla ai cambi di sorgente del tempo.
    const auto frame = loopClock.advance (transport, freeRun, blockLengthBeats);

    // Posizione normalizzata nel loop, per il playhead sulla UI (SPEC.md sezione 9):
    // stessa riduzione modulo la lunghezza del loop usata da scheduleEventsInWindow, cosi'
    // il segno sulla griglia e il MIDI generato restano coerenti fra loro.
    if (frame.shouldPlay && currentLoopLengthBeats > 0.0)
    {
        double local = std::fmod (frame.loopPosition, currentLoopLengthBeats);
        if (local < 0.0)
            local += currentLoopLengthBeats;

        loopPositionNormalized.store (static_cast<float> (local / currentLoopLengthBeats), std::memory_order_relaxed);
    }
    else
    {
        loopPositionNormalized.store (0.0f, std::memory_order_relaxed);
    }

    if (! frame.shouldPlay || currentLoopEvents.empty() || currentLoopLengthBeats <= 0.0)
    {
        // Allo stop le note ancora suonanti vanno chiuse, altrimenti restano appese
        // nello strumento a valle (SPEC.md sezione 7).
        for (const auto& off : midiOutputManager.allNotesOff())
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, off.midiNote), 0);

        return;
    }

    const auto scheduled = scheduleEventsInWindow (currentLoopEvents, currentLoopLengthBeats,
                                                    frame.loopPosition, blockLengthBeats,
                                                    samplesPerBeat, numSamples);

    for (const auto& s : scheduled)
    {
        if (s.event.kind == NoteEvent::Kind::ControlChange)
        {
            midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, s.event.midiNote, s.event.velocity),
                                    s.sampleOffset);
            continue;
        }

        if (s.event.kind == NoteEvent::Kind::NoteOn)
        {
            midiMessages.addEvent (juce::MidiMessage::noteOn (1, s.event.midiNote, static_cast<juce::uint8> (s.event.velocity)),
                                    s.sampleOffset);
        }
        else if (! midiOutputManager.isNoteActive (s.event.midiNote))
        {
            // Il NoteOff dell'ultimo step cade sull'inizio del passaggio successivo:
            // al primo giro si riferisce quindi a una nota mai suonata. Emetterlo
            // sporcherebbe il MIDI registrato con eventi senza corrispondenza.
            continue;
        }
        else
        {
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, s.event.midiNote), s.sampleOffset);
        }

        midiOutputManager.handleEvent (s.event);
    }
}

juce::AudioProcessorEditor* SmartChordAudioProcessor::createEditor()
{
    return new SmartChordAudioProcessorEditor (*this);
}

namespace
{
    // setValueNotifyingHost() vuole un valore normalizzato [0,1]; le API pubbliche
    // ragionano invece in indici/valori reali (slot 0-7, intensita' 0-3, ecc.), quindi
    // ogni setter passa da qui.
    void setParamValue (juce::RangedAudioParameter& param, float realValue)
    {
        param.setValueNotifyingHost (param.convertTo0to1 (realValue));
    }

    int readParamAsInt (const std::atomic<float>* raw, int lowest, int highest)
    {
        return juce::jlimit (lowest, highest, static_cast<int> (std::lround (raw->load (std::memory_order_relaxed))));
    }
}

void SmartChordAudioProcessor::setActiveSlot (int slot)
{
    setParamValue (*activeChordSlotParam, static_cast<float> (juce::jlimit (0, numChordBankSlots - 1, slot)));
}

void SmartChordAudioProcessor::setChordAt (int slot, const ChordDefinition& chord)
{
    const juce::ScopedLock lock (chordContentLock);
    chordBankContent.setChord (slot, chord);
}

void SmartChordAudioProcessor::setIntensityAt (InstrumentFamily family, int chordSlot, int intensityLevel)
{
    if (auto* param = apvts.getParameter (intensityParamID (family, chordSlot)))
        setParamValue (*param, static_cast<float> (juce::jlimit (minIntensityLevel, maxIntensityLevel, intensityLevel)));
}

void SmartChordAudioProcessor::setActiveFamily (InstrumentFamily family)
{
    if (auto* param = apvts.getParameter ("activeFamily"))
        setParamValue (*param, static_cast<float> (juce::jlimit (0, 3, static_cast<int> (family))));
}

void SmartChordAudioProcessor::setPatternRate (PatternRate rate)
{
    if (auto* param = apvts.getParameter ("rate"))
        setParamValue (*param, static_cast<float> (juce::jlimit (0, 3, static_cast<int> (rate))));
}

PatternRate SmartChordAudioProcessor::getPatternRate() const
{
    return static_cast<PatternRate> (readParamAsInt (rateRaw, 0, 3));
}

void SmartChordAudioProcessor::setGlobalSwing (float amount01)
{
    if (auto* param = apvts.getParameter ("globalSwing"))
        setParamValue (*param, juce::jlimit (0.0f, 1.0f, amount01));
}

float SmartChordAudioProcessor::getGlobalSwing() const
{
    return juce::jlimit (0.0f, 1.0f, globalSwingRaw->load (std::memory_order_relaxed));
}

void SmartChordAudioProcessor::setGlobalGateLength (float multiplier)
{
    if (auto* param = apvts.getParameter ("globalGate"))
        setParamValue (*param, juce::jlimit (0.25f, 1.5f, multiplier));
}

float SmartChordAudioProcessor::getGlobalGateLength() const
{
    return globalGateRaw->load (std::memory_order_relaxed);
}

void SmartChordAudioProcessor::setOctaveRange (int octaves)
{
    if (auto* param = apvts.getParameter ("octaveRange"))
        setParamValue (*param, static_cast<float> (juce::jlimit (-2, 2, octaves)));
}

int SmartChordAudioProcessor::getOctaveRange() const
{
    return readParamAsInt (octaveRangeRaw, -2, 2);
}

int SmartChordAudioProcessor::getActiveSlotSnapshot() const
{
    return readParamAsInt (activeChordSlotRaw, 0, numChordBankSlots - 1);
}

ChordBankModule SmartChordAudioProcessor::getChordBankSnapshot() const
{
    ChordBankModule snapshot;
    {
        const juce::ScopedLock lock (chordContentLock);
        snapshot = chordBankContent;
    }
    snapshot.setActiveSlot (getActiveSlotSnapshot());
    return snapshot;
}

AutoplayGridState SmartChordAudioProcessor::getGridStateSnapshot() const
{
    AutoplayGridState state;
    for (auto family : allFamilies)
        for (int slot = 0; slot < numChordSlots; ++slot)
            state.setIntensity (family, slot,
                readParamAsInt (intensityRaw[static_cast<size_t> (family)][static_cast<size_t> (slot)],
                                minIntensityLevel, maxIntensityLevel));
    return state;
}

InstrumentFamily SmartChordAudioProcessor::getActiveFamilySnapshot() const
{
    return static_cast<InstrumentFamily> (readParamAsInt (activeFamilyRaw, 0, 3));
}

void SmartChordAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, false);
    stream.writeInt (stateFormatVersion);
    stream.writeInt (static_cast<int> (getActiveFamilySnapshot()));
    stream.writeInt (getActiveSlotSnapshot());

    {
        const juce::ScopedLock lock (chordContentLock);
        for (int slot = 0; slot < numChordBankSlots; ++slot)
        {
            const auto& chord = chordBankContent.getChord (slot);
            stream.writeInt (chord.rootSemitone);
            stream.writeInt (static_cast<int> (chord.quality));
            stream.writeInt (chord.inversion);
            stream.writeInt (chord.octaveOffset);
        }
    }

    for (auto family : allFamilies)
        for (int slot = 0; slot < numChordSlots; ++slot)
            stream.writeInt (readParamAsInt (intensityRaw[static_cast<size_t> (family)][static_cast<size_t> (slot)],
                                              minIntensityLevel, maxIntensityLevel));

    stream.writeBool (freeRunWhenStopped.load (std::memory_order_relaxed));
    stream.writeBool (voiceLeadingEnabled.load (std::memory_order_relaxed));
    stream.writeInt (static_cast<int> (getPatternRate()));
    stream.writeBool (chordFromKeyboard.load (std::memory_order_relaxed));
    stream.writeFloat (getGlobalSwing());
    stream.writeFloat (getGlobalGateLength());
    stream.writeInt (getOctaveRange());
}

void SmartChordAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);

    const int version = stream.readInt();
    if (version < 1 || version > stateFormatVersion)
        return; // formato sconosciuto: mantiene lo stato di default piuttosto che corromperlo

    setActiveFamily (static_cast<InstrumentFamily> (stream.readInt()));
    const int activeSlot = stream.readInt();

    {
        const juce::ScopedLock lock (chordContentLock);
        for (int slot = 0; slot < numChordBankSlots; ++slot)
        {
            ChordDefinition chord;
            chord.rootSemitone = stream.readInt();
            chord.quality = static_cast<ChordQuality> (stream.readInt());
            chord.inversion = stream.readInt();
            chord.octaveOffset = stream.readInt();
            chordBankContent.setChord (slot, chord);
        }
    }
    setActiveSlot (activeSlot);

    for (auto family : allFamilies)
        for (int slot = 0; slot < numChordSlots; ++slot)
            setIntensityAt (family, slot, stream.readInt());

    // Assenti negli stati salvati dalle versioni precedenti: restano al default.
    if (version >= 2)
        freeRunWhenStopped.store (stream.readBool(), std::memory_order_relaxed);

    if (version >= 3)
        voiceLeadingEnabled.store (stream.readBool(), std::memory_order_relaxed);

    if (version >= 4)
    {
        const int storedRate = stream.readInt();
        if (storedRate >= static_cast<int> (PatternRate::Half) && storedRate <= static_cast<int> (PatternRate::Double))
            setPatternRate (static_cast<PatternRate> (storedRate));

        chordFromKeyboard.store (stream.readBool(), std::memory_order_relaxed);
    }

    if (version >= 5)
    {
        setGlobalSwing (stream.readFloat());
        setGlobalGateLength (stream.readFloat());
        setOctaveRange (stream.readInt());
    }
}

} // namespace smartchord

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new smartchord::SmartChordAudioProcessor();
}
