#pragma once

#include "smartchord/ChordBankModule.h"

#include <string>
#include <vector>

namespace smartchord
{

// Progressione pronta da caricare negli 8 slot del banco. Riempire otto pad a mano prima
// di poter provare un'idea e' il primo attrito che si incontra usando il plugin: questi
// preset lo tolgono di mezzo.
//
// I gradi sono espressi in semitoni sopra la tonica insieme alla loro qualita', non come
// accordi assoluti, cosi' la stessa progressione si puo' trasporre in qualunque tonalita'.
struct ProgressionDegree
{
    int semitonesAboveTonic = 0;
    ChordQuality quality = ChordQuality::Maj;
};

struct ChordProgression
{
    std::string id;
    std::string displayName;
    std::vector<ProgressionDegree> degrees;
};

// Progressioni di riferimento, in ordine di utilita' generale.
const std::vector<ChordProgression>& getChordProgressions();

const ChordProgression* findChordProgression (const std::string& id);

// Costruisce un banco di 8 accordi dalla progressione, trasposta nella tonalita' data
// (tonicSemitone 0-11, C=0). Una progressione piu' corta di 8 slot si ripete ciclicamente:
// gli 8 pad restano tutti utilizzabili invece di lasciarne metà vuoti.
ChordBankModule buildChordBank (const ChordProgression& progression, int tonicSemitone);

} // namespace smartchord
