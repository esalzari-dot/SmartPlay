# Istruzioni per Claude Code — SmartPlay

Questo repository contiene lo sviluppo di un plugin **MIDI FX** VST3/AU in JUCE (C++), ispirato agli Smart Instrument di GarageBand. La specifica completa è in `SPEC.md` — **leggila per intero prima di scrivere qualsiasi codice**. I mockup UI di riferimento sono in `docs/`.

## Vincoli fondamentali (non violare)
- Il plugin è **MIDI-only**: nessun motore audio/sintesi. `isMidiEffect() == true`, nessun output audio, solo MIDI out.
- Non riferirsi mai al codice o agli algoritmi proprietari di GarageBand/Apple: i pattern d'arpeggio nella sezione 5.3 di `SPEC.md` sono ricostruzioni originali basate su concetti musicali generici (non copiare/decompilare nulla).
- La selezione del pattern avviene tramite il modello **Autoplay Grid** (accordo × intensità, sezione 5 di SPEC.md), non tramite una lista piatta di pattern.

## Stack tecnico
- **Linguaggio**: C++17 o superiore
- **Framework**: JUCE (ultima versione stabile)
- **Build system**: CMake (preferito rispetto al Projucer, per portabilità e CI)
- **Test**: Catch2 (o framework equivalente) per unit test su `VoicingEngine`, `ChordBankModule`, `PatternLibrary`
- **Formato dati pattern**: JSON esterno o `BinaryData` embeddato — non hardcodare i pattern in logica if/else

## Convenzioni di codice
- RAII ovunque, evitare `new`/`delete` manuali
- Namespacing chiaro per modulo (es. `smartchord::VoicingEngine`, `smartchord::ArpeggiatorEngine`)
- Ogni modulo descritto in SPEC.md deve essere testabile in isolamento (niente stato globale condiviso tra moduli)
- Commenti solo dove la logica non è ovvia dal nome delle funzioni/variabili

## Ordine di lavoro
Segui rigorosamente l'ordine indicato nella sezione 10 di `SPEC.md` ("Ordine di sviluppo consigliato"). Non passare alla UI prima che `VoicingEngine`, `PatternLibrary` e `ArpeggiatorEngine` abbiano test funzionanti.

## Prima di ogni sessione
1. Rileggi `SPEC.md` per il modulo su cui stai per lavorare
2. Se stai lavorando sulla UI, apri i mockup in `docs/` come riferimento visivo (non serve renderizzarli, sono riferimento statico)
3. Verifica che le modifiche non rompano i test esistenti prima di proporre nuovo codice
