#include "AutoplayGridComponent.h"

namespace smartchord::ui
{

void AutoplayGridComponent::GridCell::paintButton (juce::Graphics& g, bool isMouseOverButton, bool /*isButtonDown*/)
{
    auto bounds = getLocalBounds().toFloat();

    juce::Colour fill { 0xffF3F4F6 };

    if (isSelected)
        fill = accent;
    else if (isActiveColumn)
        fill = juce::Colour (0xffEDF6F1);
    else if (isMouseOverButton)
        fill = juce::Colour (0xffE7E9EC);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 3.0f);
}

AutoplayGridComponent::AutoplayGridComponent()
{
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < numChordSlots; ++col)
        {
            auto& cell = cells[static_cast<size_t> (row)][static_cast<size_t> (col)];
            const int intensity = 3 - row; // riga in alto = intensita' massima
            cell.onClick = [this, col, intensity]
            {
                if (onCellSelected != nullptr)
                    onCellSelected (col, intensity);
            };
            addAndMakeVisible (cell);
        }
    }
}

void AutoplayGridComponent::refreshFrom (const AutoplayGridState& gridState, InstrumentFamily family,
                                          int activeChordSlot, juce::Colour accentColour)
{
    for (int row = 0; row < 4; ++row)
    {
        const int intensity = 3 - row;
        for (int col = 0; col < numChordSlots; ++col)
        {
            auto& cell = cells[static_cast<size_t> (row)][static_cast<size_t> (col)];
            const bool isActiveColumn = (col == activeChordSlot);
            cell.isActiveColumn = isActiveColumn;
            cell.isSelected = isActiveColumn && gridState.getIntensity (family, col) == intensity;
            cell.accent = accentColour;
            cell.repaint();
        }
    }
}

void AutoplayGridComponent::paint (juce::Graphics& g)
{
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 14.0f);
}

void AutoplayGridComponent::resized()
{
    auto bounds = getLocalBounds().reduced (8);
    const int gap = 4;
    const int cellHeight = (bounds.getHeight() - gap * 3) / 4;

    for (int row = 0; row < 4; ++row)
    {
        auto rowBounds = bounds.removeFromTop (cellHeight);
        if (row < 3)
            bounds.removeFromTop (gap);

        const int cellWidth = (rowBounds.getWidth() - gap * (numChordSlots - 1)) / numChordSlots;
        for (int col = 0; col < numChordSlots; ++col)
        {
            cells[static_cast<size_t> (row)][static_cast<size_t> (col)].setBounds (rowBounds.removeFromLeft (cellWidth));
            if (col < numChordSlots - 1)
                rowBounds.removeFromLeft (gap);
        }
    }
}

} // namespace smartchord::ui
