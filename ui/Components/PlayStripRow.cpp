#include "PlayStripRow.h"

namespace smartchord::ui
{

PlayStripRow::PlayStripRow()
{
    setInterceptsMouseClicks (true, false);
}

void PlayStripRow::setChordLabel (const juce::String& root, const juce::String& quality)
{
    rootLabel = root;
    qualityLabel = quality;
    repaint();
}

void PlayStripRow::setNotchCount (int count)
{
    notchCount = juce::jmax (1, count);
    repaint();
}

void PlayStripRow::setAccentColour (juce::Colour colour)
{
    accent = colour;
    repaint();
}

void PlayStripRow::resized()
{
    auto bounds = getLocalBounds();
    labelBounds = bounds.removeFromTop (34);
    bounds.removeFromTop (8);
    notchesBounds = bounds;
}

float PlayStripRow::positionForY (int y) const
{
    if (notchesBounds.getHeight() <= 0)
        return 0.0f;

    // 0 in alto (convenzione JUCE) -> 0 grave/basso, 1 acuto/alto: l'asse va invertito.
    const float ratio = (static_cast<float> (y) - static_cast<float> (notchesBounds.getY()))
                       / static_cast<float> (notchesBounds.getHeight());
    return juce::jlimit (0.0f, 1.0f, 1.0f - ratio);
}

int PlayStripRow::notchIndexForY (int y) const
{
    if (notchCount <= 1)
        return 0;

    return juce::roundToInt (positionForY (y) * static_cast<float> (notchCount - 1));
}

void PlayStripRow::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (Palette::panelInset);
    g.fillRoundedRectangle (bounds, 8.0f);

    if (labelPressed)
    {
        g.setColour (accent.withAlpha (0.22f));
        g.fillRoundedRectangle (labelBounds.toFloat(), 8.0f);
    }

    g.setColour (Palette::text);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText (rootLabel, labelBounds.reduced (10, 0).removeFromLeft (labelBounds.getWidth() / 2),
                juce::Justification::centredLeft);
    g.setColour (Palette::textDim);
    g.setFont (monoFont (9.5f));
    g.drawText (qualityLabel.toUpperCase(), labelBounds.reduced (10, 0),
                juce::Justification::centredRight);

    // Tacca 0 (piu' grave) in basso, l'ultima (piu' acuta) in alto: come un manico/tastiera
    // visti di fronte, non una barra letta da sinistra a destra.
    const int count = juce::jmax (1, notchCount);
    const float notchHeight = static_cast<float> (notchesBounds.getHeight()) / static_cast<float> (count);

    for (int i = 0; i < count; ++i)
    {
        const float bandTop = static_cast<float> (notchesBounds.getY())
                             + static_cast<float> (count - 1 - i) * notchHeight;
        const juce::Rectangle<float> notchArea (
            static_cast<float> (notchesBounds.getX()),
            bandTop + 1.0f,
            static_cast<float> (notchesBounds.getWidth()),
            notchHeight - 2.0f);

        if (i == litNotchIndex)
        {
            g.setColour (accent.withAlpha (0.35f));
            g.fillRoundedRectangle (notchArea.expanded (0.0f, 2.0f), 3.0f);
            g.setColour (accent);
            g.fillRoundedRectangle (notchArea, 2.0f);
        }
        else
        {
            g.setColour (Palette::text.withAlpha (0.07f + 0.03f * (i % 2)));
            g.fillRoundedRectangle (notchArea, 2.0f);
        }
    }
}

void PlayStripRow::mouseDown (const juce::MouseEvent& event)
{
    if (labelBounds.contains (event.getPosition()))
    {
        labelPressed = true;
        repaint();

        if (onChordGesture != nullptr)
            onChordGesture (slotIndex, true);
        return;
    }

    notchesPressed = true;
    litNotchIndex = notchIndexForY (event.y);
    repaint();

    if (onNotchGesture != nullptr)
        onNotchGesture (slotIndex, StripGesturePhase::Down, positionForY (event.y));
}

void PlayStripRow::mouseDrag (const juce::MouseEvent& event)
{
    if (! notchesPressed)
        return;

    const int idx = notchIndexForY (event.y);
    if (idx != litNotchIndex)
    {
        litNotchIndex = idx;
        repaint();
    }

    if (onNotchGesture != nullptr)
        onNotchGesture (slotIndex, StripGesturePhase::Move, positionForY (event.y));
}

void PlayStripRow::mouseUp (const juce::MouseEvent& event)
{
    if (labelPressed)
    {
        labelPressed = false;
        repaint();

        if (onChordGesture != nullptr)
            onChordGesture (slotIndex, false);
        return;
    }

    if (notchesPressed)
    {
        notchesPressed = false;
        litNotchIndex = -1;
        repaint();

        if (onNotchGesture != nullptr)
            onNotchGesture (slotIndex, StripGesturePhase::Up, positionForY (event.y));
    }
}

} // namespace smartchord::ui
