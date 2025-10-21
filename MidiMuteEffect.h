#ifndef MIDIMUTE_H
#define MIDIMUTE_H

#include "Globals.h"
#include "Switches.h"
#include "BaseEffect.h"
#include <stdint.h>

/* MIDI CC NUM FOR ALL NOTES OFF */
#define ALL_NOTES_OFF_CC 0x7B

enum MIDI_Channel {
  MIDI_CHANNEL_1,
  MIDI_CHANNEL_2,
  MIDI_CHANNEL_3,
  MIDI_CHANNEL_4,
  MIDI_CHANNEL_5,
  MIDI_CHANNEL_6,
  MIDI_CHANNEL_7,
  MIDI_CHANNEL_8,
  MIDI_CHANNEL_9,
  MIDI_CHANNEL_10,
  MIDI_CHANNEL_11,
  MIDI_CHANNEL_12,
  MIDI_CHANNEL_13,
  MIDI_CHANNEL_14,
  MIDI_CHANNEL_15,
  MIDI_CHANNEL_16,
  MIDI_NUM_CHANNELS
};

/* CHANNEL MUTE CLASS */
class ChannelMute {
private:
  uint8_t channel;
  uint8_t address; // EEPROM save address
  bool isMuted; // Mute state
  uint8_t midiType;
  uint8_t midiData1;
  uint8_t midiData2;
  uint8_t midiChannel; // Midi channel
  bool stopSent;

public:
  // Default constructor for usage with arrays. 
  // NOTE: These defaults gets overwritten later in the MidiMuteEffect constructor
  ChannelMute() : channel(0), address(0), isMuted(false) {}

  ChannelMute(uint8_t midiChannel, uint8_t eepromAddress);
  uint8_t getChannel();
  uint8_t getAddress();
  bool getIsMuted();
  uint8_t getMidiType();
  uint8_t getMidiData1();
  uint8_t getMidiData2();
  uint8_t getMidiChannel();
  bool getStopSent();

  void setIsMuted(bool state);
  bool toggleIsMuted();
  void setStopSent(bool state);
  void storeMidi(uint8_t type, uint8_t data1, uint8_t data2, uint8_t channel);
};

class MidiMuteEffect : public BaseEffect {
private:
  ChannelMute channelMutes[MIDI_NUM_CHANNELS];
  bool ledOn;
  unsigned long ledOnStartMs;
  
  void sendAllNotesOff(ChannelMute &channel);
  void handleMidiMessage(bool isActive, midi::MidiType type, midi::DataByte data1,
                           midi::DataByte data2, midi::Channel channel);
  void handleSwitchEvent(State_t *state, SwEvent_t event);
public:
  MidiMuteEffect();
  void process(State_t *state) override;
  void handleClock() override;
};

#endif // MIDIMUTE_H
