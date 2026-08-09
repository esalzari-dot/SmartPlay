#include "ChordPadRow.h"
#include "ChordLabel.h"

namespace smartchord::ui
{

namespace
{
    // Id delle voci del menu di modifica accordo. A scope di file, non locali alla
    // funzione: MSVC non consente di usare constexpr locali dentro una lambda con lista
    // di cattura esplicita (error C3493), a differenza di GCC/Clang.
    // L'id 0 e' riservato da JUCE per "menu chiuso senza selezione".
    constexpr int rootIdBase = 100;
    constexpr int qualityIdBase = 200;
    constexpr int inversionIdBase = 300;
    constexpr int octaveIdBase = 400;
    constexpr int octaveIdOffset = 2; // ottave da -2 a +2

    const std::array<ChordQuality, 14> menuQualities {
        ChordQuality::Maj, ChordQuality::Min, ChordQuality::Dim, ChordQuality::Aug,
        ChordQuality::Sus2, ChordQuality::Sus4, ChordQuality::Maj7, ChordQuality::Min7,
        ChordQuality::Dom7, ChordQuality::Min7b5, ChordQuality::Dim7, ChordQuality::Add9,
        ChordQuality::Six, ChordQuality::Nine
    };
}

void ChordPadRow::ChordPad::paintButton (juce::Graphics& g, bool isMouseOverButton, bool /*isButtonDown*/)
{
    auto bounds = getLocalBounds().toFloat();
    constexpr float corner = 8.0f;

    // Tasto in rilievo (gradiente verticale chiaro->scuro) o pieno d'accento quando
    // selezionato: la stessa lettura "keycap" del mockup, senza bisogno di un'ombra vera.
    if (selected)
    {
        juce::ColourGradient gradient (accent, bounds.getX(), bounds.getY(),
                                        accentDark, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (gradient);
        g.fillRoundedRectangle (bounds, corner);

        g.setColour (accent.withAlpha (0.55f));
        g.drawRoundedRectangle (bounds.reduced (1.0f), corner, 2.0f);
    }
    else
    {
        juce::ColourGradient gradient (isMouseOverButton ? Palette::panelRaisedHi : Palette::panelRaised,
                                        bounds.getX(), bounds.getY(),
                                        Palette::panelRaised.darker (0.15f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (gradient);
        g.fillRoundedRectangle (bounds, corner);
    }

    // Numero di slot, come le etichette dei tasti su un controller hardware: un
    // riferimento silenzioso a quale tasto del tastierino numerico lo richiama.
    g.setColour ((selected ? Palette::ink : Palette::text).withAlpha (0.28f));
    g.setFont (monoFont (8.5f));
    g.drawText (juce::String (slotIndex + 1), bounds.reduced (7.0f, 5.0f).removeFromTop (10.0f),
                juce::Justification::topLeft);

    g.setColour (selected ? Palette::ink : Palette::text);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.drawText (rootLabel, bounds.removeFromTop (bounds.getHeight() * 0.6f), juce::Justification::centred);

    g.setColour (selected ? Palette::ink.withAlpha (0.6f) : Palette::textDim);
    g.setFont (monoFont (9.0f));
    g.drawText (qualityLabel, bounds, juce::Justification::centred);
}

void ChordPadRow::ChordPad::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        if (onEditRequested != nullptr)
            onEditRequested();
        return;
    }

    juce::Button::mouseDown (event);
}

ChordPadRow::ChordPadRow()
{
    for (size_t i = 0; i < pads.size(); ++i)
    {
        auto& pad = pads[i];
        const int slot = static_cast<int> (i);

        pad.onClick = [this, slot]
        {
            if (onChordSelected != nullptr)
                onChordSelected (slot);
        };

        pad.onEditRequested = [this, slot] { showEditMenuFor (slot); };

        addAndMakeVisible (pad);
    }
}

void ChordPadRow::showEditMenuFor (int slot)
{
    const auto current = pads[static_cast<size_t> (slot)].chord;

    const int numQualities = static_cast<int> (menuQualities.size());

    juce::PopupMenu rootMenu;
    for (int semitone = 0; semitone < 12; ++semitone)
        rootMenu.addItem (rootIdBase + semitone, noteNameFor (semitone), true, semitone == current.rootSemitone);

    juce::PopupMenu qualityMenu;
    for (int i = 0; i < numQualities; ++i)
        qualityMenu.addItem (qualityIdBase + i, qualityAbbreviationFor (menuQualities[static_cast<size_t> (i)]),
                              true, menuQualities[static_cast<size_t> (i)] == current.quality);

    juce::PopupMenu inversionMenu;
    for (int inversion = 0; inversion < 4; ++inversion)
        inversionMenu.addItem (inversionIdBase + inversion,
                                inversion == 0 ? "Fondamentale" : juce::String (inversion) + "\xc2\xb0 rivolto",
                                true, inversion == current.inversion);

    juce::PopupMenu octaveMenu;
    for (int octave = -octaveIdOffset; octave <= octaveIdOffset; ++octave)
        octaveMenu.addItem (octaveIdBase + octave + octaveIdOffset,
                             octave == 0 ? juce::String ("0") : juce::String::formatted ("%+d", octave),
                             true, octave == current.octaveOffset);

    juce::PopupMenu menu;
    menu.addSectionHeader ("Accordo " + juce::String (slot + 1));
    menu.addSubMenu ("Fondamentale", rootMenu);
    menu.addSubMenu ("Qualita'", qualityMenu);
    menu.addSubMenu ("Rivolto", inversionMenu);
    menu.addSubMenu ("Ottava", octaveMenu);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&pads[static_cast<size_t> (slot)]),
                         [this, slot, current] (int result)
    {
        if (result == 0)
            return;

        auto edited = current;

        if (result >= octaveIdBase)
            edited.octaveOffset = result - octaveIdBase - octaveIdOffset;
        else if (result >= inversionIdBase)
            edited.inversion = result - inversionIdBase;
        else if (result >= qualityIdBase)
            edited.quality = menuQualities[static_cast<size_t> (result - qualityIdBase)];
        else if (result >= rootIdBase)
            edited.rootSemitone = result - rootIdBase;

        if (onChordEdited != nullptr)
            onChordEdited (slot, edited);
    });
}

void ChordPadRow::refreshFrom (const ChordBankModule& bank, juce::Colour accentColour, juce::Colour accentDarkColour)
{
    for (int i = 0; i < numChordBankSlots; ++i)
    {
        const auto& chord = bank.getChord (i);
        auto& pad = pads[static_cast<size_t> (i)];

        pad.chord = chord;
        pad.rootLabel = noteNameFor (chord.rootSemitone);
        pad.qualityLabel = qualityAbbreviationFor (chord.quality);
        pad.slotIndex = i;
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
