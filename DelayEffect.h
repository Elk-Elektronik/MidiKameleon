#ifndef DELAY_H
#define DELAY_H

#include "Globals.h"
#include "BaseEffect.h"

#define MAX_DELAY_NOTES 30
#define CLOCK_TIMEOUT 1000
#define EXT_TIMEOUT 5000

typedef struct {
  bool isActive;            // If the note still has some velocity, it's active
  bool isOn;                // Note is currently playing
  uint8_t note;             // MIDI note number
  uint8_t initVelocity;     // MIDI note velocity when recorded
  uint8_t velocity;         // Current MIDI note velocity (decays per repeat)
  uint8_t channel;          // MIDI channel note was played on
  unsigned long lastPlayMs; // When the note was LAST turned on
  unsigned long noteOnMs;   // When the note was INITIALLY turned on
  unsigned long noteOffIntervalMs; // The interval between the initial note
                                   // record, and when it was released
} DelayNote_t;

class DelayEffect : public BaseEffect {
private:
  volatile unsigned long delayTimeMs; // How long each delay is

  /* Clock Input */
  volatile unsigned long lastClockMs; // The last clock pulse time
  volatile unsigned long clockIntervalMs; // The interval between clock pulses

  /* External footswitch tap intervals */
  unsigned long lastExtTapMs; // Use this to calculate the intervals above
  unsigned long extTapIntervalsMs[2]; // the last two (2) recorded ext footswitch tap intervals

  uint8_t delayDivision; // How much to divide the delayTime by (1-16)

  volatile uint16_t bpm; // The current bpm
  volatile bool clockRecieved;

  DelayNote_t delayNotes[MAX_DELAY_NOTES]; // the last 30 notes stored for delay
  uint8_t delayNotesIdx; // The current index of the next free slot
  
  uint8_t numRepeats; // The number of repeats for the delay (1-16)
  bool inDivisionMode; // If the pedal is in division mode

  int8_t findDelayNote(uint8_t note, uint8_t channel);
  void resetDelayNote(uint8_t idx, uint8_t velocity, unsigned long now);
  void addDelayNote(uint8_t note, uint8_t velocity, uint8_t channel,
                    unsigned long now);
  void decayVelocity(DelayNote_t &note);
  void handleMidiMessage(bool isActive, midi::MidiType type, midi::DataByte data1,
                          midi::DataByte data2, midi::Channel channel);
  void checkExtTapTimeout();
public:
  DelayEffect();
  void process(State_t *state) override;
  void handleClock() override;
};

#endif // DELAY_H