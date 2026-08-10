# SmartPlay

Plugin MIDI FX (VST3/AU) per DAW desktop, scritto in JUCE (C++), ispirato agli Smart Instrument di GarageBand per iOS.

Permette di selezionare fino a 9 accordi e generare pattern di arpeggio/accompagnamento tramite una griglia **Autoplay** (accordo × intensità), con profili musicali dedicati per Piano, Basso, Chitarra e Archi. Il plugin genera solo eventi MIDI, da instradare verso un VST strumento a scelta dell'utente.

## ⬇️ Download

### **[→ Scarica l'ultima versione](https://github.com/esalzari-dot/SmartPlay/releases/latest)**

(tutte le versioni: [pagina Releases](https://github.com/esalzari-dot/SmartPlay/releases))

Quale file scaricare:

| DAW | File |
|---|---|
| **Ableton Live** (qualsiasi edizione) | `SmartPlay-VST3-Instrument-<piattaforma>.zip` |
| Cubase, Reaper, Studio One, FL Studio, Bitwig | `SmartPlay-VST3-MIDIFX-<piattaforma>.zip` |
| Logic Pro | `SmartPlay-AU-MIDIFX-macos.zip` |
| Senza DAW (app autonoma) | `SmartPlay-Standalone-<piattaforma>.zip` — include un piano di anteprima incorporato, si sente subito |

Nel dubbio, la variante **Instrument** funziona in tutti gli host: la si carica su una
traccia MIDI e se ne preleva l'uscita da un'altra traccia. La variante **MIDIFX** è più
comoda dove è supportata (sta nello slot MIDI della stessa traccia dello strumento, senza
seconda traccia), ma **Ableton Live non la carica** — Live non ospita i MIDI effect VST3 di
terze parti, in nessuna edizione. L'unico modo per avere un vero MIDI effect in Live sarebbe
un device Max for Live, che richiede Live Suite ed è un progetto a sé.

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
- **Scegliere un accordo da suonare**: click sinistro su uno dei 9 pad.
- **Cambiare l'accordo contenuto in un pad**: **click destro** sul pad → menu con
  fondamentale, qualità (14 tipi), rivolto e ottava. La modifica è immediata e viene
  salvata nella sessione della DAW.
- **Riempire i 9 pad in un colpo solo**: menu **Progressione** + tonalità, sopra la riga
  dei pad. Otto giri d'armonia pronti (pop I-V-vi-IV, doo-wop, canone, ii-V-I jazz, blues
  di 12 battute, modale…) trasposti nella tonalità che scegli. Le progressioni più corte
  di 9 accordi si ripetono, così tutti i pad restano utilizzabili.
- **Salvare i tuoi accordi come preset**: accanto a Progressione, **Salva...** cattura il
  contenuto attuale dei 9 pad con un nome a scelta; il menu **Preset** lo richiama in
  qualunque altro progetto, **Elimina** lo rimuove. A differenza delle progressioni
  incorporate, i preset sono tuoi: salvati in
  `Documenti/SmartPlay/chordBankPresets.json`, un JSON leggibile e modificabile a
  mano come `patterns.json`.
- **Cambiare accordo mentre suoni**: manda al plugin una nota MIDI nella fascia dei
  keyswitch, **C1–G#1** (note MIDI 24-32, con C3 = 60): ognuna seleziona uno dei 9 slot.
  Queste note non vengono passate a valle e stanno sotto la tessitura dei profili
  strumentali, quindi non si sovrappongono a quello che suoni.
- **Cambiare accordo da tastiera del computer**: quando la finestra del plugin ha il
  focus, i tasti **1-9** (sia sul tastierino numerico sia sulla riga in alto) selezionano
  lo slot corrispondente, senza bisogno di un controller MIDI.
- **Accordi da tastiera** (in alto, da attivare): invece di scegliere un pad, suona
  l'accordo. Le note sopra la fascia dei keyswitch vengono riconosciute (triadi, settime,
  sus, diminuite…) e l'arpeggiatore le usa finché le tieni premute; al rilascio torna
  all'accordo selezionato sul banco. Servono almeno tre note diverse: sotto quella soglia
  l'accordo sarebbe ambiguo e il plugin preferisce non indovinare.
- **Quando suona**: l'arpeggiatore è sincronizzato al trasporto dell'host, quindi genera
  MIDI solo mentre la DAW sta suonando (premi Play). Per provare accordi e pattern a
  trasporto fermo attiva **Suona a trasporto fermo** in alto a destra: passa a un clock
  interno al BPM dell'host.
- **Voice leading** (in alto, attivo di default): sceglie per ogni accordo il rivolto che
  muove meno le voci rispetto al precedente, invece di saltare. Disattivandolo vale il
  rivolto che imposti a mano su ogni pad.
- **Switch a tempo** (sotto Swing/Gate/Ottava, disattivo di default): quando lo attivi, un
  cambio di accordo/pattern mentre l'arpeggiatore sta già suonando non scatta subito —
  aspetta che il loop corrente finisca il giro, così non taglia una nota o uno strum a
  metà. A trasporto fermo, o se non c'era nulla in corso, si applica comunque subito.
  Disattivato (il default) il cambio è immediato, come sempre.
- **Humanize** (accanto, attivo di default): quando un pattern prevede variazione casuale
  di attacco/dinamica (`humanizeTiming`/`humanizeVelocity` nel JSON), disattivando questo
  interruttore la sequenza torna deterministica per quel pattern, ignorando quei campi.
- **Intensità del pattern**: griglia Autoplay 9×4 — colonna = accordo, riga = intensità
  (dal basso, semplice, verso l'alto, complesso). Un click seleziona entrambe insieme.
- **Play** (toggle Autoplay/Play, sopra la griglia): una seconda modalità di esecuzione,
  alternativa al pattern preregistrato — `SPEC.md` §5.5. Al posto della griglia mostra 9
  barre, una per accordo, ognuna con un numero fisso di tacche (7 pianoforte, 6 chitarra,
  4 basso/archi) che rappresentano la scala dell'accordo: clic su una tacca suona quella
  nota tenuta finché premi, clic e trascina suona le tacche attraversate una dopo l'altra
  (velocity legata alla velocità del trascinamento). Clic sul nome dell'accordo (a
  sinistra di ogni barra) suona l'accordo intero, basso compreso. Passare a Play ferma il
  pattern automatico — le due modalità non suonano mai insieme.
- **Rate** (in alto a destra): moltiplicatore globale sulla velocità del pattern —
  `1/2x`, `1x`, `Terzine`, `2x`. Lo stesso pattern suona a metà, a doppio o in terzine
  senza doverne scrivere una variante.
- **Swing / Gate / Ottava** (sotto la riga di progressione): tre controlli globali che si
  sommano a quelli del pattern invece di sostituirli. **Swing** ritarda gli step in
  levare, **Gate** allunga o accorcia ogni nota (25%-150%), **Ottava** trasla l'intero
  voicing di ±2 ottave.
- **Playhead**: la striscia sottile in fondo al riquadro del pattern mostra il punto in
  cui si trova l'esecuzione dentro il loop corrente.
- **Automazione host**: accordo attivo, famiglia attiva, intensità di ogni cella della
  griglia, rate, swing, gate e ottava sono tutti parametri automatizzabili — visibili
  nella lane di automazione della DAW, non solo controllabili da questa UI. Un cambio
  fatto dall'host (automazione, o un altro controller che scrive lo stesso parametro) si
  riflette sulla UI entro un frame o due, esattamente come un keyswitch.

## Personalizzare i pattern

I pattern non sono cablati nel codice: il plugin li legge da un file JSON che puoi
modificare, senza ricompilare nulla (`SPEC.md` §5.4). Al primo avvio viene creato con i 16
pattern di default, e da quel momento ha la precedenza sui pattern embeddati nel binario:

- **Windows**: `Documenti\SmartPlay\patterns.json`
- **macOS**: `~/Documents/SmartPlay/patterns.json`
- **Linux**: `~/Documents/SmartPlay/patterns.json`

Modifica il file, poi riapri il plugin (o ricarica la sessione) per vedere l'effetto. Se il
JSON contiene errori il plugin non si rompe: torna silenziosamente ai pattern di default.
Per ripartire da zero, cancella il file: verrà riscritto al prossimo avvio.

### Campi che caratterizzano lo strumento

| Campo | Effetto |
|---|---|
| `noteOrderSequence` | Indici nel voicing: `0` = fondamentale, `1` = terza, `2` = quinta… Negativo o oltre la dimensione = ottava sotto/sopra |
| `rhythmGrid` | Durata di ogni step in frazioni di battuta (`0.25` = un sedicesimo) |
| `gateLength` | Quanto della durata viene realmente suonato: `0.3` staccato, `1.0` legato |
| `velocityCurve` | Velocity per step, per gli accenti |
| `strumOffsetMs` | **Chitarra**: ritardo progressivo tra le note dello stesso step — è ciò che simula la strimpellata |
| `strumDirection` | `"up"` (grave→acuto), `"down"` (acuto→grave) o `"alternate"`: alterna a ogni step, come una strimpellata vera |
| `palmMute` | **Chitarra**: array di `true`/`false` per step. Uno step mutato suona più corto e più piano, come col palmo appoggiato sulle corde |
| `octaveSpread` | Numero di ottave su cui il pattern sale (o scende, se negativo) nell'arco di un ciclo |
| `loopLength` | Quante volte `rhythmGrid` si ripete prima che il ciclo ricominci: a ogni ripetizione la sequenza ruota di un gruppo, così due battute non suonano identiche |
| `swingAmount` | Ritarda gli step in levare (0–1) |
| `humanizeTiming` / `humanizeVelocity` | Variazione casuale di attacco (ms) e dinamica |
| `crescendoCurve` | **Archi**: crescendo lungo il loop. Scala le velocity **e** invia una rampa di CC11 (expression), perché su una nota tenuta la sola velocity non basta a far gonfiare il suono |

Una **pausa** si scrive con `null` al posto di un indice: lo step consuma il suo tempo
senza suonare. È ciò che fa respirare un accompagnamento — ed è anche il modo di ottenere
una **strimpellata parziale**: dentro uno step che raggruppa più note, un `null` toglie
una corda dalla pennata.

Il numero di note per step si ricava da `noteOrderSequence.size() / rhythmGrid.size()`:
è la leva che distingue un arpeggio da un accordo pieno.

```jsonc
// Accordo pieno: 3 indici su 1 solo step -> suonano insieme
{ "noteOrderSequence": [0, 1, 2],    "rhythmGrid": [1.0],            "gateLength": [0.95] }

// Arpeggio: 3 indici su 3 step -> una nota per battuta
{ "noteOrderSequence": [0, 1, 2],    "rhythmGrid": [1.0, 1.0, 1.0],  "gateLength": [0.95, 0.95, 0.95] }

// Strimpellata alternata giù/su, con attacco leggermente irregolare
{ "noteOrderSequence": [0, 1, 2, 3], "rhythmGrid": [1.0],
  "strumOffsetMs": 8, "strumDirection": "alternate", "humanizeTiming": 3 }

// Con una pausa sul terzo step: l'accompagnamento respira
{ "noteOrderSequence": [0, 0, null, -1], "rhythmGrid": [0.5, 0.25, 0.25, 0.5] }
```

> **Limite attuale**: la griglia Autoplay ha 4 righe per famiglia (`SPEC.md` §5.1), quindi
> il plugin usa esattamente 16 combinazioni `instrumentFamily` × `intensityLevel` (0-3).
> Puoi ridefinire liberamente tutte e 16, ma per *aggiungerne* altre e sceglierle servirebbe
> un selettore di pattern, che non c'è ancora.

## UI

`/ui/Components` contiene i componenti JUCE riusabili (switcher famiglia, riga di 9 pad accordo,
griglia Autoplay 9×4, readout del pattern), che seguono il mockup in
`docs/mockup-v2-garageband-style.html` e operano per riferimento su `ChordBankModule` /
`AutoplayGridState` / `PatternLibrary` — cosi' lo stesso `AutoplayGridPanel` e' condiviso sia
dall'harness standalone di sviluppo (`/ui/Source`, stato posseduto localmente) sia dal vero
plugin (`/plugin`, stato posseduto da `AudioProcessor`).

```
cmake --build build --target SmartChordArpUI -j
./build/ui/SmartChordArpUI_artefacts/Debug/SmartPlay
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
| `SmartChordArpInst` | Strumento (VSTi, uscita audio silenziosa in una DAW) | Host che non ospitano i MIDI FX VST3 — **Ableton Live** |

In Ableton usa la variante *Inst*: caricala su una traccia MIDI, poi sulla traccia dello
strumento vero imposta **MIDI From → \<traccia\> → SmartPlay Inst**.

Il vero standalone (l'app autonoma, non l'harness `SmartChordArpUI` di sviluppo) e' la build
Standalone di `SmartChordArpInst`: e' l'unica con un bus audio, quindi l'unica che puo' avere
un dispositivo audio a valle. Contiene un piccolo synth di anteprima incorporato
(`plugin/Source/PreviewSynth.h`, due sinusoidi con inviluppo ADSR in stile "toy piano") che
suona **solo** quando il binario gira come vero standalone (`wrapperType ==
wrapperType_Standalone`): lo stesso identico `.vst3` caricato in una DAW resta silenzioso come
prima, per non violare il vincolo MIDI-only di `CLAUDE.md`/`SPEC.md` sezione 1 li' dove conta.

```
cmake --build build --target SmartChordArp_VST3 -j         # bundle .vst3 (MIDI FX)
cmake --build build --target SmartChordArpInst_VST3 -j     # bundle .vst3 (strumento)
cmake --build build --target SmartChordArpInst_Standalone -j   # eseguibile standalone (con audio)
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

## Licenza

SmartPlay è software libero: puoi ridistribuirlo e/o modificarlo secondo i termini della
**GNU General Public License v3.0** (vedi [`LICENSE`](LICENSE)). In breve: puoi usarlo,
studiarlo, modificarlo e ridistribuirlo liberamente, anche a scopo commerciale — ma se
distribuisci una versione modificata, anche quella deve restare sotto GPLv3, sorgente
incluso.

```
Copyright (C) 2026 esalzari-dot
```

SmartPlay non ha alcuna affiliazione con Apple Inc.: è un progetto indipendente ispirato
alla *categoria* degli Smart Instrument di GarageBand, ricostruito con codice, dataset e
grafica originali (`CLAUDE.md`, `SPEC.md` sezione 1). "GarageBand" resta un marchio Apple,
citato qui solo a scopo descrittivo.

### Sviluppo assistito da AI

Il codice di questo repository è stato scritto da [Claude Code](https://claude.ai/code)
(Anthropic) sotto la direzione, la revisione e l'approvazione dell'autore umano indicato
sopra, che ha definito la specifica (`SPEC.md`), le decisioni architetturali e di design, e
approvato ogni funzionalità prima del merge. Lo dichiariamo per trasparenza verso chi userà
o contribuirà al progetto.
