#include "midi_Defs.h"
#include "Arduino.h"
#include "EEPROM.h"
#include "Utils.h"
#include "MidiMuteEffect.h"

// Constructor
ChannelMute::ChannelMute(uint8_t midiChannel, uint8_t eepromAddress) {
  channel = midiChannel;
  address = eepromAddress;
  stopSent = 0;
}

// Getters
uint8_t ChannelMute::getChannel() { return channel; }
uint8_t ChannelMute::getAddress() { return address; }
bool ChannelMute::getIsMuted() { return isMuted; }
uint8_t ChannelMute::getMidiType() { return midiType; }
uint8_t ChannelMute::getMidiData1() { return midiData1; }
uint8_t ChannelMute::getMidiData2() { return midiData2; }
uint8_t ChannelMute::getMidiChannel() { return midiChannel; }
bool ChannelMute::getStopSent() { return stopSent; }

// Setters
void ChannelMute::setIsMuted(bool state) { isMuted = state; }
void ChannelMute::setStopSent(bool state) { stopSent = state; }

bool ChannelMute::toggleIsMuted() {
  isMuted = !isMuted;
  return isMuted;
}

void ChannelMute::storeMidi(uint8_t type, uint8_t data1, uint8_t data2,
                            uint8_t channel) {
  midiType = type;
  midiData1 = data1;
  midiData2 = data2;
  midiChannel = channel;
}
/* END CHANNEL MUTE CLASS */

MidiMuteEffect::MidiMuteEffect() {
  ledOn = false;
  ledOnStartMs = 0;

  // Populate the channelMutes array and get 
  // the last mute state for each channelMute from EEPROM
  for (uint8_t i = 0; i < MIDI_NUM_CHANNELS; i++) {
    channelMutes[i] = ChannelMute(i+1, EEPROM_MUTE_BASE + i);
    channelMutes[i].setIsMuted(EEPROM.read(EEPROM_MUTE_BASE + i));
  }
}

void MidiMuteEffect::sendAllNotesOff(ChannelMute &channel) {
  if (!channel.getStopSent()) {
    // Send all notes off
    sendMidiBoth(midi::MidiType::ControlChange, midi::AllNotesOff, 0,
                 channel.getChannel());

    // Handles hardware that does not have all notes off CC by sending note off
    // for the last recorded note
    sendMidiBoth(midi::MidiType::NoteOff, channel.getMidiData1(),
                 channel.getMidiData2(), channel.getChannel());

    // Change channel state so we know a note off has been sent
    channel.setStopSent(true);
  }
}

void MidiMuteEffect::handleMidiMessage(
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
  default: 
    // If pedal isn't active, or the channel isn't muted, send the midi
    if (!isActive || !channelMutes[channel - 1].getIsMuted()) {
      // Pass through midi data
      sendMidiBoth(type, data1, data2, channel);

      // Store MIDI message in channel mute
      channelMutes[channel - 1].storeMidi(type, data1, data2, channel);
      channelMutes[channel - 1].setStopSent(false);
    }
    break;
  }
}

void MidiMuteEffect::handleSwitchEvent(State_t *state, SwEvent_t event) {
  switch (event) {
  case Click:
    state->isActive = !state->isActive;
    if (state->isActive) { // Send all notes off if muted
      for (uint8_t i = 0; i < MIDI_NUM_CHANNELS; i++) {
        if (channelMutes[i].getIsMuted()) {
          sendAllNotesOff(channelMutes[i]);
        }
      }
    }
    break;
  case LongPress: {
    ChannelMute &chan = channelMutes[state->rotaryPos];
    bool isMuted = chan.toggleIsMuted();

    if (isMuted) {
      setLed(0, 0, 255); // Blue
    } else {
      setLed(0, 255, 0); // Green
    }
    ledOn = true;
    ledOnStartMs = millis();

    // Save new mute state
    EEPROM.write(chan.getAddress(), chan.getIsMuted());
    break;
  } default:
    break;
  }
}

void MidiMuteEffect::handlePanic() {
  for (uint8_t i = 0; i < 16; i++) { // 16 midi channels total
    sendMidiBoth(midi::MidiType::ControlChange, midi::AllNotesOff, 0, i+1);
  }
}

void MidiMuteEffect::handleClock() {
  hardwareMIDI.sendClock();
  usbMIDI.sendClock();
}

void MidiMuteEffect::process(State_t *state) {
  if (state->isActive && !ledOn) {
    setLed(255, 0, 0); // Red  
  } else if (!state->isActive && !ledOn) {
    setLed(0, 0, 0); // Off
  }

  // If the rotary has changed position, update the position and show
  // the user the current position's channel mute status
  if (state->rotaryMoved) {
    if (channelMutes[state->rotaryPos].getIsMuted()) {
      setLed(0, 0, 255); // Blue
    } else {
      setLed(0, 255, 0); // Green
    }
    ledOn = true;
    ledOnStartMs = millis();
  }

  // We pass the state object so we can modify the isActive state
  handleSwitchEvent(state, state->stompEvent);
  handleSwitchEvent(state, state->extEvent);

  if (ledOn && (millis() - ledOnStartMs > LED_TIME_MS)) {
    setLed(0, 0, 0);
    ledOn = false;
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