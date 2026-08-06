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
/ui/          → componenti JUCE della UI (Components/) + harness standalone di sviluppo (Source/)
/plugin/      → il vero plugin JUCE: AudioProcessor/AudioProcessorEditor, formati VST3 e Standalone
```

## Stato del progetto
Sviluppo in corso, seguendo l'ordine indicato nella sezione 10 di `SPEC.md`:

- [x] 1. `ChordDefinition` + tabella intervalli
- [x] 2. `VoicingEngine` + `VoicingProfile` per le 4 famiglie
- [x] 3. `PatternLibrary` (dataset JSON + parser)
- [x] 4. `AutoplayGridState` + `resolvePattern()`
- [x] 5. `ArpeggiatorEngine`
- [x] 6. `MidiOutputManager`
- [x] 7. UI (componenti JUCE riusabili, vedi `/ui`)
- [x] 8. Integrazione VST3/AU (VST3 + Standalone su Linux; AU richiede una build su macOS, non disponibile in questo ambiente)

## Build & test

```
cmake -S . -B build
cmake --build build -j
cd build && ctest --output-on-failure
```

La prima configurazione scarica JUCE (via `FetchContent`): richiede rete e qualche minuto in più
rispetto alle build precedenti, solo core + test.

## UI

`/ui/Components` contiene i componenti JUCE riusabili (switcher famiglia, riga di 8 pad accordo,
griglia Autoplay 8×4, readout del pattern), che seguono il mockup in
`docs/mockup-v2-garageband-style.html` e operano per riferimento su `ChordBankModule` /
`AutoplayGridState` / `PatternLibrary` — cosi' lo stesso `AutoplayGridPanel` e' condiviso sia
dall'harness standalone di sviluppo (`/ui/Source`, stato posseduto localmente) sia dal vero
plugin (`/plugin`, stato posseduto da `AudioProcessor`).

```
cmake --build build --target SmartChordArpUI -j
./build/ui/SmartChordArpUI_artefacts/Debug/Smart\ Chord\ \&\ Arpeggiator
```

Per disabilitare la build della UI (es. in ambienti senza le librerie di sistema di JUCE:
X11/ALSA/FreeType/Fontconfig/OpenGL), passa `-DSMARTCHORD_BUILD_UI=OFF` a `cmake`.

## Plugin (VST3 / Standalone)

`/plugin` e' il vero `AudioProcessor`/`AudioProcessorEditor` JUCE: `isMidiEffect() == true`,
nessun bus audio, solo MIDI out. In `processBlock()` si aggancia all'`AudioPlayHead` dell'host
per BPM e posizione PPQ (SPEC.md sezione 6), pianifica gli eventi del pattern corrente con
`smartchord::scheduleEventsInWindow()` (gestisce anche il wrap-around a fine loop) e applica
`MidiOutputManager::allNotesOff()` (panic) quando accordo o intensita' cambiano a meta' pattern
(SPEC.md sezione 7). Lo stato condiviso con la UI (`ChordBankModule`, `AutoplayGridState`,
famiglia attiva) e' protetto da un lock breve preso solo per copiarlo a inizio blocco, cosi' il
thread audio non lo tiene mai per la durata di `processBlock()`. Lo stato e' persistito tramite
`getStateInformation`/`setStateInformation` (salvataggio di sessione/preset nella DAW).

Il dataset dei pattern e' **embeddato nel binario** (`juce_add_binary_data`, come previsto
da `SPEC.md` sezione 5.4): un plugin distribuito non puo' dipendere da un percorso
dell'albero sorgente della macchina che lo ha compilato.

### Due varianti

| Target | Formato | Quando serve |
|---|---|---|
| `SmartChordArp` | MIDI FX (`isMidiEffect() == true`, nessun bus audio) | La forma corretta da `SPEC.md`: Cubase, Reaper, Studio One, FL Studio, Logic (AU) |
| `SmartChordArpInst` | Strumento (VSTi, uscita audio silenziosa) | Host che non ospitano i MIDI FX VST3 — **Ableton Live** |

In Ableton usa la variante *Inst*: caricala su una traccia MIDI, poi sulla traccia dello
strumento vero imposta **MIDI From → \<traccia\> → Smart Chord Arpeggiator Inst**.

```
cmake --build build --target SmartChordArp_VST3 -j         # bundle .vst3 (MIDI FX)
cmake --build build --target SmartChordArpInst_VST3 -j     # bundle .vst3 (strumento)
cmake --build build --target SmartChordArp_Standalone -j   # eseguibile standalone
```

I binari finiscono in `build/plugin/SmartChordArp_artefacts/<Debug|Release>/`. Per usare il VST3
in una DAW, copia (o crea un link a) la cartella `.vst3` nella cartella dei plugin VST3 della DAW
(es. `~/.vst3/` su Linux). **Nota**: questo ambiente di sviluppo e' headless e senza una DAW reale
installata — la build e la UI sono state verificate (compilazione pulita, avvio sotto Xvfb,
interazione simulata sul target Standalone), ma il test in una DAW vera (SPEC.md sezione 10,
step 8: "test in DAW reale con VST strumento a valle") va fatto sulla tua macchina.

Per disabilitare la build del plugin, passa `-DSMARTCHORD_BUILD_PLUGIN=OFF` a `cmake`.

## Requisiti
- JUCE (ultima versione stabile)
- CMake
- Compilatore C++17+
- Su Linux, per UI e plugin: librerie di sviluppo X11/ALSA/FreeType/Fontconfig/OpenGL
  (`libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype-dev libfontconfig1-dev
  libasound2-dev libgl1-mesa-dev libcurl4-openssl-dev`)
