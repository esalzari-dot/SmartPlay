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

    std::array<juce::TextButton, 4> buttons;
    InstrumentFamily selectedFamily = InstrumentFamily::Guitar;

    void updateButtonStates();
};

} // namespace smartchord::ui
