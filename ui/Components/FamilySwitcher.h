#pragma once

#include "Palette.h"
#include "smartchord/VoicingEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace smartchord::ui
{

// Switcher segmentato per famiglia strumentale (SPEC.md sezione 9): Piano/Basso/Chitarra/Archi.
class FamilySwitcher : public juce::Component
{
public:
    FamilySwitcher();

    void setSelectedFamily (InstrumentFamily family);
    InstrumentFamily getSelectedFamily() const noexcept { return selectedFamily; }

    std::function<void (InstrumentFamily)> onFamilyChanged;

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    static constexpr std::array<InstrumentFamily, 4> families {
        InstrumentFamily::Piano, InstrumentFamily::Bass, InstrumentFamily::Guitar, InstrumentFamily::Strings
    };

    // Pulsante custom invece di juce::TextButton: serve per disegnare il puntino colore
    // famiglia e il bagliore sullo stato selezionato, come nel mockup dello studio panel.
    class SegmentButton : public juce::Button
    {
    public:
        SegmentButton() : juce::Button ({}) {}
        void paintButton (juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;

        juce::String label;
        juce::Colour accent;
        bool selected = false;
    };

    std::array<SegmentButton, 4> buttons;
    InstrumentFamily selectedFamily = InstrumentFamily::Guitar;

    void updateButtonStates();
};

} // namespace smartchord::ui
