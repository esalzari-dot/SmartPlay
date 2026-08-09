#include "FamilySwitcher.h"

namespace smartchord::ui
{

void FamilySwitcher::SegmentButton::paintButton (juce::Graphics& g, bool isMouseOverButton, bool /*isButtonDown*/)
{
    auto bounds = getLocalBounds().toFloat();

    if (selected)
    {
        juce::ColourGradient gradient (Palette::panelRaisedHi, bounds.getX(), bounds.getY(),
                                        Palette::panelRaised, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (gradient);
        g.fillRoundedRectangle (bounds, 7.0f);
    }
    else if (isMouseOverButton)
    {
        g.setColour (Palette::panelRaised.withAlpha (0.5f));
        g.fillRoundedRectangle (bounds, 7.0f);
    }

    constexpr float dotSize = 6.0f;
    const float dotX = bounds.getCentreX() - 26.0f;
    const auto dotColour = selected ? accent : accent.withAlpha (0.5f);
    g.setColour (dotColour);
    if (selected)
    {
        // Bagliore: lo stesso puntino disegnato piu' grande e trasparente sotto quello
        // pieno, invece di un vero blur (costoso da fare per ogni ridisegno di un
        // pulsante piccolo come questo).
        g.setColour (accent.withAlpha (0.35f));
        g.fillEllipse (dotX - 2.0f, bounds.getCentreY() - dotSize * 0.5f - 2.0f, dotSize + 4.0f, dotSize + 4.0f);
        g.setColour (accent);
    }
    g.fillEllipse (dotX, bounds.getCentreY() - dotSize * 0.5f, dotSize, dotSize);

    g.setColour (selected ? Palette::text : Palette::textDim);
    g.setFont (monoFont (11.5f, juce::Font::bold));
    g.drawText (label, bounds.withTrimmedLeft (18.0f), juce::Justification::centred);
}

FamilySwitcher::FamilySwitcher()
{
    for (size_t i = 0; i < families.size(); ++i)
    {
        auto& button = buttons[i];
        button.label = displayNameFor (families[i]).toUpperCase();
        button.accent = accentColourFor (families[i]);
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
        auto& button = buttons[i];
        button.selected = (families[i] == selectedFamily);
        button.repaint();
    }
}

void FamilySwitcher::paint (juce::Graphics& g)
{
    g.setColour (Palette::panelInset);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);
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
