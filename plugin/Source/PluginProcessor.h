#pragma once

#include "smartchord/ArpeggiatorEngine.h"
#include "smartchord/AutoplayGridState.h"
#include "smartchord/ChordBankModule.h"
#include "smartchord/ChordBankPresets.h"
#include "smartchord/ChordRecognizer.h"
#include "smartchord/LoopClock.h"
#include "smartchord/MidiOutputManager.h"
#include "smartchord/PatternLibrary.h"
#include "smartchord/PlayStripEngine.h"
#include "smartchord/VoicingEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#if ! JucePlugin_IsMidiEffect
#include "PreviewSynth.h"
#endif

#include <array>
#include <atomic>
#include <optional>
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
// Stato condiviso con la UI (SPEC.md sezione 8): tutto cio' che ha senso automatizzare
// dall'host - accordo attivo, intensita' per (accordo, famiglia), rate, swing globale,
// gate globale, range d'ottava, famiglia attiva - vive in AudioProcessorValueTreeState,
// letto in processBlock() come atomici semplici (nessun lock, nessuna allocazione). Resta
// dietro un lock breve solo il contenuto dei pad (le 8 ChordDefinition): SPEC.md non lo
// richiede automatizzabile, e' modificato solo dal menu di modifica accordo (interazione
// utente rara), quindi il lock non e' mai conteso dal thread audio per piu' di una copia
// di 8 struct.
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
    // SPEC.md sezione 8: passano tutte attraverso apvts, quindi ogni valore e' anche
    // automatizzabile dall'host, non solo impostabile da UI.
    void setActiveSlot (int slot);
    void setChordAt (int slot, const ChordDefinition& chord);
    void setIntensityAt (InstrumentFamily family, int chordSlot, int intensityLevel);
    void setActiveFamily (InstrumentFamily family);
    void setPatternRate (PatternRate rate);
    PatternRate getPatternRate() const;

    // Swing globale (0-1) e moltiplicatore di gate globale, che si sommano/combinano a
    // quelli del pattern attivo; range d'ottava (-2..+2) che trasla l'intero voicing.
    void setGlobalSwing (float amount01);
    float getGlobalSwing() const;
    void setGlobalGateLength (float multiplier);
    float getGlobalGateLength() const;
    void setOctaveRange (int octaves);
    int getOctaveRange() const;

    // Quando true l'arpeggiatore suona anche a trasporto fermo, usando un clock interno
    // al posto della posizione PPQ dell'host (utile per provare accordi e pattern senza
    // far girare la sessione). Di default false: il comportamento sincronizzato al
    // trasporto e' quello atteso da una DAW. Non fa parte della lista automatizzabile di
    // SPEC.md sezione 8 (e' una preferenza d'uso, non un parametro musicale), quindi resta
    // un semplice atomico invece che un parametro apvts.
    void setFreeRunWhenStopped (bool shouldFreeRun) { freeRunWhenStopped.store (shouldFreeRun, std::memory_order_relaxed); }
    bool getFreeRunWhenStopped() const { return freeRunWhenStopped.load (std::memory_order_relaxed); }

    // Quando true il rivolto di ogni accordo viene scelto per muovere il meno possibile le
    // voci rispetto all'accordo precedente, invece di usare quello impostato a mano.
    void setVoiceLeading (bool shouldLead) { voiceLeadingEnabled.store (shouldLead, std::memory_order_relaxed); }
    bool getVoiceLeading() const { return voiceLeadingEnabled.load (std::memory_order_relaxed); }

    // Quando true, le note suonate sopra la fascia dei keyswitch vengono riconosciute come
    // accordo e prendono il posto di quello selezionato sul banco, finche' restano premute.
    void setChordFromKeyboard (bool shouldRecognize) { chordFromKeyboard.store (shouldRecognize, std::memory_order_relaxed); }
    bool getChordFromKeyboard() const { return chordFromKeyboard.load (std::memory_order_relaxed); }

    // Quando true, un cambio di accordo/intensita'/famiglia/rate mentre l'arpeggiatore sta
    // gia' suonando non scatta subito: resta in sospeso e viene applicato al prossimo giro
    // del loop corrente, cosi' non taglia una nota o uno strum a meta'. A trasporto fermo
    // (o se non c'era nulla in corso) si applica comunque subito: non avrebbe senso
    // aspettare un loop che non sta suonando. Di default false, come freeRun/voiceLeading:
    // e' una preferenza d'uso, non un parametro musicale da automatizzare.
    void setQuantizeChordSwitch (bool shouldQuantize) { quantizeChordSwitch.store (shouldQuantize, std::memory_order_relaxed); }
    bool getQuantizeChordSwitch() const { return quantizeChordSwitch.load (std::memory_order_relaxed); }

    // Quando true (default) applica humanizeTiming/humanizeVelocity dove il pattern attivo
    // li prevede. Disattivandolo la sequenza generata resta deterministica anche per quei
    // pattern, indipendentemente da cosa dice il loro JSON.
    void setHumanizeEnabled (bool shouldHumanize) { humanizeEnabled.store (shouldHumanize, std::memory_order_relaxed); }
    bool getHumanizeEnabled() const { return humanizeEnabled.load (std::memory_order_relaxed); }

    // Modalita' Play (SPEC.md sezione 5.5): quando true, processBlock() non genera piu' il
    // pattern automatico e reagisce solo ai gesti sulla barra (push*Gesture, sotto) - le
    // due modalita' non suonano mai insieme, come in GarageBand passare da Autoplay a Play
    // interrompe il pattern automatico. Preferenza d'uso come le altre sopra, non un
    // parametro apvts.
    void setPlayModeEnabled (bool shouldEnablePlayMode) { playModeEnabled.store (shouldEnablePlayMode, std::memory_order_relaxed); }
    bool getPlayModeEnabled() const { return playModeEnabled.load (std::memory_order_relaxed); }

    // Chiamate dal thread messaggi (UI) quando l'utente interagisce con la barra Play per
    // lo slot dato. Lock-free: accodano l'evento in stripGestureFifo, il thread audio lo
    // consuma nel prossimo processBlock() (drenato comunque anche a modo Play spento, per
    // non lasciare eventi rimasti in coda a sorprendere una riattivazione successiva).
    void pushStripNotchGesture (int chordSlot, StripGesturePhase phase, float position);
    void pushStripChordGesture (int chordSlot, bool down);

    // true (una sola volta) se un keyswitch MIDI ha cambiato lo slot attivo da quando e'
    // stato interrogato l'ultima volta: l'editor lo usa per riallinearsi.
    bool consumeSlotChangedByMidi() { return slotChangedByMidi.exchange (false, std::memory_order_acq_rel); }

    int getActiveSlotSnapshot() const;
    ChordBankModule getChordBankSnapshot() const;
    AutoplayGridState getGridStateSnapshot() const;
    InstrumentFamily getActiveFamilySnapshot() const;

    // Posizione normalizzata (0-1) nel loop attualmente in esecuzione, aggiornata dal
    // thread audio a ogni blocco: e' cio' che disegna il playhead sulla UI (SPEC.md
    // sezione 9). 0 quando non sta suonando nulla.
    float getLoopPositionSnapshot() const { return loopPositionNormalized.load (std::memory_order_relaxed); }

    // Preset del banco accordi (SPEC.md non li tratta, ma il meccanismo e' lo stesso di
    // ChordProgressions): salvati come file su disco nei Documenti dell'utente, cosi'
    // sopravvivono a una sessione e si possono riusare in un altro progetto. Solo thread
    // messaggi, mai toccati da processBlock().
    juce::StringArray getChordBankPresetNames() const;
    bool findChordBankPreset (const juce::String& name, ChordBankPreset& outPreset) const;
    void saveChordBankPreset (const juce::String& name, const ChordBankModule& bank);
    void deleteChordBankPreset (const juce::String& name);

    const PatternLibrary& getPatternLibrary() const noexcept { return patternLibrary; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void regenerateAudioThreadLoop (const ChordDefinition& chord, InstrumentFamily family,
                                     int intensityLevel, const SyncClock& clock, int octaveRange);

    // Un gesto sulla barra Play in attesa di essere consumato dal thread audio (SPEC.md
    // sezione 5.5). isChordGesture distingue il tocco sul nome dell'accordo (accordo
    // completo, position/phase Move ignorati) da un tocco/trascinamento su una tacca.
    struct QueuedStripGesture
    {
        int chordSlot = 0;
        bool isChordGesture = false;
        StripGesturePhase phase = StripGesturePhase::Down;
        float position = 0.0f;
        double timestampSeconds = 0.0;
    };

    void pushStripGesture (const QueuedStripGesture& gesture);

    // Svuota stripGestureFifo. Se playModeActive e' true traduce ogni gesto in MIDI
    // (tramite handleStripGesture); altrimenti li scarta - evita che gesti rimasti in coda
    // da quando il modo Play era attivo scattino piu' tardi alla riattivazione.
    void pumpStripGestures (InstrumentFamily family, juce::MidiBuffer& midiMessages, bool playModeActive);
    void handleStripGesture (const QueuedStripGesture& gesture, InstrumentFamily family, juce::MidiBuffer& midiMessages);

    // Caricata una sola volta nel costruttore, mai modificata dopo: sicura senza lock.
    PatternLibrary patternLibrary;

    // Preset del banco accordi: caricati una volta nel costruttore, modificati solo dal
    // thread messaggi (salva/elimina passano da qui e riscrivono il file ogni volta).
    // Mai letti dal thread audio, quindi non serve alcuna sincronizzazione.
    std::vector<ChordBankPreset> chordBankPresets;

    // I parametri di SPEC.md sezione 8. apvts.getRawParameterValue() restituisce un
    // std::atomic<float>* aggiornato in modo sincrono e lock-free a ogni cambiamento, sia
    // che arrivi dalla UI sia che arrivi dall'automazione dell'host: e' il meccanismo che
    // rende il resto del thread audio libero da lock.
    juce::AudioProcessorValueTreeState apvts;

    // Puntatori cache verso i parametri: risolti una volta nel costruttore invece che
    // per stringa a ogni blocco. Di proprieta' di apvts, validi per tutta la vita del
    // processor.
    juce::RangedAudioParameter* activeChordSlotParam = nullptr;
    std::atomic<float>* activeFamilyRaw = nullptr;
    std::atomic<float>* activeChordSlotRaw = nullptr;
    std::atomic<float>* rateRaw = nullptr;
    std::atomic<float>* globalSwingRaw = nullptr;
    std::atomic<float>* globalGateRaw = nullptr;
    std::atomic<float>* octaveRangeRaw = nullptr;
    std::array<std::array<std::atomic<float>*, numChordSlots>, 4> intensityRaw {};

    // Contenuto degli 8 pad (SPEC.md sezione 3): non fa parte della lista automatizzabile
    // di SPEC.md sezione 8, quindi resta fuori da apvts. Protetto da un lock breve: la UI
    // lo modifica solo tramite il menu di modifica accordo (interazione rara), il thread
    // audio ne fa una copia di lavoro a inizio blocco.
    mutable juce::CriticalSection chordContentLock;
    ChordBankModule chordBankContent;
    ChordBankModule audioChordBankContent;

    // Scritto dal thread audio quando un keyswitch cambia lo slot, letto dall'editor.
    std::atomic<bool> slotChangedByMidi { false };

    std::atomic<bool> freeRunWhenStopped { false };
    std::atomic<bool> voiceLeadingEnabled { true };
    std::atomic<bool> chordFromKeyboard { false };
    std::atomic<bool> quantizeChordSwitch { false };
    std::atomic<bool> humanizeEnabled { true };
    std::atomic<bool> playModeEnabled { false };

    // Coda lock-free UI -> thread audio per i gesti sulla barra Play (SPEC.md sezione 5.5,
    // produttore singolo/consumatore singolo: la UI scrive da mouseDown/Drag/Up, solo
    // processBlock() legge). 256 e' ampiamente sufficiente per i gesti di un utente umano
    // fra due blocchi audio consecutivi.
    static constexpr int stripGestureQueueCapacity = 256;
    juce::AbstractFifo stripGestureFifo { stripGestureQueueCapacity };
    std::array<QueuedStripGesture, stripGestureQueueCapacity> stripGestureBuffer;

    // Un solo engine (non uno per slot): con un solo mouse puo' esserci al massimo un
    // gesto in corso alla volta, quindi le sue note vengono impostate di nuovo a ogni
    // nuovo Down (vedi handleStripGesture). Le note "accese" dalla modalita' Play (sia
    // dalle tacche sia dal tocco sul nome dell'accordo) sono tracciate a parte da
    // MidiOutputManager - che segue solo il pattern automatico - per poterle chiudere in
    // panic se il modo Play si disattiva o il plugin si ferma a meta' gesto.
    PlayStripEngine playStripEngine;
    std::vector<int> playStripActiveNotes;

    // Note attualmente premute sulla tastiera, sopra la fascia dei keyswitch. Usato solo
    // dal thread audio.
    std::vector<int> heldKeyboardNotes;
    std::optional<ChordDefinition> keyboardChord;

    // Voicing dell'accordo precedente, per il voice leading; e il generatore casuale per
    // humanizeTiming/humanizeVelocity. Usati solo dal thread audio.
    std::vector<int> previousVoicing;
    std::mt19937 humanizeRng { 0x5EED };

    MidiOutputManager midiOutputManager;

   #if ! JucePlugin_IsMidiEffect
    // Sente il MIDI generato quando il binario gira come vero standalone (vedi
    // PreviewSynth.h e processBlock): dentro una DAW SmartPlay Inst resta silenzioso
    // come prima, per compatibilita' con host come Ableton Live.
    PreviewSynth previewSynth;
   #endif

    LoopClock loopClock;
    std::vector<NoteEvent> currentLoopEvents;
    double currentLoopLengthBeats = 0.0;

    std::atomic<float> loopPositionNormalized { 0.0f };

    // Cambio in sospeso, quando quantizeChordSwitch e' attivo e il loop sta gia' suonando:
    // i valori "obiettivo" restano qui finche' il loop non ricomincia un giro (vedi
    // processBlock). Coalizza automaticamente cambi rapidi in sequenza: se ne arriva uno
    // nuovo prima che quello sospeso venga applicato, lo sostituisce.
    struct PendingSelection
    {
        int activeSlot = 0;
        InstrumentFamily family = InstrumentFamily::Piano;
        int intensityLevel = 0;
        ChordDefinition chord;
        PatternRate rate = PatternRate::Normal;
        float swing = 0.0f;
        float gate = 1.0f;
        int octaveRange = 0;
    };
    PendingSelection pendingSelection;
    bool hasPendingChange = false;

    // Rilevazione del giro di loop per il cambio in sospeso: il playhead normalizzato
    // scende invece di salire quando il loop ricomincia. Il cambio si applica all'inizio
    // del blocco SUCCESSIVO a quello in cui il giro viene rilevato (un blocco di ritardo,
    // impercettibile) invece che nel mezzo dello stesso blocco: applicarlo li' userebbe
    // una posizione di playhead ancora relativa al loop vecchio per programmare eventi
    // del loop nuovo, disallineando tutto.
    double previousLoopPositionLocal = -1.0;
    bool loopWrappedLastBlock = false;

    int previousActiveSlot = -1;
    InstrumentFamily previousFamily = InstrumentFamily::Piano;
    int previousIntensity = -1;
    ChordDefinition previousChord;
    PatternRate previousRate = PatternRate::Normal;
    float previousSwing = -1.0f;
    float previousGate = -1.0f;
    int previousOctaveRange = 0;
    bool previousHumanizeEnabled = true;
    bool havePreviousSelection = false;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmartChordAudioProcessor)
};

} // namespace smartchord
