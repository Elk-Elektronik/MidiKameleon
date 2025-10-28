#include "ChordGenEffect.h"
#include "Utils.h"

void ChordGenEffect::handleMidiMessage(
  bool isActive, 
  midi::MidiType type, 
  midi::DataByte data1,
  midi::DataByte data2, 
  midi::Channel channel
) {
  switch (type) {
  case midi::MidiType::Clock: // Has seperate handler
    break;
  case midi::MidiType::ActiveSensing: // Has seperate handler
    break;
  case midi::MidiType::Start: // Send start
    sendMidiStart();
    break;
  case midi::MidiType::Stop: // Send stop
    sendMidiStop();
    break;
  case midi::MidiType::Continue: // Send continue
    sendMidiContinue();
    break;
  case midi::MidiType::NoteOn: {
    // Either send chord or pass through original MIDI
    if (isActive)
      sendChord(type, data1, data2, channel);
    else
      sendMidiBoth(type, data1, data2, channel);

    // Track the last note
    lastNote = data1;
    lastVelocity = data2;
    lastChannel = channel;
    break;
  }
  case midi::MidiType::NoteOff: {
    // Handles stomp bypass while holding chord
    removeOldNotes();

    // Either send chord off or pass through original MIDI
    if (isActive) sendChord(type, data1, data2, channel);
    else sendMidiBoth(type, data1, data2, channel);
    break;
  }
  default: // Pass through any other message
    sendMidiBoth(type, data1, data2, channel);
  }
}

void ChordGenEffect::sendChord(
  midi::MidiType type, 
  midi::DataByte data1, 
  midi::DataByte data2,
  midi::Channel channel
) {
  const Chord_t chord = chordBank[chordIdx];
  for (uint8_t i = 0; i < chord.count; i++) {
    sendMidiBoth(type, data1 + chord.intervals[i], data2, channel);
  }
}

void ChordGenEffect::saveOldNotes(
  uint8_t root,
  const Chord_t oldChord,
  const Chord_t newChord
) {
  for (uint8_t i = 0; i < oldChord.count; ++i) {
    int8_t oldInt = oldChord.intervals[i];
    uint8_t oldPC = root + oldInt;

    bool stillPresent = false;
    for (uint8_t j = 0; j < newChord.count; ++j) {
      int8_t newInt = newChord.intervals[j];
      if ((root + newInt) == oldPC) {
        stillPresent = true;
        break;
      }
    }
    if (!stillPresent) {
      oldNotes[i] = oldInt;
      hasOldNotes = true;
    }
  }
}

void ChordGenEffect::removeOldNotes() {
  if (hasOldNotes) {
    for (uint8_t i = 0; i < MAX_CHORD_TONES; i++) {
      hardwareMIDI.sendNoteOff(lastNote + oldNotes[i], lastVelocity,
                               lastChannel);
      usbMIDI.sendNoteOff(lastNote + oldNotes[i], lastVelocity,
                          lastChannel);
    }
    hasOldNotes = false;
  }
}

void ChordGenEffect::handleSwitchEvent(State_t *state, SwEvent_t event) {
  switch (event) {
  case Click:
    saveOldNotes(lastNote, chordBank[chordIdx], CHORD_OFF);
    hasOldNotes = true;

    state->isActive = !state->isActive;
    break;
  default:
    break;
  }
}

ChordGenEffect::ChordGenEffect(uint8_t bankNum) {
  chordIdx = 0;

  switch (bankNum) {
  case 1:
    chordBank = (Chord_t*) CHORD_BANK1;
    break;
  case 2:
    chordBank = (Chord_t*) CHORD_BANK2;
    break;
  case 3:
    chordBank = (Chord_t*) CHORD_BANK3;
    break;
  default: // Shouldn't happen, but default to bank 1 if out of range
    chordBank = (Chord_t*) CHORD_BANK1;
    break;
  }
  
  lastNote = 0;
  lastVelocity = 0;
  lastChannel = 0;
  hasOldNotes = false;
}

void ChordGenEffect::handlePanic() {
  for (uint8_t i = 0; i < 16; i++) { // 16 midi channels total
    sendMidiBoth(midi::MidiType::ControlChange, midi::AllNotesOff, 0, i+1);
  }
}

void ChordGenEffect::handleClock() {
  hardwareMIDI.sendClock();
  usbMIDI.sendClock();
}

void ChordGenEffect::process(State_t *state) {
  if (state->isActive) {
    setLed(0, 255, 0); // Green
  } else {
    setLed(0, 0, 0); // Off
  }

  if (state->rotaryMoved) {
    const Chord_t oldChord = chordBank[chordIdx];
    chordIdx = state->rotaryPos;
    const Chord_t newChord = chordBank[chordIdx];
    saveOldNotes(lastNote, oldChord, newChord);
  }

  handleSwitchEvent(state, state->stompEvent);
  handleSwitchEvent(state, state->extEvent);

  if (usbMIDI.read()) {
    handleMidiMessage(state->isActive, usbMIDI.getType(), usbMIDI.getData1(),
                      usbMIDI.getData2(), usbMIDI.getChannel());
  }

  if (hardwareMIDI.read()) {
    handleMidiMessage(state->isActive, hardwareMIDI.getType(), hardwareMIDI.getData1(),
                      hardwareMIDI.getData2(), hardwareMIDI.getChannel());
  }
}