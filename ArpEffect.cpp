#include "midi_Defs.h"
#include "MIDIUSB.h"
#include "ArpEffect.h"
#include "Utils.h"
#include <EEPROM.h>

/* BEGIN ARPLIST CLASS */
ArpList::ArpList() {
  size = 0;
  noteIdx = 0;
  stepIdx = 0;
  prevNoteIdx = -1;
  prevStepIdx = -1;
  directionFlag = 1;
  playMode = PLAY_AP;
  octaves = 0;

  // Set all steps on as default
  for (int i = 0; i < 16; i++) {
    stepList[i] = EEPROM.read(EEPROM_ARP_BASE + i);
  }
}

void ArpList::add(midi::DataByte note, midi::DataByte velocity,
                   midi::DataByte channel) {
  ArpNote_t n = {};
  n.note = note;
  n.velocity = velocity;
  n.channel = channel;
  n.lastOctave = 0;
  n.currOctave = 0; // Offset for octave since we do note + (currOctave * 12)
  

  // When inserting a new note, default to note order ascending,
  // But if the current play mode is as-played, insert in order of play
  switch (playMode) {
    case PLAY_AP:
      noteList[size] = n; // Insert at the next available space
      break;
    default: 
      uint8_t i;
      for (i = 0; i < size; i++) {
        // If the note is bigger, insert at that spot
        if (noteList[i].note > n.note) {
          // If the current note is within the shifted notes, we need to shift
          // The current indices up as well
          if (noteIdx >= i) {
            noteIdx++;
          } if (prevNoteIdx >= i) {
            prevNoteIdx++;
          }

          for (uint8_t j = size; j > i; j--) { // Shift all notes up 
            noteList[j] = noteList[j-1];
          }
          break;
        }
      }
      noteList[i] = n;
      break;
  }
  size++;
}

void ArpList::del(midi::DataByte note) {
  // Search through list to find note 
  for (uint8_t i = 0; i < size; i++) {
    // If note found, shift all entries down
    if (noteList[i].note == note) {
      // Turn off the note first (handles octaves)
      sendMidiBoth(midi::MidiType::NoteOff, noteList[i].note + (noteList[i].lastOctave * 12), noteList[i].velocity,
                 noteList[i].channel);
      for (uint8_t j = i; j < size; j++) {
        noteList[j] = noteList[j + 1];
      }
      size--;
      if (prevNoteIdx > i) {
        prevNoteIdx--;
      }
      if (noteIdx > i) {
        noteIdx--;
      }

      // Reset to defaults if no notes are being held
      if (size == 0) {
        noteIdx = 0;
        stepIdx = 0;
        prevNoteIdx = -1;
        prevStepIdx = -1;

        // Set the play mode to default for up/down mode
        // NOTE: Just up/down as others don't change direction
        if (playMode == PLAY_UPDN) {
          directionFlag = 1;
        }
      }
      break; // Stop the loop since the note has been found
    }
  }
}

uint8_t ArpList::getSize() { return size; }

uint8_t ArpList::getOctaves() { return octaves; }

int8_t ArpList::getDirFlag() { return directionFlag; }

ArpNote_t *ArpList::getPrevNote() {
  if (size == 0 || prevNoteIdx == -1 || !stepList[prevStepIdx]) {
    return nullptr;
  }

  return &noteList[prevNoteIdx];
}

ArpNote_t *ArpList::getNote() {

  if (size == 0) {
    return nullptr;
  }

  ArpNote_t *note;

  // 1. If current step is enabled, get the current note
  if (stepList[stepIdx]) {
    note = &noteList[noteIdx];
    prevNoteIdx = noteIdx;
    prevStepIdx = stepIdx;
  } else {
    note = nullptr;
  }

  // 2. Choose the next note index based on the current play mode
  // Up and as-played + octave variants
  if (playMode == PLAY_AP || 
      playMode == PLAY_AP_OCT || 
      playMode == PLAY_AP_OCT2 ||
      playMode == PLAY_UP || 
      playMode == PLAY_UP_OCT ||
      playMode == PLAY_UP_OCT2
  ) {
    noteIdx = (noteIdx + directionFlag) % size;
  }

  // Down + octave variants
  else if (playMode == PLAY_DN || playMode == PLAY_DN_OCT || playMode == PLAY_DN_OCT2) {
    if (noteIdx == 0) {
      noteIdx = size - 1;
    }

    // Otherwise, get next note (directionFlag = -1)
    else {
      noteIdx = (noteIdx + directionFlag) % size;
    }
  }

  else if (playMode == PLAY_UPDN /* || playMode == PLAY_UPDN_OCT*/) {
    // Wrap around if we aren't at the first octave
    if (noteIdx == 0) {
      directionFlag = 1;
    } else if (noteIdx+1 >= size) {
      directionFlag = -1;
    }

    // Proceed to the next step with the direction flag
    noteIdx = (noteIdx + directionFlag);
  }
  
  // 3. Increment the step index
  stepIdx = (stepIdx + 1) % 16; // Go to next step
  return note;
}

void ArpList::toggleStep(uint8_t index) { 
  stepList[index] = !stepList[index];
  EEPROM.write(EEPROM_ARP_BASE + index, stepList[index]);
}

bool ArpList::getStep(uint8_t index) { return stepList[index]; }

void ArpList::setPlayMode(ArpPlayMode_t _playMode) {
  switch (_playMode) {
    case PLAY_AP:
      directionFlag = 1;
      octaves = 1;
      break;
    case PLAY_UP:
      directionFlag = 1;
      octaves = 1;
      break;
    case PLAY_DN:
      directionFlag = -1;
      octaves = 1;
      break;
    case PLAY_UPDN:
      directionFlag = 1;
      octaves = 1;
      break;
    case PLAY_AP_OCT:
      directionFlag = 1;
      octaves = 2;
      break;
    case PLAY_UP_OCT:
      directionFlag = 1;
      octaves = 2;
      break;
    case PLAY_DN_OCT:
      directionFlag = -1;
      octaves = 2;
      break;
    case PLAY_AP_OCT2:
      directionFlag = 1;
      octaves = 3;
    case PLAY_UP_OCT2:
      directionFlag = 1;
      octaves = 3;
      break;
    case PLAY_DN_OCT2:
      directionFlag = -1;
      octaves = 3;
      break;
    default:
      break;
  }
  playMode = _playMode;
}

void ArpList::clear() {
  for (uint8_t i = 0; i < size; i++) {
    ArpNote_t n = noteList[noteIdx];
    sendMidiBoth(midi::MidiType::NoteOff, n.note + (n.lastOctave * 12), n.velocity, n.channel);
  }
  size = 0;
  noteIdx = 0;
  stepIdx = 0;
  prevNoteIdx = -1;
  prevStepIdx = -1;
}
/* END ARPLIST CLASS */

ArpEffect::ArpEffect() {
  inProgramMode = false;
  lastClockMs = 0;
  clockCount = 0;
  clocksPerStep = 6; // 6 = 1/16, 12 = 1/4
  extTapIntervalsMs[0] = 500;
  extTapIntervalsMs[1] = 500;
  lastExtTapMs = 0;
  extClockIntervalMs = 125; // 500/4 = 125 
  lastExtClockMs = 0;
  isInitialised = false;
}

void ArpEffect::advanceArpStep() {
  if (isStompActive) {
    // turn off previous note
    ArpNote_t *last = arpList.getPrevNote();
    if (last != nullptr) {
      sendMidiBoth(midi::MidiType::NoteOff, last->note + (last->lastOctave * 12), last->velocity,
                  last->channel);
    }

    // turn on current note
    ArpNote_t *curr = arpList.getNote();
    if (curr != nullptr) {
      sendMidiBoth(midi::MidiType::NoteOn, curr->note + (curr->currOctave * 12), curr->velocity,
                  curr->channel);
    }
    curr->lastOctave = curr->currOctave;

    if (arpList.getOctaves() > 0) {
      curr->currOctave = (curr->currOctave + arpList.getDirFlag()) % arpList.getOctaves();
    }
  }
}

void ArpEffect::handleMidiMessage(
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
  case midi::MidiType::Continue: // Send continue
    hardwareMIDI.sendContinue();
    usbMIDI.sendContinue();
    break;
  case midi::MidiType::Start: // Send start
    hardwareMIDI.sendStart();
    usbMIDI.sendStart();
    break;
  case midi::MidiType::Stop: // Send stop
    hardwareMIDI.sendStop();
    usbMIDI.sendStop();
    break;
  case midi::MidiType::NoteOn: {
    if (isActive) {
      arpList.add(data1, data2, channel); // Add note to list
    } else {
      arpList.add(data1, data2, channel); // Add anyway
      sendMidiBoth(type, data1, data2, channel);
    }
    break;
  }
  case midi::MidiType::NoteOff: {
    if (isActive) {
      arpList.del(data1); // Delete note from list
    } else {
      arpList.del(data1); // Delete anyway
      sendMidiBoth(type, data1, data2, channel);
    }
    break;
  }
  default: // Send any other message normally
    sendMidiBoth(type, data1, data2, channel);
  }
}

void ArpEffect::process(State_t *state) {
  unsigned long now = millis();

  if (!isInitialised) {
    isStompActive = state->isActive;
    arpList.setPlayMode((state->rotaryPos < NUM_PLAYMODE ? (ArpPlayMode_t) state->rotaryPos : PLAY_AP));
    isInitialised = true;
  }

  /* Set the correct LED colour based on states */
  if (state->isActive) {
    if (inProgramMode) {
      if (arpList.getStep(state->rotaryPos)) setLed(0, 0, 255); // Blue
      else setLed(0, 0, 0); // Off
    } else if (turnOffLed) {
      setLed(0, 0, 0); // Off
      turnOffLed = false;
      clockLedOn = false;
    } else if (turnOnLed) {
      setLed(0, 255, 50); // Weird green
      turnOnLed = false;
      clockLedOn = true;
    } else if (!clockLedOn) {
      setLed(0, 255, 0); // Green
    }
  } else {
    setLed(0, 0, 0); // Off
  }

  /* Change play mode if rotary changed and in normal mode */
  if (
    state->rotaryMoved && 
    !inProgramMode && 
    state->rotaryPos < NUM_PLAYMODE
  ) {
    arpList.setPlayMode((ArpPlayMode_t) state->rotaryPos);
  }

  /* Save the tap time for ext footswitch click for internal clock source */
  switch (state->extEvent) {
  case Click: // Shuffle the last tap over, and add the current tap interval
    extTapIntervalsMs[1] = extTapIntervalsMs[0];
    extTapIntervalsMs[0] = now - lastExtTapMs;
    lastExtTapMs = now;
    extClockIntervalMs = ((extTapIntervalsMs[0] + extTapIntervalsMs[1]) / 2)/4;
    break;
  default: 
    break;
  }

  /* Limit footswitch time by setting to now on timeout */
  if (now - lastExtTapMs > EXT_TIMEOUT) {
    lastExtTapMs = now;
  }

  /* Handle clock generation when no midi clock is available */
  unsigned long clockTimeout = now - lastClockMs > CLOCK_TIMEOUT;
  unsigned long extTime = now - lastExtClockMs;
  if ( // Play next step if ext reach and no midi clock
    clockTimeout &&
    (extTime > extClockIntervalMs)
  ) {
    advanceArpStep();
    lastExtClockMs = now;
    turnOnLed = true;
  } else if ( // Turn off clock led if half ext clock time reached
    clockTimeout && // and there's no midi clock
    (extTime > extClockIntervalMs/2) &&
    clockLedOn
  ) {
    turnOffLed = true;
  }

  /* Handle the stomp footswitch state */
  switch (state->stompEvent) {
  case Click:
    if (inProgramMode) {
      arpList.toggleStep(state->rotaryPos);
    } else {
      if (state->isActive) {
        arpList.clear(); // Clear the list and send notes off
      }
      state->isActive = !state->isActive;
      isStompActive = state->isActive;
    }
    break;
  case LongPress:
    if (state->isActive) {
      inProgramMode = !inProgramMode;
    }
    break;
  default:
    break;
  }

  /* Handle incoming midi */
  if (usbMIDI.read()) {
    handleMidiMessage(state->isActive, usbMIDI.getType(), usbMIDI.getData1(),
                      usbMIDI.getData2(), usbMIDI.getChannel());
  }

  if (hardwareMIDI.read()) {
    handleMidiMessage(state->isActive, hardwareMIDI.getType(), hardwareMIDI.getData1(),
                      hardwareMIDI.getData2(), hardwareMIDI.getChannel());
  }
}

void ArpEffect::handlePanic() {
  arpList.clear();
  for (uint8_t i = 0; i < 16; i++) { // 16 midi channels total
    sendMidiBoth(midi::MidiType::ControlChange, midi::AllNotesOff, 0, i+1);
  }
}

void ArpEffect::handleClock() {
  hardwareMIDI.sendClock();
  usbMIDI.sendClock();

  clockCount++;
  if (clockCount >= clocksPerStep) {
    clockCount = 0;
    advanceArpStep(); // queue next note
    if (!inProgramMode) {
      turnOnLed = true;
    }
  } else if (clockCount == clocksPerStep/2) {
    turnOffLed = true;
  }
  lastClockMs = millis();
}

/* BEGIN ARP EFFECT CLASS */


