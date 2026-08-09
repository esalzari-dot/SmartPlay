#pragma once

#include "smartchord/ChordBankModule.h"

#include <array>
#include <string>
#include <vector>

namespace smartchord
{

// Configurazione dei 9 pad salvata dall'utente con un nome, per richiamarla in un altro
// progetto/sessione. A differenza di ChordProgressions (otto giri d'armonia pronti,
// incorporati nel plugin), un preset e' scritto e riletto da un file su disco: SPEC.md
// non lo tratta esplicitamente, ma il meccanismo di applicazione al banco e' lo stesso di
// ChordProgressions::buildChordBank - sostituisce in blocco il contenuto degli slot.
struct ChordBankPreset
{
    std::string name;
    std::array<ChordDefinition, numChordBankSlots> chords{};
};

// Analizza un elenco di preset da una stringa JSON (lo stesso formato scritto da
// serializeChordBankPresets). Una stringa vuota, o un array vuoto, restituisce un
// vettore vuoto. Un preset con meno di 9 accordi viene completato con ChordDefinition{}
// di default; uno con piu' di 9 viene troncato - tollerante verso un file modificato a
// mano, come PatternLibrary lo e' verso patterns.json. Lancia std::runtime_error solo se
// il JSON stesso non e' valido o manca il campo "name".
std::vector<ChordBankPreset> parseChordBankPresets (const std::string& json);

// Serializza l'elenco nello stesso formato letto da parseChordBankPresets, indentato per
// restare leggibile/modificabile a mano.
std::string serializeChordBankPresets (const std::vector<ChordBankPreset>& presets);

// Sostituisce in blocco il contenuto degli slot con quello del preset. Come
// ChordProgressions::buildChordBank, non tocca lo slot attivo: il chiamante lo imposta
// se necessario.
ChordBankModule chordBankFromPreset (const ChordBankPreset& preset);

// L'inverso: cattura il contenuto attuale di un banco (non lo slot attivo) come preset
// con questo nome.
ChordBankPreset presetFromChordBank (const std::string& name, const ChordBankModule& bank);

} // namespace smartchord
