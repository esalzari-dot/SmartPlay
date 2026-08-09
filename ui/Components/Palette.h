#pragma once

#include "smartchord/VoicingEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace smartchord::ui
{

// Palette "studio panel": pannello scuro e materico invece del pannello chiaro piatto
// della versione precedente. Replica i valori CSS in docs/mockup-v3-studio-panel.html
// (evoluzione di docs/mockup-v1-hardware.html, mai implementato fino ad ora).
//
// Tre livelli di superficie, dal piu' scuro al piu' chiaro: seam (incassato/divisori) <
// panel (corpo del plugin) < panelRaised/panelRaisedHi (pad, manopole, in rilievo). E'
// la stessa gerarchia di un pannello hardware vero: i controlli "escono" dal corpo dello
// strumento, la griglia e i readout numerici vi "affondano" dentro.
namespace Palette
{
    inline const juce::Colour ink           { 0xff17161B }; // sfondo esterno alla finestra del plugin
    inline const juce::Colour panel         { 0xff211F26 }; // corpo del plugin
    inline const juce::Colour panelRaised   { 0xff2B2831 }; // pad/manopole/combobox, in rilievo
    inline const juce::Colour panelRaisedHi { 0xff332F39 }; // cima del gradiente in rilievo, stato hover
    inline const juce::Colour panelInset    { 0xff19171D }; // griglia autoplay, incassata
    inline const juce::Colour seam          { 0xff0C0B10 }; // divisori, fascia titolo/readout, bordi

    inline const juce::Colour text          { 0xffEDE9E2 }; // testo primario (avorio caldo, non bianco puro)
    inline const juce::Colour textMuted     { 0xff938F9B }; // testo secondario
    inline const juce::Colour textDim       { 0xff635F6B }; // etichette terziarie/micro (mono, tracciate)

    inline const juce::Colour piano        { 0xff5AA9E6 };
    inline const juce::Colour pianoDark    { 0xff2E5C82 };
    inline const juce::Colour bass         { 0xffF0794A };
    inline const juce::Colour bassDark     { 0xff8A3F24 };
    inline const juce::Colour guitar       { 0xff4FCC9C };
    inline const juce::Colour guitarDark   { 0xff276B4E };
    inline const juce::Colour strings      { 0xffA78BFA };
    inline const juce::Colour stringsDark  { 0xff5B4A96 };
}

// Font monospace di sistema (SF Mono/Consolas/Roboto Mono a seconda della piattaforma):
// per tutto cio' che e' lettura numerica o meta - percentuali, tempo, etichette di
// sezione - come i readout di uno strumento hardware. Non serve incorporare un font: le
// piattaforme target hanno tutte un monospace di sistema decente.
inline juce::Font monoFont (float height, int styleFlags = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height, styleFlags));
}

inline juce::Colour accentColourFor (InstrumentFamily family)
{
    switch (family)
    {
        case InstrumentFamily::Piano:   return Palette::piano;
        case InstrumentFamily::Bass:    return Palette::bass;
        case InstrumentFamily::Guitar:  return Palette::guitar;
        case InstrumentFamily::Strings: return Palette::strings;
    }
    return Palette::guitar;
}

inline juce::Colour accentDarkColourFor (InstrumentFamily family)
{
    switch (family)
    {
        case InstrumentFamily::Piano:   return Palette::pianoDark;
        case InstrumentFamily::Bass:    return Palette::bassDark;
        case InstrumentFamily::Guitar:  return Palette::guitarDark;
        case InstrumentFamily::Strings: return Palette::stringsDark;
    }
    return Palette::guitarDark;
}

inline juce::String displayNameFor (InstrumentFamily family)
{
    switch (family)
    {
        case InstrumentFamily::Piano:   return "Piano";
        case InstrumentFamily::Bass:    return "Bass";
        case InstrumentFamily::Guitar:  return "Guitar";
        case InstrumentFamily::Strings: return "Strings";
    }
    return {};
}

} // namespace smartchord::ui
