# Smart Chord & Arpeggiator

Plugin MIDI FX (VST3/AU) per DAW desktop, scritto in JUCE (C++), ispirato agli Smart Instrument di GarageBand per iOS.

Permette di selezionare fino a 8 accordi e generare pattern di arpeggio/accompagnamento tramite una griglia **Autoplay** (accordo × intensità), con profili musicali dedicati per Piano, Basso, Chitarra e Archi. Il plugin genera solo eventi MIDI, da instradare verso un VST strumento a scelta dell'utente.

## Struttura del repository

```
/SPEC.md      → specifica tecnica completa (architettura, moduli, dataset pattern)
/CLAUDE.md    → istruzioni operative per lo sviluppo assistito da Claude Code
/docs/        → mockup UI di riferimento (HTML)
/data/        → dataset pattern in formato JSON (PatternLibrary, SPEC.md sezione 5.3)
/src/         → moduli core C++ (smartchord::ChordDefinition, VoicingEngine, PatternLibrary, ...)
/tests/       → unit test Catch2
/ui/          → harness JUCE standalone per la UI (switcher famiglia, pad accordo, griglia Autoplay)
```

## Stato del progetto
Sviluppo in corso, seguendo l'ordine indicato nella sezione 10 di `SPEC.md`:

- [x] 1. `ChordDefinition` + tabella intervalli
- [x] 2. `VoicingEngine` + `VoicingProfile` per le 4 famiglie
- [x] 3. `PatternLibrary` (dataset JSON + parser)
- [x] 4. `AutoplayGridState` + `resolvePattern()`
- [x] 5. `ArpeggiatorEngine`
- [x] 6. `MidiOutputManager`
- [x] 7. UI (harness JUCE standalone, vedi `/ui`)
- [ ] 8. Integrazione VST3/AU

## Build & test

```
cmake -S . -B build
cmake --build build -j
cd build && ctest --output-on-failure
```

La prima configurazione scarica JUCE (via `FetchContent`): richiede rete e qualche minuto in più
rispetto alle build precedenti, solo core + test.

## UI (harness di sviluppo)

`/ui` contiene un'app JUCE standalone (non ancora il plugin VST3/AU, quello è lo step 8) che
mostra lo switcher famiglia, la riga di 8 pad accordo e la griglia Autoplay 8×4, seguendo il
mockup in `docs/mockup-v2-garageband-style.html` e collegata ai moduli core reali
(`ChordBankModule`, `AutoplayGridState`, `PatternLibrary`).

```
cmake --build build --target SmartChordArpUI -j
./build/ui/SmartChordArpUI_artefacts/Debug/Smart\ Chord\ \&\ Arpeggiator
```

Per disabilitare la build della UI (es. in ambienti senza le librerie di sistema di JUCE:
X11/ALSA/FreeType/Fontconfig/OpenGL), passa `-DSMARTCHORD_BUILD_UI=OFF` a `cmake`.

## Requisiti
- JUCE (ultima versione stabile)
- CMake
- Compilatore C++17+
