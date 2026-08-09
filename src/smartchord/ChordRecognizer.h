#pragma once

#include "smartchord/ChordDefinition.h"

#include <optional>
#include <vector>

namespace smartchord
{

// Riconosce l'accordo suonato su una tastiera MIDI, cosi' che gli 8 slot del banco non
// siano l'unico modo di scegliere un accordo: si puo' semplicemente suonarlo.
//
// Il riconoscimento lavora sulle classi di altezza (nota modulo 12) piu' il basso: e' cio'
// che permette di riconoscere lo stesso accordo indipendentemente da come lo si distribuisce
// sulla tastiera. Il rivolto viene dedotto dalla nota piu' grave.
//
// Serve un minimo di note perche' il risultato abbia senso: due note sole sono ambigue
// (una terza maggiore appartiene a decine di accordi diversi), quindi sotto la soglia la
// funzione non azzarda e restituisce nullopt - meglio nessun cambio d'accordo che uno
// sbagliato a ogni nota di passaggio.
constexpr size_t minimumNotesForRecognition = 3;

// heldNotes sono numeri di nota MIDI, in qualunque ordine e con eventuali duplicati.
// Restituisce nullopt se non ci sono abbastanza note o se nessun accordo e' abbastanza
// vicino a quelle suonate.
std::optional<ChordDefinition> recognizeChord (const std::vector<int>& heldNotes);

} // namespace smartchord
