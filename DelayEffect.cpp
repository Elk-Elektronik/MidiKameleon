#include "Arduino.h"
#include "Utils.h"
#include "DelayEffect.h"

DelayEffect::DelayEffect() {
  delayTimeMs = 0;
  lastClockMs = 0;
  clockIntervalMs = 0;
  delayNotesIdx = 0;
  delayDivision = 1;
  inDivisionMode = false;
  extTapIntervalsMs[0] = 500;
  extTapIntervalsMs[1] = 500;
  delayLedOn = false;
  clockFlag = false;
}

int8_t DelayEffect::findDelayNote(uint8_t note, uint8_t channel) {
  for (int i = 0; i < MAX_DELAY_NOTES; i++) {
    if (delayNotes[i].isActive && delayNotes[i].note == note &&
        delayNotes[i].channel == channel) {
      return i; // found
    }
  }
  return -1; // not found
}

void DelayEffect::resetDelayNote(uint8_t idx, uint8_t velocity, unsigned long now) {
  delayNotes[idx].velocity = velocity;
  delayNotes[idx].initVelocity = velocity;
  delayNotes[idx].lastPlayMs = now;
  delayNotes[idx].noteOffIntervalMs = 0;
  delayNotes[idx].noteOnMs = now;
  delayNotes[idx].repeatsLeft = numRepeats;
}

  void DelayEffect::addDelayNote(uint8_t note, uint8_t velocity, uint8_t channel,
                                  unsigned long now) {
    DelayNote_t dn = {};
    dn.isActive = true;
    dn.isOn = true;

    dn.note = note;
    dn.initVelocity = velocity;
    dn.velocity = velocity;
    dn.channel = channel;
    dn.lastPlayMs = now;

    dn.noteOnMs = now;
    dn.noteOffIntervalMs = 0;
    dn.repeatsLeft = numRepeats;

    delayNotes[delayNotesIdx] = dn;
    delayNotesIdx = (delayNotesIdx + 1) % MAX_DELAY_NOTES;
  }

void DelayEffect::decayVelocity(DelayNote_t &note) {
  // Only reduce velocity if note has been released
  if (note.noteOffIntervalMs != 0) {
    uint8_t step = (note.velocity + note.repeatsLeft - 1) / note.repeatsLeft;
    note.velocity = (note.velocity > step) ? (note.velocity - step) : 0;

    if (note.repeatsLeft != 0) {
      note.repeatsLeft--;
    }
  }
}

void DelayEffect::handleMidiMessage(
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
    sendMidiContinue();
    break;
  case midi::MidiType::Start: // Send start
    sendMidiStart();
    break;
  case midi::MidiType::Stop: // Send stop
    sendMidiStop();
    break;
  case midi::MidiType::NoteOn: {
    int8_t idx = findDelayNote(data1, channel);

    // Pedal is active
    if (isActive) {
      unsigned long now = millis();

      // Reset the note (if found) or add it
      if (idx != -1) {
        resetDelayNote(idx, data2, now);

        // Turn off if already on
        if (delayNotes[idx].isOn) {
          sendMidiBoth(midi::MidiType::NoteOff, data1, data2, channel);
        }
      } else {
        addDelayNote(data1, data2, channel, now);
      }

    }
    delayNotes[idx].isOn = true;
    sendMidiBoth(type, data1, data2, channel);
    break;
  }
  case midi::MidiType::NoteOff: {
    int8_t idx = findDelayNote(data1, channel);

    // Pedal active, note found, and note is active
    if (isActive) {
      if (idx != -1 && delayNotes[idx].isActive) {
        delayNotes[idx].noteOffIntervalMs = millis() - delayNotes[idx].noteOnMs;
      }

      if (!delayNotes[idx].isOn) {
        delayNotes[idx].isOn = false;
        sendMidiBoth(type, data1, data2, channel);
      }
    } else {
      sendMidiBoth(type, data1, data2, channel);
    }
    break;
  }
  default: // Send any other message normally
    sendMidiBoth(type, data1, data2, channel);
    break;
  }
}

void DelayEffect::process(State_t *state) {
  unsigned long now = millis();

  // Initialise the numRepeats on the first call
  static bool initialised = false;
  if (!initialised) {
    numRepeats = state->rotaryPos;
    initialised = true;
  }

  if (clockFlag) {
    hardwareMIDI.sendClock();
    usbMIDI.sendClock();

    unsigned long now = millis();
    clockIntervalMs = now - lastClockMs;
    delayTimeMs = clockIntervalMs * MIDI_CLOCKS_PER_QUARTER;
    lastClockMs = now;
    clockFlag = false;
  }

  if (state->isActive) {
    if (inDivisionMode) {
      setLed(127, 127, 127); // Light blue
    } else if (delayLedOn && (now - lastLedOnTime) > (delayTimeMs/(float)delayDivision)/2) {
      setLed(0, 0, 0);
      delayLedOn = false;
    } else if (!delayLedOn && (now - lastLedOnTime) > (delayTimeMs/(float)delayDivision)) {
      setLed(127, 127, 0); // Yellow
      delayLedOn = true;
      lastLedOnTime = now;
    } else if (!delayLedOn) {
      setLed(0, 0, 255); // Blue
    }
  } else {
    setLed(0, 0, 0);
  }

  if (state->rotaryMoved) {
    if (inDivisionMode) {
      delayDivision = state->rotaryPos+1; // Add 1 so we don't get divide by 0 error
    } else {
      numRepeats = state->rotaryPos+1;
    }
  }

  // The ext footswitch hasn't been pressed recently, so use the current time as the last ext tap
  if (now - lastExtTapMs > EXT_TIMEOUT) {
    lastExtTapMs = now;
  }

  // Clock hasn't been recieved recently, so use the ext footswitch as clock source
  if (now - lastClockMs > CLOCK_TIMEOUT) {
    delayTimeMs = (extTapIntervalsMs[0] + extTapIntervalsMs[1]) / 2; // Average out taps 
  }

  switch (state->extEvent) {
  case Click: // Shuffle the last tap over, and add the current tap interval
    extTapIntervalsMs[1] = extTapIntervalsMs[0];
    extTapIntervalsMs[0] = now - lastExtTapMs;
    lastExtTapMs = now;
    break;
  default: 
    break;
  }

  switch (state->stompEvent) {
  case Click:
    if (inDivisionMode) {
      inDivisionMode = false;
    } else {
      state->isActive = !state->isActive;
    }
    break;
  case LongPress:
    if (state->isActive) {
      inDivisionMode = !inDivisionMode;
    }
    break;
  default:
    break;
  }

  for (uint8_t i = 0; i < MAX_DELAY_NOTES; i++) {
    DelayNote_t dn = delayNotes[i];

    if (delayNotes[i].isActive) {
      
      // Note should be turned off
      if (
        delayNotes[i].isOn && 
        delayNotes[i].noteOffIntervalMs > 0 && 
        (now - delayNotes[i].lastPlayMs) > delayNotes[i].noteOffIntervalMs
      ) {
        delayNotes[i].isOn = false; // Modify the original delay note
        sendMidiBoth(midi::MidiType::NoteOff, delayNotes[i].note, 
                      delayNotes[i].velocity, delayNotes[i].channel);
      }

      // Next delay should be handled
      if ((now - delayNotes[i].lastPlayMs) > delayTimeMs/(float)delayDivision) {

        // Note should delay again with less velocity
        if (delayNotes[i].velocity > 0) {

          // Send note off (if needed) before sending note on again 
          if (delayNotes[i].isOn) {
            sendMidiBoth(midi::MidiType::NoteOff, delayNotes[i].note, 
                          delayNotes[i].velocity, delayNotes[i].channel);
          }
          delayNotes[i].isOn = true;
          sendMidiBoth(midi::MidiType::NoteOn, delayNotes[i].note, 
                        delayNotes[i].velocity, delayNotes[i].channel);

          // Decay the velocity
          decayVelocity(delayNotes[i]);

          // Set note's state variables
          delayNotes[i].lastPlayMs = now;

        // Note should be turned off since velocity is < 0
        } else {
          delayNotes[i].isActive = false;
          delayNotes[i].isOn = false;
          sendMidiBoth(midi::MidiType::NoteOff, delayNotes[i].note, 
                        delayNotes[i].velocity, delayNotes[i].channel);
        }
      }
    }
  }

  if (usbMIDI.read()) {
    handleMidiMessage(state->isActive, usbMIDI.getType(), usbMIDI.getData1(),
                      usbMIDI.getData2(), usbMIDI.getChannel());
  }

  if (hardwareMIDI.read()) {
    handleMidiMessage(state->isActive, hardwareMIDI.getType(), hardwareMIDI.getData1(),
                      hardwareMIDI.getData2(), hardwareMIDI.getChannel());
  }
}

void DelayEffect::handlePanic() {
  for (uint8_t i = 0; i < delayNotesIdx; i++) {
    delayNotes[i].isActive = false;
    delayNotes[i].isOn = false;
  }
  for (uint8_t i = 0; i < 16; i++) { // 16 midi channels total
    sendMidiBoth(midi::MidiType::ControlChange, midi::AllNotesOff, 0, i+1);
    
  }
}

void DelayEffect::handleClock() {
  clockFlag = true;
}