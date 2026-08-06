# Smart Chord & Arpeggiator

Plugin MIDI FX (VST3/AU) per DAW desktop, scritto in JUCE (C++), ispirato agli Smart Instrument di GarageBand per iOS.

Permette di selezionare fino a 8 accordi e generare pattern di arpeggio/accompagnamento tramite una griglia **Autoplay** (accordo × intensità), con profili musicali dedicati per Piano, Basso, Chitarra e Archi. Il plugin genera solo eventi MIDI, da instradare verso un VST strumento a scelta dell'utente.

## ⬇️ Download

### **[→ Scarica l'ultima versione](https://github.com/esalzari-dot/SmartPlay/releases/latest)**

(tutte le versioni: [pagina Releases](https://github.com/esalzari-dot/SmartPlay/releases))

Quale file scaricare:

| DAW | File |
|---|---|
| **Ableton Live** (Windows) | `SmartChordArp-VST3-Instrument-windows.zip` |
| **Ableton Live** (macOS) | `SmartChordArp-VST3-Instrument-macos.zip` |
| Cubase, Reaper, Studio One, FL Studio (Windows) | `SmartChordArp-VST3-windows.zip` |
| Cubase, Reaper, Studio One, FL Studio (macOS) | `SmartChordArp-VST3-macos.zip` |
| Logic Pro | `SmartChordArp-AU-macos.zip` |
| Senza DAW (app autonoma) | `SmartChordArp-Standalone-<piattaforma>.zip` |

Ableton Live non ospita i plugin VST3 di tipo MIDI FX: lì serve la variante *Instrument*
(vedi [Plugin](#plugin-vst3--standalone) per il perché e per come instradare il MIDI).

Dove copiare la cartella `.vst3` estratta:

- **Windows**: `C:\Program Files\Common Files\VST3\`
- **macOS**: `~/Library/Audio/Plug-Ins/VST3/` — per l'AU, il `.component` va in `~/Library/Audio/Plug-Ins/Components/`
- **Linux**: `~/.vst3/`

Poi riavvia la DAW e fai un rescan dei plugin.

> I binari non sono firmati digitalmente: su macOS Gatekeeper li blocca al primo avvio
> (tasto destro → Apri, oppure `xattr -dr com.apple.quarantine <percorso>`), su Windows
> può comparire un avviso SmartScreen.

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

## Come si usa

- **Famiglia strumentale**: switcher in alto (Piano / Bass / Guitar / Strings). Cambia i
  voicing e il set di pattern disponibili.
- **Scegliere un accordo da suonare**: click sinistro su uno degli 8 pad.
- **Cambiare l'accordo contenuto in un pad**: **click destro** sul pad → menu con
  fondamentale, qualità (14 tipi), rivolto e ottava. La modifica è immediata e viene
  salvata nella sessione della DAW.
- **Cambiare accordo mentre suoni**: manda al plugin una nota MIDI nella fascia dei
  keyswitch, **C1–G1** (note MIDI 24-31, con C3 = 60): ognuna seleziona uno degli 8 slot.
  Queste note non vengono passate a valle e stanno sotto la tessitura dei profili
  strumentali, quindi non si sovrappongono a quello che suoni.
- **Intensità del pattern**: griglia Autoplay 8×4 — colonna = accordo, riga = intensità
  (dal basso, semplice, verso l'alto, complesso). Un click seleziona entrambe insieme.

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
