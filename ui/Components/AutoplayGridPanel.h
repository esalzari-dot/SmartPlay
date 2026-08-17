#pragma once

#include "AutoplayGridComponent.h"
#include "ChordPadRow.h"
#include "FamilySwitcher.h"
#include "PatternReadout.h"
#include "PlayStripRow.h"

#include "smartchord/ArpeggiatorEngine.h"
#include "smartchord/AutoplayGridState.h"
#include "smartchord/ChordBankModule.h"
#include "smartchord/ChordProgressions.h"
#include "smartchord/PatternLibrary.h"
#include "smartchord/PlayStripEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace smartchord::ui
{

// Compone lo switcher famiglia, la riga di 8 pad accordo e la griglia Autoplay 8x4
// (SPEC.md sezione 9). Non possiede lo stato: opera per riferimento su ChordBankModule
// e AutoplayGridState fornite dal chiamante, cosi' lo stesso pannello puo' essere
// riusato sia dall'harness standalone (ui/Source/Main.cpp) sia dal vero plugin
// (PluginEditor, dove lo stato e' posseduto da AudioProcessor).
class AutoplayGridPanel : public juce::Component
{
public:
    AutoplayGridPanel (ChordBankModule& chordBank,
                        AutoplayGridState& gridState,
                        const PatternLibrary& patternLibrary,
                        InstrumentFamily initialFamily = InstrumentFamily::Guitar);

    InstrumentFamily getActiveFamily() const noexcept { return activeFamily; }

    // Cambia la famiglia attiva senza passare dal click sul FamilySwitcher (es. quando la
    // famiglia e' cambiata da automazione host - SPEC.md sezione 8). Non chiama refresh():
    // il chiamante che aggiorna piu' campi insieme lo fa una volta sola alla fine.
    void setActiveFamily (InstrumentFamily family);

    // Da chiamare quando lo stato esterno cambia per motivi non originati da questo
    // pannello (es. host che ripristina uno stato salvato).
    void refresh();

    // Chiamata dopo ogni interazione che modifica chordBank/gridState/activeFamily
    // (incluso il refresh() iniziale). Il chiamante che condivide questo stato con un
    // thread audio (es. PluginEditor) puo' usarla per ripropagare la modifica in modo
    // thread-safe verso AudioProcessor.
    std::function<void()> onStateChanged;

    // Interruttore "suona a trasporto fermo": ha senso solo dentro il plugin, dove c'e'
    // un trasporto dell'host, quindi resta nascosto finche' non lo si abilita.
    void setFreeRunControlVisible (bool shouldBeVisible);
    void setFreeRun (bool shouldFreeRun);
    std::function<void (bool)> onFreeRunChanged;

    void setVoiceLeading (bool shouldLead);
    std::function<void (bool)> onVoiceLeadingChanged;

    // Moltiplicatore globale sulla velocita' dei pattern.
    void setPatternRate (PatternRate rate);
    std::function<void (PatternRate)> onPatternRateChanged;

    void setChordFromKeyboard (bool shouldRecognize);
    std::function<void (bool)> onChordFromKeyboardChanged;

    // Panico manuale (MIDI panic, vedi setFreeRunControlVisible: stesso gruppo, ha senso
    // solo dentro il plugin): un click, non un interruttore - chiude tutte le note in
    // corso (Autoplay e Play) e la riproduzione riprende subito pulita, non resta muta.
    std::function<void()> onStopRequested;

    // Swing globale (0-1), gate globale (moltiplicatore 0.25-1.5) e range d'ottava
    // (-2..+2): gli ultimi tre automatizzabili di SPEC.md sezione 8, oltre a rate e alla
    // griglia di intensita' gia' esposti altrove nel pannello.
    void setGlobalSwing (float amount01);
    std::function<void (float)> onGlobalSwingChanged;

    void setGlobalGateLength (float multiplier);
    std::function<void (float)> onGlobalGateLengthChanged;

    void setOctaveRange (int octaves);
    std::function<void (int)> onOctaveRangeChanged;

    // Quando true un cambio di accordo/pattern mentre si sta gia' suonando aspetta il
    // prossimo giro del loop invece di scattare subito.
    void setQuantizeChordSwitch (bool shouldQuantize);
    std::function<void (bool)> onQuantizeChordSwitchChanged;

    // Quando false, i pattern che prevedono humanizeTiming/humanizeVelocity suonano
    // comunque in modo deterministico, ignorando quei campi.
    void setHumanizeEnabled (bool shouldHumanize);
    std::function<void (bool)> onHumanizeEnabledChanged;

    // Posizione normalizzata (0-1) nel loop in esecuzione, per il playhead (SPEC.md
    // sezione 9); fuori da [0,1] lo nasconde.
    void setPlayheadPosition (float normalized);

    // Preset del banco accordi salvati dall'utente. Il pannello non li persiste da solo
    // (non ha accesso al filesystem, ed e' riusato anche dall'harness standalone): apre
    // il dialogo per il nome e lascia al chiamante il salvataggio/cancellazione vero e
    // proprio su disco, poi si aspetta un setAvailablePresets() aggiornato.
    void setAvailablePresets (const juce::StringArray& names);
    std::function<void (const juce::String& name)> onSavePresetRequested;
    std::function<void (const juce::String& name)> onLoadPresetRequested;
    std::function<void (const juce::String& name)> onDeletePresetRequested;

    // Modalita' Play (SPEC.md sezione 5.5): alternativa gestuale all'Autoplay Grid, non la
    // sostituisce. setPlayModeActive() serve al chiamante per ripristinare lo stato salvato
    // senza generare a sua volta onPlayModeChanged (stesso pattern di setFreeRun/
    // setVoiceLeading/ecc.).
    void setPlayModeActive (bool shouldBeActive);
    bool getPlayModeActive() const noexcept { return playModeActive; }
    std::function<void (bool)> onPlayModeChanged;

    // Tocco su una tacca di una barra Play: chordSlot indica quale delle 9, phase/position
    // sono lo stesso gesto grezzo di PlayStripEngine::processGesture (SPEC.md sezione 5.5).
    // Il pannello non genera MIDI da solo (non ha accesso al thread audio): il chiamante
    // (PluginEditor) inoltra il gesto al processor.
    std::function<void (int chordSlot, StripGesturePhase phase, float position)> onPlayStripGesture;

    // Tocco sul nome dell'accordo di una barra Play: l'accordo completo, basso incluso,
    // invece di una singola tacca.
    std::function<void (int chordSlot, bool down)> onPlayStripChordGesture;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    ChordBankModule& chordBank;
    AutoplayGridState& gridState;
    const PatternLibrary& patternLibrary;
    InstrumentFamily activeFamily;

    juce::Label titleLabel;

    // Etichette "eyebrow" mono tracciate, come le sezioni di un pannello hardware
    // (INSTRUMENT/BANK/PERFORMANCE/CHORD BANK/AUTOPLAY). performanceSectionLabel segue
    // la visibilita' di swing/gate/ottava (plugin-only, vedi setGlobalControlsVisible);
    // le altre sono sempre visibili.
    juce::Label instrumentSectionLabel;
    juce::Label bankSectionLabel;
    juce::Label performanceSectionLabel;
    juce::Label chordBankSectionLabel;
    juce::Label autoplaySectionLabel;
    juce::Label simpleCaptionLabel;
    juce::Label complexCaptionLabel;

    juce::ToggleButton freeRunButton { "Suona a trasporto fermo" };
    juce::ToggleButton voiceLeadingButton { "Voice leading" };
    juce::ToggleButton chordFromKeyboardButton { "Accordi da tastiera" };
    juce::TextButton stopButton { "Stop" };
    juce::Label rateLabel;
    juce::ComboBox rateBox;
    juce::Label progressionLabel;
    juce::ComboBox progressionBox;
    juce::ComboBox keyBox;

    juce::Label presetLabel { {}, "Preset" };
    juce::ComboBox presetBox;
    juce::TextButton savePresetButton { "Salva..." };
    juce::TextButton deletePresetButton { "Elimina" };

    juce::Label swingLabel { {}, "Swing" };
    juce::Slider swingSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label gateLabel { {}, "Gate" };
    juce::Slider gateSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label octaveLabel { {}, "Ottava" };
    juce::ComboBox octaveBox;
    juce::ToggleButton quantizeSwitchButton { "Switch a tempo" };
    juce::ToggleButton humanizeButton { "Humanize" };

    void applySelectedProgression();
    void setGlobalControlsVisible (bool shouldBeVisible);
    void updateModeButtons();
    void refreshPlayStrips();
    FamilySwitcher familySwitcher;
    ChordPadRow chordPadRow;
    AutoplayGridComponent autoplayGrid;
    PatternReadout patternReadout;

    // Modalita' Play (SPEC.md sezione 5.5): un secondo modo di eseguire l'accordo attivo,
    // scelto con un piccolo toggle accanto all'etichetta di sezione "Autoplay". false di
    // default: l'Autoplay Grid resta il comportamento atteso appena aperto il plugin.
    bool playModeActive = false;
    juce::TextButton autoplayModeButton { "Autoplay" };
    juce::TextButton playModeButton { "Play" };
    // Una sola barra, non una per accordo: rappresenta l'accordo attualmente attivo (il
    // pad selezionato sopra), verticale come in GarageBand - vedi PlayStripRow.h.
    PlayStripRow playStripRow;
};

} // namespace smartchord::ui
