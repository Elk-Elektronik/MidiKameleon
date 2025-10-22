#include "Arduino.h"
#include "Utils.h"
#include "DelayEffect.h"

DelayEffect::DelayEffect() {
  delayTimeMs = 0;
  lastClockMs = 0;
  clockIntervalMs = 0;
  bpm = 0;
  delayNotesIdx = 0;
  delayDivision = 1;
  inDivisionMode = false;
  extTapIntervalsMs[0] = 500;
  extTapIntervalsMs[1] = 500;
  delayLedOn = false;
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
}

void DelayEffect::addDelayNote(uint8_t note, uint8_t velocity, uint8_t channel,
                                unsigned long now) {
  DelayNote_t dn = {};
  dn.note = note;
  dn.velocity = velocity;
  dn.initVelocity = velocity;
  dn.channel = channel;
  dn.lastPlayMs = now;
  dn.noteOnMs = now;
  dn.isActive = true;
  dn.noteOffIntervalMs = 0;
  delayNotes[delayNotesIdx] = dn;
  delayNotesIdx = (delayNotesIdx + 1) % MAX_DELAY_NOTES;
}

void DelayEffect::decayVelocity(DelayNote_t &note) {
  uint8_t step = note.initVelocity / numRepeats;

  // Only reduce velocity if note has been released
  if (note.noteOffIntervalMs != 0) {
    note.velocity = max(note.velocity - step, 0);
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
    if (isActive) {
      unsigned long now = millis();
      bool isOn = false;
      int8_t idx = findDelayNote(data1, channel);

      // Reset the note (if found) or add it
      if (idx != -1) {
        resetDelayNote(idx, data2, now);
        isOn = delayNotes[idx].isOn;
      } else {
        addDelayNote(data1, data2, channel, now);
      }

      // Turn off note if its currently on
      if (isOn) {
        sendMidiBoth(midi::MidiType::NoteOff, data1, data2, channel);
      }
    }
    sendMidiBoth(type, data1, data2, channel);
    break;
  }
  case midi::MidiType::NoteOff: {
    bool isOn = false;
    if (isActive) {
      
      // Check if the note is present
      int8_t idx = findDelayNote(data1, channel);

      // if present and active, record when it was turned off (for delay length)
      if (idx != -1 && delayNotes[idx].isActive &&
          delayNotes[idx].noteOffIntervalMs == 0
      ) {
        delayNotes[idx].noteOffIntervalMs = millis() - delayNotes[idx].noteOnMs;
        isOn = delayNotes[idx].isOn;
      }
    
    // Otherwise if the note isn't an active delay note, or the pedal isn't active,
    // Just pass through the message
    } if (!isOn) {
      sendMidiBoth(type, data1, data2, channel);
    }
    break;
  }
  default: // Send any other message normally
    sendMidiBoth(type, data1, data2, channel);
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

  if (state->isActive) {
    if (inDivisionMode) {
      setLed(255, 100, 100);
    } else if (delayLedOn && (now - lastLedOnTime) > (delayTimeMs/delayDivision)/2) {
      setLed(0, 0, 0);
      delayLedOn = false;
    } else if (!delayLedOn && (now - lastLedOnTime) > (delayTimeMs/delayDivision)) {
      setLed(0, 255, 50); // Weird green
      delayLedOn = true;
      lastLedOnTime = now;
    } else if (!delayLedOn) {
      setLed(0, 255, 0);
    }
  } else {
    setLed(0, 0, 0);
  }

  if (state->rotaryMoved) {
    if (inDivisionMode) {
      delayDivision = state->rotaryPos+1; // Add 1 so we don't get divide by 0 error
    } else {
      numRepeats = state->rotaryPos;
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

    if (dn.isActive) {
      
      // Note should be turned off
      if (dn.isOn && dn.noteOffIntervalMs > 0 && 
          (now - dn.lastPlayMs) > dn.noteOffIntervalMs
      ) {
        sendMidiBoth(midi::MidiType::NoteOff, dn.note, dn.velocity, dn.channel);
        delayNotes[i].isOn = false; // Modify the original delay note
      }

      // Next delay should be handled
      if ((now - dn.lastPlayMs) > delayTimeMs/delayDivision) {

        // Note should delay again with less velocity
        if (dn.velocity > 0) {

          // Send note off (if needed) before sending note on again 
          if (dn.isOn) {
            sendMidiBoth(midi::MidiType::NoteOff, dn.note, dn.velocity, dn.channel);
          }
          sendMidiBoth(midi::MidiType::NoteOn, dn.note, dn.velocity, dn.channel);

          // Decay the velocity
          decayVelocity(delayNotes[i]);

          // Set note's state variables
          delayNotes[i].lastPlayMs = now;
          delayNotes[i].isOn = true;

        // Note should be turned off since velocity is < 0
        } else {
          delayNotes[i].isActive = false;
          if (dn.isOn) {
            sendMidiBoth(midi::MidiType::NoteOff, dn.note, dn.velocity, dn.channel);
          }
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

void DelayEffect::handleClock() {
  hardwareMIDI.sendClock();
  usbMIDI.sendClock();

  unsigned long now = millis();
  if (lastClockMs != 0) {
    clockIntervalMs = now - lastClockMs; // ms per clock
    bpm = 60000 / (clockIntervalMs * MIDI_CLOCKS_PER_QUARTER);
    delayTimeMs = 60000 / bpm;
  }
  lastClockMs = now;
}