# Smart Chord & Arpeggiator

Plugin MIDI FX (VST3/AU) per DAW desktop, scritto in JUCE (C++), ispirato agli Smart Instrument di GarageBand per iOS.

Permette di selezionare fino a 8 accordi e generare pattern di arpeggio/accompagnamento tramite una griglia **Autoplay** (accordo × intensità), con profili musicali dedicati per Piano, Basso, Chitarra e Archi. Il plugin genera solo eventi MIDI, da instradare verso un VST strumento a scelta dell'utente.

## Struttura del repository

```
/SPEC.md      → specifica tecnica completa (architettura, moduli, dataset pattern)
/CLAUDE.md    → istruzioni operative per lo sviluppo assistito da Claude Code
/docs/        → mockup UI di riferimento (HTML)
/data/        → dataset pattern in formato JSON (PatternLibrary, SPEC.md sezione 5.3)
/src/         → moduli C++ (smartchord::ChordDefinition, VoicingEngine, PatternLibrary, ...)
/tests/       → unit test Catch2
```

## Stato del progetto
Sviluppo in corso, seguendo l'ordine indicato nella sezione 10 di `SPEC.md`:

- [x] 1. `ChordDefinition` + tabella intervalli
- [x] 2. `VoicingEngine` + `VoicingProfile` per le 4 famiglie
- [x] 3. `PatternLibrary` (dataset JSON + parser)
- [x] 4. `AutoplayGridState` + `resolvePattern()`
- [x] 5. `ArpeggiatorEngine`
- [x] 6. `MidiOutputManager`
- [ ] 7. UI
- [ ] 8. Integrazione VST3/AU

## Build & test

```
cmake -S . -B build
cmake --build build -j
cd build && ctest --output-on-failure
```

## Requisiti
- JUCE (ultima versione stabile)
- CMake
- Compilatore C++17+
