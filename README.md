# Smart Chord & Arpeggiator

Plugin MIDI FX (VST3/AU) per DAW desktop, scritto in JUCE (C++), ispirato agli Smart Instrument di GarageBand per iOS.

Permette di selezionare fino a 8 accordi e generare pattern di arpeggio/accompagnamento tramite una griglia **Autoplay** (accordo × intensità), con profili musicali dedicati per Piano, Basso, Chitarra e Archi. Il plugin genera solo eventi MIDI, da instradare verso un VST strumento a scelta dell'utente.

## Struttura del repository

```
/SPEC.md      → specifica tecnica completa (architettura, moduli, dataset pattern)
/CLAUDE.md    → istruzioni operative per lo sviluppo assistito da Claude Code
/docs/        → mockup UI di riferimento (HTML)
```

## Stato del progetto
Fase di design completata (vedi `SPEC.md`). Sviluppo del codice non ancora iniziato — seguire l'ordine indicato nella sezione 10 di `SPEC.md`.

## Requisiti
- JUCE (ultima versione stabile)
- CMake
- Compilatore C++17+
