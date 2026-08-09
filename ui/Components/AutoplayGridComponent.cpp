#include "AutoplayGridComponent.h"

namespace smartchord::ui
{

void AutoplayGridComponent::GridCell::paintButton (juce::Graphics& g, bool isMouseOverButton, bool /*isButtonDown*/)
{
    auto bounds = getLocalBounds().toFloat();

    if (isSelected)
    {
        // Bagliore intorno alla cella accesa: un rettangolo piu' grande e trasparente
        // sotto quello pieno, come il puntino del FamilySwitcher - stessa tecnica, stesso
        // motivo (niente vero blur per una cella ridisegnata spesso).
        g.setColour (accent.withAlpha (0.35f));
        g.fillRoundedRectangle (bounds.expanded (1.5f), 4.0f);
        g.setColour (accent);
        g.fillRoundedRectangle (bounds, 3.0f);
        return;
    }

    juce::Colour fill = Palette::text.withAlpha (isActiveColumn ? 0.10f : 0.05f);
    if (isMouseOverButton)
        fill = Palette::text.withAlpha (0.16f);

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
    // Incassata rispetto al corpo del plugin, come una matrice LED vista dall'alto:
    // un'ombra interna leggera invece di un riempimento piatto la fa leggere come un
    // "buco" nel pannello, non come un'altra card in rilievo.
    auto bounds = getLocalBounds().toFloat();
    g.setColour (Palette::panelInset);
    g.fillRoundedRectangle (bounds, 12.0f);

    juce::ColourGradient shadow (Palette::seam.withAlpha (0.5f), bounds.getX(), bounds.getY(),
                                  juce::Colours::transparentBlack, bounds.getX(), bounds.getY() + 14.0f, false);
    g.setGradientFill (shadow);
    g.fillRoundedRectangle (bounds, 12.0f);
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
