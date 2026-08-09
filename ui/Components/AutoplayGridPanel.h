#pragma once

#include "AutoplayGridComponent.h"
#include "ChordPadRow.h"
#include "FamilySwitcher.h"
#include "PatternReadout.h"

#include "smartchord/ArpeggiatorEngine.h"
#include "smartchord/AutoplayGridState.h"
#include "smartchord/ChordBankModule.h"
#include "smartchord/ChordProgressions.h"
#include "smartchord/PatternLibrary.h"

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

    // Swing globale (0-1), gate globale (moltiplicatore 0.25-1.5) e range d'ottava
    // (-2..+2): gli ultimi tre automatizzabili di SPEC.md sezione 8, oltre a rate e alla
    // griglia di intensita' gia' esposti altrove nel pannello.
    void setGlobalSwing (float amount01);
    std::function<void (float)> onGlobalSwingChanged;

    void setGlobalGateLength (float multiplier);
    std::function<void (float)> onGlobalGateLengthChanged;

    void setOctaveRange (int octaves);
    std::function<void (int)> onOctaveRangeChanged;

    // Posizione normalizzata (0-1) nel loop in esecuzione, per il playhead (SPEC.md
    // sezione 9); fuori da [0,1] lo nasconde.
    void setPlayheadPosition (float normalized);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    ChordBankModule& chordBank;
    AutoplayGridState& gridState;
    const PatternLibrary& patternLibrary;
    InstrumentFamily activeFamily;

    juce::Label titleLabel;
    juce::ToggleButton freeRunButton { "Suona a trasporto fermo" };
    juce::ToggleButton voiceLeadingButton { "Voice leading" };
    juce::ToggleButton chordFromKeyboardButton { "Accordi da tastiera" };
    juce::Label rateLabel;
    juce::ComboBox rateBox;
    juce::Label progressionLabel;
    juce::ComboBox progressionBox;
    juce::ComboBox keyBox;

    juce::Label swingLabel { {}, "Swing" };
    juce::Slider swingSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label gateLabel { {}, "Gate" };
    juce::Slider gateSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label octaveLabel { {}, "Ottava" };
    juce::ComboBox octaveBox;

    void applySelectedProgression();
    void setGlobalControlsVisible (bool shouldBeVisible);
    FamilySwitcher familySwitcher;
    ChordPadRow chordPadRow;
    AutoplayGridComponent autoplayGrid;
    PatternReadout patternReadout;
};

} // namespace smartchord::ui
