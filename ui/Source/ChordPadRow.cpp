#include "ChordPadRow.h"
#include "ChordLabel.h"

namespace smartchord::ui
{

void ChordPadRow::ChordPad::paintButton (juce::Graphics& g, bool isMouseOverButton, bool /*isButtonDown*/)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (selected ? accent : Palette::panel);
    g.fillRoundedRectangle (bounds, 12.0f);

    if (selected)
    {
        g.setColour (accentDark);
        g.drawRoundedRectangle (bounds.reduced (1.0f), 12.0f, 2.0f);
    }
    else if (isMouseOverButton)
    {
        g.setColour (Palette::panelEdge);
        g.drawRoundedRectangle (bounds.reduced (1.0f), 12.0f, 1.0f);
    }

    g.setColour (selected ? juce::Colours::white : Palette::text);
    g.setFont (juce::FontOptions (17.0f, juce::Font::bold));
    g.drawText (rootLabel, bounds.removeFromTop (bounds.getHeight() * 0.6f), juce::Justification::centred);

    g.setColour (selected ? juce::Colours::white : Palette::textMuted);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (qualityLabel, bounds, juce::Justification::centred);
}

ChordPadRow::ChordPadRow()
{
    for (size_t i = 0; i < pads.size(); ++i)
    {
        auto& pad = pads[i];
        pad.onClick = [this, i]
        {
            if (onChordSelected != nullptr)
                onChordSelected (static_cast<int> (i));
        };
        addAndMakeVisible (pad);
    }
}

void ChordPadRow::refreshFrom (const ChordBankModule& bank, juce::Colour accentColour, juce::Colour accentDarkColour)
{
    for (int i = 0; i < numChordBankSlots; ++i)
    {
        const auto& chord = bank.getChord (i);
        auto& pad = pads[static_cast<size_t> (i)];

        pad.rootLabel = noteNameFor (chord.rootSemitone);
        pad.qualityLabel = qualityAbbreviationFor (chord.quality);
        pad.selected = (i == bank.getActiveSlot());
        pad.accent = accentColour;
        pad.accentDark = accentDarkColour;
        pad.repaint();
    }
}

void ChordPadRow::resized()
{
    auto bounds = getLocalBounds();
    const int gap = 8;
    const int padWidth = (bounds.getWidth() - gap * (static_cast<int> (pads.size()) - 1))
        / static_cast<int> (pads.size());

    for (auto& pad : pads)
    {
        pad.setBounds (bounds.removeFromLeft (padWidth));
        bounds.removeFromLeft (gap);
    }
}

} // namespace smartchord::ui
