# SmartPlay — Project Spec

## 1. Panoramica

Plugin **MIDI FX** (VST3/AU) per DAW desktop, scritto in **JUCE (C++)**, ispirato agli Smart Instrument di GarageBand per iOS.

**Importante — natura del plugin:** questo è un plugin *MIDI-only*, senza motore audio/sintesi proprio. Genera eventi MIDI (note on/off, velocity, timing) da instradare verso un VST strumento esterno (piano, basso, chitarra, archi campionati, ecc.) posizionato subito dopo nella catena della DAW:

```
[Smart Chord & Arp — genera MIDI] → [VST strumento esterno — genera audio]
```

In JUCE: `AudioProcessor` con `isMidiEffect() == true`, nessun output audio, bus MIDI out abilitato.

Funzionalità core:
- L'utente definisce almeno 8 slot-accordo, selezionabili in tempo reale (pad/keyswitch)
- Per ogni accordo attivo, un arpeggiatore genera una sequenza ritmica di note secondo un pattern selezionabile
- Il pattern non si sceglie da una lista piatta, ma tramite una **griglia Autoplay accordo × intensità** (vedi sezione 5) — replica della UX di GarageBand Smart Instrument
- I pattern sono organizzati per famiglia strumentale (Piano, Basso, Chitarra, Archi) — non generano suoni diversi, ma voicing e ritmi musicalmente coerenti con lo strumento che l'utente collegherà a valle

### Riferimento UI/UX
Vedi `docs/mockup-v2-garageband-style.html` (mockup di riferimento, aperto in un browser) per il comportamento atteso della griglia Autoplay. `docs/mockup-v1-hardware.html` è una variante scartata, tenuta come riferimento storico.

---

## 2. Architettura dei moduli

```
PluginProcessor (AudioProcessor, MIDI effect)
├── ChordBankModule       → gestisce gli slot accordo (min. 8)
├── VoicingEngine          → converte ChordDefinition → note MIDI concrete per famiglia strumentale
├── PatternLibrary         → dataset dei pattern di arpeggio, organizzati per famiglia + livello di intensità
├── AutoplayGridState      → stato della griglia: per ogni (slot accordo, famiglia) l'intensità selezionata
├── ArpeggiatorEngine      → sequencer step-based che applica il PatternDefinition risolto dalla griglia
├── PlayStripEngine        → modalità gestuale alternativa all'Autoplay, per accordo (sezione 5.5)
├── SyncClock              → aggancio al BPM/PPQ dell'host, quantizzazione, swing
└── MidiOutputManager      → gestisce note-off puliti, panic/all-notes-off al cambio accordo
```

Comunicazione tra moduli via eventi MIDI interni, non stato condiviso diretto — ogni modulo deve essere testabile in isolamento.

---

## 3. ChordBankModule

Ogni slot accordo è definito da:

```
ChordDefinition {
  rootSemitone: int        // 0-11, C=0
  quality: ChordQuality    // enum, vedi tabella intervalli sotto
  inversion: int           // 0 = fondamentale, 1 = primo rivolto, ecc.
  octaveOffset: int        // ottava base del voicing
}
```

- Minimo 8 slot, selezionabili via UI pad, MIDI note trigger, o keyswitch
- Serializzabili in `AudioProcessorValueTreeState` per salvataggio preset/stato DAW

### Tabella intervalli per ChordQuality (semitoni dalla root)

| Quality | Intervalli |
|---|---|
| maj | 0, 4, 7 |
| min | 0, 3, 7 |
| dim | 0, 3, 6 |
| aug | 0, 4, 8 |
| sus2 | 0, 2, 7 |
| sus4 | 0, 5, 7 |
| maj7 | 0, 4, 7, 11 |
| min7 | 0, 3, 7, 10 |
| dom7 | 0, 4, 7, 10 |
| min7b5 | 0, 3, 6, 10 |
| dim7 | 0, 3, 6, 9 |
| add9 | 0, 4, 7, 14 |
| six | 0, 4, 7, 9 |
| nine | 0, 4, 7, 10, 14 |

---

## 4. VoicingEngine

Traduce `ChordDefinition` + `InstrumentFamily` → array di note MIDI assolute, tramite `VoicingProfile` specifico per strumento.

**Pipeline:**
1. `getChordTones(quality)` → intervalli in semitoni (tabella sopra)
2. `applyInversion(tones, inversion)` → riordina/trasla le note secondo l'inversione
3. `mapToInstrumentRange(tones, profile)` → applica i vincoli del `VoicingProfile`

```
VoicingProfile {
  instrumentFamily: enum { Piano, Bass, Guitar, Strings }
  midiRangeLow: int
  midiRangeHigh: int
  maxNotes: int              // quante note dell'accordo mantenere
  minSpacingSemitones: int   // spaziatura minima tra note adiacenti
  maxSpacingSemitones: int   // spaziatura massima (per spread voicing)
  allowDoubling: bool        // raddoppio di root/ottava
  voicingStyle: enum { Block, Spread, Monophonic }
}
```

### Profili di riferimento

- **Piano**: range C2–C6, `voicingStyle = Block`, `maxNotes` = tutte le note dell'accordo, spaziatura libera entro il blocco
- **Basso**: range E1–G3, `voicingStyle = Monophonic`, `maxNotes = 1-2` (tipicamente solo root, o root+quinta per pattern walking)
- **Chitarra**: range E2–E5, `voicingStyle = Block` con `maxSpacingSemitones` vincolato per restare "suonabile" (evita voicing innaturali)
- **Archi**: range ampio (violino/viola/cello), `voicingStyle = Spread`, note distanziate su ottave diverse per sezione

Output: array di note MIDI ordinate + flag sulla "top note" (utile all'ArpeggiatorEngine per pattern che enfatizzano la nota più acuta).

---

## 5. PatternLibrary + Autoplay Grid + ArpeggiatorEngine

### 5.1 Modello "Autoplay Grid" (accordo × intensità)

Questo è l'elemento UX centrale, mutuato da GarageBand Smart Instrument: **non esiste un selettore di pattern a lista piatta**. Per ogni combinazione (slot accordo, famiglia strumentale) esiste una colonna nella griglia, con **4 livelli di intensità** crescente dal basso (semplice) verso l'alto (complesso). L'utente tocca/clicca una cella della griglia per scegliere contemporaneamente l'accordo (colonna) e l'intensità del pattern (riga).

```
AutoplayGridState {
  // per ogni InstrumentFamily, un array [8] di livelli intensità (0-3), uno per slot accordo
  intensityByChordSlot: Map<InstrumentFamily, int[8]>
}
```

Il livello di intensità selezionato mappa a un `PatternDefinition` specifico tramite:

```
PatternDefinition resolvePattern(InstrumentFamily family, int intensityLevel)
```

### 5.2 PatternDefinition

```
PatternDefinition {
  id: String
  displayName: String
  instrumentFamily: enum { Piano, Bass, Guitar, Strings }
  intensityLevel: int              // 0 (più semplice) - 3 (più complesso)

  noteOrderSequence: int[]      // indice della nota nel voicing (0=root, 1=terza, 2=quinta...)
                                  // negativo = ottava sotto, oltre size = ottava sopra
  rhythmGrid: float[]             // durata step in frazioni di beat (0.25 = 1/16)
  gateLength: float[]             // % della durata effettivamente suonata (staccato/legato)
  velocityCurve: int[]            // velocity MIDI per step (accenti)
  octaveSpread: int
  loopLength: int
  humanizeTiming: float           // ms variazione random
  humanizeVelocity: int
  swingAmount: float              // 0-1
  strumOffsetMs: float            // opzionale, solo chitarra: offset crescente tra note quasi-simultanee
  crescendoCurve: bool            // opzionale, solo archi
}
```

### 5.3 Le 4 intensità per famiglia (dataset di riferimento)

Ogni pattern ha anche `humanizeTiming`/`humanizeVelocity` (spenti da `Humanize`, sezione 8,
quando l'utente li disattiva) tranne dove specificato altrimenti; omessi qui sotto per
brevità. I livelli 0 di ogni famiglia sono un accordo/nota tenuti per un'intera battuta
(`rhythmGrid:[4.0]`, un solo step) invece di note separate in sequenza — altrimenti
"sostenuto" scatterebbe come un lento arpeggio a tre note anziché un accordo vero.

**Piano** (0→3): As Played · Up-Down · Alberti Bass · Broken Chord Classic
```
0 As Played:            noteOrderSequence:[0,1,2], rhythmGrid:[4.0], gateLength:[0.97] (accordo tenuto per battuta)
1 Up · Down:             noteOrderSequence:[0,1,2,1], rhythmGrid:[0.5]*4, gateLength:[0.85]*4
2 Alberti Bass:          noteOrderSequence:[0,2,1,2], rhythmGrid:[0.25]*4, gateLength:[0.9]*4, swingAmount:0.05
3 Broken Chord Classic:  noteOrderSequence:[0,1,2,3,2,1], rhythmGrid:[0.25]*6, gateLength:[0.85]*6, octaveSpread:1, loopLength:2 (sale fino all'ottava e ridiscende)
```

**Basso** (0→3): Root Sostenuto · Root-Fifth Alternato · Walking Pop · Walking Sincopato
```
0 Root Sostenuto:        noteOrderSequence:[0], rhythmGrid:[4.0], gateLength:[0.95] (root tenuta per battuta)
1 Root-Fifth Alternato:  noteOrderSequence:[0,2], rhythmGrid:[0.5,0.5], gateLength:[0.85,0.85]
2 Walking Pop:           noteOrderSequence:[0,0,null,-1], rhythmGrid:[0.5,0.25,0.25,0.5], gateLength:[0.7,0.5,0.5,0.7]
3 Walking Sincopato:     noteOrderSequence:[0,2,0,-1,2], rhythmGrid:[0.375,0.125,0.25,0.125,0.125], swingAmount:0.15
```

**Chitarra** (0→3): Pad Sostenuto · Strumming Simulato · Up-Down Picking · Travis Picking
```
0 Pad Sostenuto:         noteOrderSequence:[0,1,2,3], rhythmGrid:[4.0], strumOffsetMs:4, gateLength:[0.95] (accordo tenuto per battuta)
1 Strumming Simulato:    noteOrderSequence:[0,1,2,3], rhythmGrid:[1.0], strumOffsetMs:8, gateLength:[0.92], strumDirection:alternate
2 Up · Down Picking:     noteOrderSequence:[0,1,2,3,2,1], rhythmGrid:[0.25]*6, gateLength:[0.7]*6, palmMute alternato, swingAmount:0.08
3 Travis Picking:        noteOrderSequence:[0,2,1,2,0,2,1,2], rhythmGrid:[0.25]*8, gateLength:[0.6]*8, swingAmount:0.1, loopLength:2
```

**Archi** (0→3): Sostenuto Legato · Legato Mosso · Tremolo Leggero · Tremolo Pizzicato
```
0 Sostenuto Legato:      noteOrderSequence:[0,1,2], rhythmGrid:[4.0], gateLength:[1.0], crescendoCurve:true (accordo tenuto per battuta, con swell)
1 Legato Mosso:          noteOrderSequence:[0,1,2,1], rhythmGrid:[0.5]*4, gateLength:[0.95]*4
2 Tremolo Leggero:       noteOrderSequence:[0,1,2,1], rhythmGrid:[0.125]*4, gateLength:[0.45]*4 (attraversa l'accordo, non solo la root)
3 Tremolo Pizzicato:     noteOrderSequence:[0,2,1,2], rhythmGrid:[0.125]*4, gateLength:[0.25]*4, octaveSpread:1, humanizeVelocity:8 (idem)
```

### 5.4 ArpeggiatorEngine
- Sequencer step-based sincronizzato a `SyncClock`
- Ad ogni step: legge lo slot accordo attivo + la sua intensità da `AutoplayGridState` → risolve il `PatternDefinition` tramite `resolvePattern()` → applica il voicing del `VoicingEngine` → applica `rhythmGrid`, `gateLength`, `velocityCurve`, `humanize*`, `swingAmount` → invia MIDI note on/off
- Deve gestire correttamente il cambio di accordo o di intensità a metà pattern (vedi `MidiOutputManager`)

Consiglio: serializzare l'intera `PatternLibrary` come dataset esterno (JSON, o `BinaryData` embeddato in JUCE) per poterla espandere senza ricompilare.

### 5.5 Play Strip (modalità gestuale, alternativa all'Autoplay)

Secondo modo di far suonare un accordo, alternativo — non sostitutivo — all'Autoplay Grid:
invece di un `PatternDefinition` preregistrato, l'utente suona in tempo reale muovendo il
puntatore (o il dito, su touch) lungo una barra a "tacche", una per ogni tono dell'accordo.
Ogni pad accordo espone entrambe le modalità (toggle Autoplay/Play); passare all'una o
all'altra non cambia l'accordo selezionato, solo come viene eseguito.

**Nota sui riferimenti**: questa modalità replica un *concetto* di interazione (tap = nota
singola, trascinamento = passaggio legato/arco) osservato negli Smart Instrument di
GarageBand, ricostruito qui in modo originale — non copia asset, codice o algoritmi Apple
(vedi `CLAUDE.md`).

```
StripGesture {
  phase: enum { Down, Move, Up }
  position: float           // 0.0-1.0 lungo la barra
  timestampSeconds: double
}

StripNoteEvent {
  kind: enum { NoteOn, NoteOff }
  midiNote: int
  velocity: int              // derivata dalla velocità di attraversamento delle tacche
  timestampSeconds: double
}
```

**Numero di tacche per famiglia** (fisso, non dipende da quante note dell'accordo entrano
nella tessitura):

| Famiglia | Tacche | Perché |
|---|---|---|
| Piano | 7 | la scala intera dell'accordo (un'ottava di gradi) |
| Chitarra | 6 | come le 6 corde dello strumento reale |
| Basso | 4 | come le 4 corde dello strumento reale |
| Archi | 4 | come le 4 corde di violino/viola/violoncello/contrabbasso |

**Tacche → note**: le tacche non sono i soli *chord tones* dell'accordo (3-4 note), ma la
sua **scala implicita** — `getChordScaleTones(quality)`, 7 gradi diatonici che contengono i
chord tones (per gli accordi diminuiti di settima, simmetrici, li approssima: non esiste
una scala diatonica di 7 gradi che li contenga esattamente). La scala si estende su più
ottave entro la tessitura della famiglia (stesso `VoicingProfile` di `VoicingEngine`,
sezione 4), poi si campiona a un numero di valori equidistanti pari alle tacche della
tabella sopra: un basso a 4 tacche copre la stessa estensione grave-acuto di un piano a 7,
solo con meno fermate. `position` si arrotonda all'indice di tacca più vicino:
`notchIndex = round(position * (numNotches - 1))`.

**Nome dell'accordo → accordo completo**: oltre alle tacche, la barra espone anche
un'etichetta col nome dell'accordo (es. "Fm"), sempre visibile a un'estremità. Toccarla
suona l'accordo intero così come lo suonerebbe `VoicingEngine::voiceChord()` (stesso
voicing usato altrove, basso compreso) invece di una singola nota della scala — è
un'azione a parte, non passa da `PlayStripEngine`: non tocca lo stato delle tacche
(nessun `notchIndex` coinvolto), quindi non ha bisogno di un caso speciale nel motore.
`Down` sull'etichetta suona tutte le note del voicing insieme, `Up` le chiude.

**Regola di articolazione** (unica per tutte le famiglie: cambia solo il *risultato sonoro* a
valle, non la logica che lo produce — SmartPlay resta MIDI-only, sezione 1: non decide lui
il timbro, lo decide lo strumento campionato collegato dopo):
- `Down` seguito da `Up` **senza** attraversare un'altra tacca → una nota sola, tenuta per
  tutta la durata della pressione (`NoteOn` a `Down`, `NoteOff` a `Up`). Per Archi è
  pizzicato, per Piano/Chitarra/Basso una nota normale: la differenza timbrica non la fa
  SmartPlay, la fa il patch campionato a valle (spesso già pensato per decadere da solo su
  una nota pizzicato, indipendentemente da quanto resta premuta).
- `Down` con `Move` che attraversa una o più tacche prima di `Up` → ogni attraversamento
  ritriggera: `NoteOff` sulla tacca precedente + `NoteOn` sulla nuova, per tutta la durata
  della pressione ("streaming"/arco). La `velocity` di ogni retrigger è derivata dalla
  velocità di attraversamento (tacche al secondo): gesto più rapido → velocity più alta,
  entro `[minVelocity, maxVelocity]`.
- Non c'è una durata "finta" imposta dal motore: `PlayStripEngine` è puramente reattivo agli
  eventi gesto in ingresso, nessun timer interno. Un'eventuale commutazione d'articolazione
  esplicita (CC/keyswitch per patch orchestrali che la supportano) resta un'estensione
  futura, fuori dal perimetro di questa prima versione.

**Differenza da ArpeggiatorEngine**: `PlayStripEngine` non è agganciato a `SyncClock` e non
schedula nulla in anticipo — reagisce agli eventi gesto man mano che arrivano, è esecuzione
dal vivo, non un pattern. Va tenuto un modulo separato (testabile isolatamente con sequenze
sintetiche di `StripGesture`), non un'estensione di `ArpeggiatorEngine`.

**UI**: `PlayStripRow` — una sola barra **verticale**, non una per accordo (come in
GarageBand): rappresenta l'accordo attualmente attivo (il pad selezionato nella riga sopra,
"Chord Bank"), con le tacche impilate dal basso (grave) verso l'alto (acuto), come un manico
o una tastiera visti di fronte — non una riga letta da sinistra a destra. Selezionare un
altro pad sopra cambia l'accordo rappresentato dalla barra, sia in Autoplay sia in Play;
sostituisce visivamente l'Autoplay Grid quando la modalità è Play, nella stessa area
riservata del pannello. Su desktop, senza multitouch: click singolo = `Down`+`Up` senza
movimento; click e trascina (in verticale) = `Down`→`Move`→`Up`, stesso gesto della barra
touch ma con il mouse, posizione 0 in basso e 1 in alto invece che 0 a sinistra e 1 a destra.

---

## 6. SyncClock
- Aggancio a `AudioPlayHead` dell'host per BPM e posizione PPQ
- Quantizzazione step selezionabile (1/4, 1/8, 1/16, 1/8T, ecc.)
- Swing globale, combinabile con `swingAmount` del pattern

## 7. MidiOutputManager
- Note-off puliti quando l'arpeggiatore si ferma o cambia pattern/intensità
- **All-notes-off / panic** al cambio di accordo (per evitare note "appese" quando si passa da uno slot all'altro a metà pattern)
- Gestione del MIDI output bus verso l'host (VST3 supporta nativamente il routing MIDI-out → altro plugin; in AU dipende dall'host, es. Logic Pro lo supporta bene)

## 8. Stato & parametri
`AudioProcessorValueTreeState` per: accordo attivo, intensità/pattern per (accordo, famiglia), rate, gate length globale, swing, octave range, instrument family attivo — tutti automatizzabili dall'host.

## 9. UI — riferimento GarageBand Autoplay Grid

Layout della UI (vedi `docs/mockup-v2-garageband-style.html`):
- Switcher segmentato per famiglia strumentale (Piano/Basso/Chitarra/Archi) in alto
- Riga di 8 pulsanti accordo, selezionabili singolarmente
- Griglia Autoplay sottostante: 8 colonne (una per accordo) × 4 righe (intensità, dal basso=semplice all'alto=complesso)
- Il tocco/click su una cella seleziona sia l'accordo (colonna) sia l'intensità (riga) contemporaneamente
- Readout in basso: nome del pattern risolto, famiglia, intensità corrente, routing MIDI verso il VST esterno
- Palette colore d'accento diversa per famiglia strumentale (identità visiva per ciascuna colonna sonora)

---

## 10. Ordine di sviluppo consigliato

1. `ChordDefinition` + tabella intervalli + test unitari su `getChordTones`
2. `VoicingEngine` con `VoicingProfile` per le 4 famiglie + test (dato un accordo, verificare le note generate per ciascuno strumento)
3. `PatternLibrary` come dataset dati (JSON) con le 16 pattern definitions (4 famiglie × 4 intensità) della sezione 5.3 + parser
4. `AutoplayGridState` + `resolvePattern()`
5. `ArpeggiatorEngine` come sequencer step-based agganciato a `SyncClock`
6. `MidiOutputManager` (panic/note-off al cambio accordo/intensità)
7. UI: switcher famiglia + 8 pad accordo + griglia Autoplay 8×4 (vedi mockup in `docs/`)
8. Integrazione VST3/AU wrapper JUCE, test in DAW reale con VST strumento a valle
9. `PlayStripEngine` (sezione 5.5) + test con sequenze sintetiche di `StripGesture` + UI del
   toggle Autoplay/Play e della barra a tacche
