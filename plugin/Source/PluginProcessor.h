#pragma once

#include "smartchord/ArpeggiatorEngine.h"
#include "smartchord/AutoplayGridState.h"
#include "smartchord/ChordBankModule.h"
#include "smartchord/LoopClock.h"
#include "smartchord/MidiOutputManager.h"
#include "smartchord/PatternLibrary.h"
#include "smartchord/VoicingEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <random>
#include <vector>

namespace smartchord
{

// AudioProcessor MIDI-only (SPEC.md sezione 1): genera solo MIDI out, mai audio.
// Aggancia ArpeggiatorEngine/MidiOutputManager all'AudioPlayHead dell'host per BPM e
// posizione PPQ (SPEC.md sezione 6) e li esegue in tempo reale in processBlock()
// (SPEC.md sezione 10, step 8).
//
// Lo stesso processor serve due varianti dello stesso plugin, distinte dal target che lo
// compila (vedi plugin/CMakeLists.txt):
//  - MIDI FX (JucePlugin_IsMidiEffect): isMidiEffect() == true, nessun bus audio, come
//    da SPEC.md. E' la forma corretta, supportata da Cubase/Reaper/Studio One/FL/Logic.
//  - strumento (VSTi): espone un'uscita audio che resta silenziosa, per gli host che non
//    ospitano i MIDI FX VST3 (Ableton Live).
//
// Lo stato condiviso con la UI (ChordBankModule, AutoplayGridState, famiglia attiva) e'
// protetto da un lock breve: la UI lo modifica raramente (interazione utente), il thread
// audio ne fa una copia di lavoro a inizio blocco, cosi' il lock non resta mai preso per
// la durata di processBlock().
class SmartChordAudioProcessor : public juce::AudioProcessor
{
public:
    SmartChordAudioProcessor();
    ~SmartChordAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

   #if ! JucePlugin_IsMidiEffect
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return JucePlugin_IsMidiEffect != 0; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- API thread-safe per la UI (message thread) --------------------------------
    void setActiveSlot (int slot);
    void setChordAt (int slot, const ChordDefinition& chord);
    void setIntensityAt (InstrumentFamily family, int chordSlot, int intensityLevel);
    void setActiveFamily (InstrumentFamily family);

    // Quando true l'arpeggiatore suona anche a trasporto fermo, usando un clock interno
    // al posto della posizione PPQ dell'host (utile per provare accordi e pattern senza
    // far girare la sessione). Di default false: il comportamento sincronizzato al
    // trasporto e' quello atteso da una DAW.
    void setFreeRunWhenStopped (bool shouldFreeRun) { freeRunWhenStopped.store (shouldFreeRun, std::memory_order_relaxed); }
    bool getFreeRunWhenStopped() const { return freeRunWhenStopped.load (std::memory_order_relaxed); }

    // Quando true il rivolto di ogni accordo viene scelto per muovere il meno possibile le
    // voci rispetto all'accordo precedente, invece di usare quello impostato a mano.
    void setVoiceLeading (bool shouldLead) { voiceLeadingEnabled.store (shouldLead, std::memory_order_relaxed); }
    bool getVoiceLeading() const { return voiceLeadingEnabled.load (std::memory_order_relaxed); }

    // true (una sola volta) se un keyswitch MIDI ha cambiato lo slot attivo da quando e'
    // stato interrogato l'ultima volta: l'editor lo usa per riallinearsi.
    bool consumeSlotChangedByMidi() { return slotChangedByMidi.exchange (false, std::memory_order_acq_rel); }

    ChordBankModule getChordBankSnapshot() const;
    AutoplayGridState getGridStateSnapshot() const;
    InstrumentFamily getActiveFamilySnapshot() const;
    const PatternLibrary& getPatternLibrary() const noexcept { return patternLibrary; }

private:
    void regenerateAudioThreadLoop (double bpm);

    // Caricata una sola volta nel costruttore, mai modificata dopo: sicura senza lock.
    PatternLibrary patternLibrary;

    // Stato autorevole, condiviso con la UI: protetto da stateLock.
    mutable juce::CriticalSection stateLock;
    ChordBankModule chordBank;
    AutoplayGridState gridState;
    InstrumentFamily activeFamily = InstrumentFamily::Guitar;

    // Copie di lavoro usate esclusivamente dal thread audio in processBlock().
    ChordBankModule audioChordBank;
    AutoplayGridState audioGridState;
    InstrumentFamily audioFamily = InstrumentFamily::Guitar;

    // Scritto dal thread audio quando un keyswitch cambia lo slot, letto dall'editor.
    std::atomic<bool> slotChangedByMidi { false };

    std::atomic<bool> freeRunWhenStopped { false };
    std::atomic<bool> voiceLeadingEnabled { true };

    // Voicing dell'accordo precedente, per il voice leading; e il generatore casuale per
    // humanizeTiming/humanizeVelocity. Usati solo dal thread audio.
    std::vector<int> previousVoicing;
    std::mt19937 humanizeRng { 0x5EED };

    MidiOutputManager midiOutputManager;

    LoopClock loopClock;
    std::vector<NoteEvent> currentLoopEvents;
    double currentLoopLengthBeats = 0.0;

    int previousActiveSlot = -1;
    InstrumentFamily previousFamily = InstrumentFamily::Piano;
    int previousIntensity = -1;
    bool havePreviousSelection = false;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmartChordAudioProcessor)
};

} // namespace smartchord
