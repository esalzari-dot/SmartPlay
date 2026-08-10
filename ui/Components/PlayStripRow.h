#pragma once

#include "Palette.h"
#include "smartchord/PlayStripEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace smartchord::ui
{

// Una barra della modalita' Play (SPEC.md sezione 5.5): un'etichetta col nome
// dell'accordo (tocco = accordo completo, basso incluso) e N tacche (tocco/trascinamento =
// singole note della scala dell'accordo). Componente "muto" come ChordPadRow/FamilySwitcher:
// non conosce PlayStripEngine ne' produce MIDI, si limita a tradurre gli eventi del mouse in
// callback con fase del gesto e posizione normalizzata (0-1) lungo le tacche.
class PlayStripRow : public juce::Component
{
public:
    PlayStripRow();

    void setChordLabel (const juce::String& rootLabel, const juce::String& qualityLabel);
    void setNotchCount (int count);
    void setAccentColour (juce::Colour accent);
    void setSlotIndex (int index) noexcept { slotIndex = index; }
    int getSlotIndex() const noexcept { return slotIndex; }

    // Tocco su una tacca: fase del gesto (Down/Move/Up) + posizione normalizzata (0-1)
    // lungo le tacche. Il chiamante riceve lo slotIndex per sapere quale barra e' stata
    // toccata senza doversi registrare a una callback per barra.
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

    float positionForX (int x) const;
    int notchIndexForX (int x) const;
};

} // namespace smartchord::ui
