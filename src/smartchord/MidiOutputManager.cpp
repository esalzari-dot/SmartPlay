#include "smartchord/MidiOutputManager.h"

namespace smartchord
{

void MidiOutputManager::handleEvent (const NoteEvent& event)
{
    if (event.kind == NoteEvent::Kind::ControlChange)
        return; // i control change non aprono ne' chiudono note

    if (event.kind == NoteEvent::Kind::NoteOn)
    {
        ++activeNoteCounts[event.midiNote];
        return;
    }

    auto it = activeNoteCounts.find (event.midiNote);
    if (it == activeNoteCounts.end())
        return;

    if (--(it->second) <= 0)
        activeNoteCounts.erase (it);
}

bool MidiOutputManager::isNoteActive (int midiNote) const
{
    return activeNoteCounts.find (midiNote) != activeNoteCounts.end();
}

std::vector<int> MidiOutputManager::getActiveNotes() const
{
    std::vector<int> notes;
    notes.reserve (activeNoteCounts.size());
    for (const auto& entry : activeNoteCounts)
        notes.push_back (entry.first);
    return notes;
}

std::vector<NoteEvent> MidiOutputManager::allNotesOff (double atBeatPosition)
{
    std::vector<NoteEvent> offEvents;
    offEvents.reserve (activeNoteCounts.size());

    for (const auto& entry : activeNoteCounts)
        offEvents.push_back ({ NoteEvent::Kind::NoteOff, entry.first, 0, atBeatPosition });

    activeNoteCounts.clear();
    return offEvents;
}

} // namespace smartchord
