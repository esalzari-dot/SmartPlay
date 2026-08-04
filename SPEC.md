# Smart Chord & Arpeggiator — Project Spec

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
Vedi `/docs/mockup-v2-garageband-style.html` (mockup di riferimento, aperto in un browser) per il comportamento atteso della griglia Autoplay. `/docs/mockup-v1-hardware.html` è una variante scartata, tenuta come riferimento storico.

---

## 2. Architettura dei moduli

```
PluginProcessor (AudioProcessor, MIDI effect)
├── ChordBankModule       → gestisce gli slot accordo (min. 8)
├── VoicingEngine          → converte ChordDefinition → note MIDI concrete per famiglia strumentale
├── PatternLibrary         → dataset dei pattern di arpeggio, organizzati per famiglia + livello di intensità
├── AutoplayGridState      → stato della griglia: per ogni (slot accordo, famiglia) l'intensità selezionata
├── ArpeggiatorEngine      → sequencer step-based che applica il PatternDefinition risolto dalla griglia
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

**Piano** (0→3): As Played · Up-Down · Alberti Bass · Broken Chord Classic
```
0 As Played:            noteOrderSequence:[0,1,2], rhythmGrid:[1.0,1.0,1.0], gateLength:[0.95]*3
1 Up · Down:             noteOrderSequence:[0,1,2,1], rhythmGrid:[0.5]*4, gateLength:[0.85]*4
2 Alberti Bass:          noteOrderSequence:[0,2,1,2], rhythmGrid:[0.25]*4, gateLength:[0.9]*4
3 Broken Chord Classic:  noteOrderSequence:[0,1,2,1], rhythmGrid:[0.25]*4, gateLength:[0.9]*4, velocityCurve:[100,75,85,75]
```

**Basso** (0→3): Root Sostenuto · Root-Fifth Alternato · Walking Pop · Walking Sincopato
```
0 Root Sostenuto:        noteOrderSequence:[0], rhythmGrid:[1.0], gateLength:[0.9]
1 Root-Fifth Alternato:  noteOrderSequence:[0,2], rhythmGrid:[0.5,0.5], gateLength:[0.85,0.85]
2 Walking Pop:           noteOrderSequence:[0,0,2,-1], rhythmGrid:[0.5,0.25,0.25,0.5], gateLength:[0.7,0.5,0.5,0.7]
3 Walking Sincopato:     noteOrderSequence:[0,2,0,-1,2], rhythmGrid:[0.375,0.125,0.25,0.125,0.125], swingAmount:0.15
```

**Chitarra** (0→3): Pad Sostenuto · Strumming Simulato · Up-Down Picking · Travis Picking
```
0 Pad Sostenuto:         noteOrderSequence:[0,1,2,3], rhythmGrid:[1.0], strumOffsetMs:4, gateLength:[1.0]
1 Strumming Simulato:    noteOrderSequence:[0,1,2,3], rhythmGrid:[1.0], strumOffsetMs:8, gateLength:[0.95], humanizeTiming:3
2 Up · Down Picking:     noteOrderSequence:[0,1,2,3,2,1], rhythmGrid:[0.25]*6, gateLength:[0.7]*6
3 Travis Picking:        noteOrderSequence:[0,2,1,2,0,2,1,2], rhythmGrid:[0.25]*8, gateLength:[0.6]*8, swingAmount:0.1
```

**Archi** (0→3): Sostenuto Legato · Legato Mosso · Tremolo Leggero · Tremolo Pizzicato
```
0 Sostenuto Legato:      noteOrderSequence:[0,1,2], rhythmGrid:[1.0,1.0,1.0], gateLength:[1.0]*3, crescendoCurve:true
1 Legato Mosso:          noteOrderSequence:[0,1,2,1], rhythmGrid:[0.5]*4, gateLength:[0.95]*4
2 Tremolo Leggero:       noteOrderSequence:[0,0], rhythmGrid:[0.25,0.25], gateLength:[0.5,0.5]
3 Tremolo Pizzicato:     noteOrderSequence:[0,0,0,0], rhythmGrid:[0.125]*4, gateLength:[0.3]*4, humanizeVelocity:8
```

### 5.4 ArpeggiatorEngine
- Sequencer step-based sincronizzato a `SyncClock`
- Ad ogni step: legge lo slot accordo attivo + la sua intensità da `AutoplayGridState` → risolve il `PatternDefinition` tramite `resolvePattern()` → applica il voicing del `VoicingEngine` → applica `rhythmGrid`, `gateLength`, `velocityCurve`, `humanize*`, `swingAmount` → invia MIDI note on/off
- Deve gestire correttamente il cambio di accordo o di intensità a metà pattern (vedi `MidiOutputManager`)

Consiglio: serializzare l'intera `PatternLibrary` come dataset esterno (JSON, o `BinaryData` embeddato in JUCE) per poterla espandere senza ricompilare.

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

Layout della UI (vedi `/docs/mockup-v2-garageband-style.html`):
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
7. UI: switcher famiglia + 8 pad accordo + griglia Autoplay 8×4 (vedi mockup in `/docs`)
8. Integrazione VST3/AU wrapper JUCE, test in DAW reale con VST strumento a valle
