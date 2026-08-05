#include "FamilySwitcher.h"

namespace smartchord::ui
{

FamilySwitcher::FamilySwitcher()
{
    for (size_t i = 0; i < families.size(); ++i)
    {
        auto& button = buttons[i];
        button.setButtonText (displayNameFor (families[i]));
        button.setClickingTogglesState (false);
        button.onClick = [this, i]
        {
            selectedFamily = families[i];
            updateButtonStates();
            if (onFamilyChanged != nullptr)
                onFamilyChanged (selectedFamily);
            repaint();
        };
        addAndMakeVisible (button);
    }

    updateButtonStates();
}

void FamilySwitcher::setSelectedFamily (InstrumentFamily family)
{
    selectedFamily = family;
    updateButtonStates();
    repaint();
}

void FamilySwitcher::updateButtonStates()
{
    for (size_t i = 0; i < families.size(); ++i)
    {
        const bool isSelected = families[i] == selectedFamily;
        auto& button = buttons[i];
        button.setColour (juce::TextButton::buttonColourId, isSelected ? Palette::panel : Palette::panelEdge);
        button.setColour (juce::TextButton::textColourOffId, isSelected ? Palette::text : Palette::textMuted);
    }
}

void FamilySwitcher::paint (juce::Graphics& g)
{
    g.setColour (Palette::panelEdge);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 12.0f);
}

void FamilySwitcher::resized()
{
    auto bounds = getLocalBounds().reduced (3);
    const int gap = 3;
    const int segmentWidth = (bounds.getWidth() - gap * (static_cast<int> (buttons.size()) - 1))
        / static_cast<int> (buttons.size());

    for (auto& button : buttons)
    {
        button.setBounds (bounds.removeFromLeft (segmentWidth));
        bounds.removeFromLeft (gap);
    }
}

} // namespace smartchord::ui
