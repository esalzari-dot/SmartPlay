#pragma once

#include "Palette.h"
#include "smartchord/PlayStripEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace smartchord::ui
{

// La barra della modalita' Play (SPEC.md sezione 5.5): un'etichetta col nome dell'accordo
// (tocco = accordo completo, basso incluso) in alto, e N tacche impilate verticalmente
// sotto (tocco/trascinamento = singole note della scala dell'accordo) - grave in basso,
// acuto in alto, come una tastiera o un manico visto di fronte, non una barra orizzontale.
// Una istanza per ciascuno dei 9 accordi del banco, affiancate come colonne di una stessa
// griglia (AutoplayGridPanel::playStripRows): a differenza della vista "un accordo alla
// volta" di GarageBand, restano tutte suonabili insieme, senza dover riselezionare il pad
// sopra per cambiare accordo. Componente "muto" come ChordPadRow/FamilySwitcher: non
// conosce PlayStripEngine ne' produce MIDI, si limita a tradurre gli eventi del mouse in
// callback con fase del gesto, slotIndex e posizione normalizzata (0-1).
class PlayStripRow : public juce::Component
{
public:
    PlayStripRow();

    void setChordLabel (const juce::String& rootLabel, const juce::String& qualityLabel);
    void setNotchCount (int count);
    void setAccentColour (juce::Colour accent);

    // Lo slot dell'accordo attualmente rappresentato (quello attivo sul banco): il
    // chiamante lo aggiorna a ogni refresh(), non e' fisso per l'intera vita del componente.
    void setSlotIndex (int index) noexcept { slotIndex = index; }
    int getSlotIndex() const noexcept { return slotIndex; }

    // Tocco su una tacca: fase del gesto (Down/Move/Up) + posizione normalizzata (0-1),
    // 0 = tacca piu' grave (in basso), 1 = piu' acuta (in alto). Il chiamante riceve lo
    // slotIndex corrente per sapere a quale accordo si riferisce.
    std::function<void (int slotIndex, StripGesturePhase phase, float position)> onNotchGesture;

    // Tocco sul nome dell'accordo: true quando preme, false quando rilascia.
    std::function<void (int slotIndex, bool down)> onChordGesture;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    juce::String rootLabel, qualityLabel;
    int notchCount = 7;
    int slotIndex = 0;
    juce::Colour accent = Palette::guitar;

    juce::Rectangle<int> labelBounds, notchesBounds;
    bool labelPressed = false;
    bool notchesPressed = false;
    int litNotchIndex = -1; // -1 = nessuna tacca evidenziata

    // y e' in coordinate locali del componente (0 in alto, come ogni Component JUCE):
    // la conversione a posizione musicale (0 grave/basso -> 1 acuto/alto) inverte l'asse.
    float positionForY (int y) const;
    int notchIndexForY (int y) const;
};

} // namespace smartchord::ui
