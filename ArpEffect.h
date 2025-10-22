#ifndef ARP_H
#define ARP_H

#include "Globals.h"
#include "BaseEffect.h"
#include "Switches.h"

typedef enum { ARPMODE_DEFAULT, ARPMODE_PROGRAM, NUM_ARPMODE } ArpMode_t; // The current mode the arp effect is in

typedef enum { PLAY_AP, PLAY_UP, PLAY_DN, PLAY_UPDN, PLAY_AP_OCT, PLAY_UP_OCT, PLAY_DN_OCT, PLAY_UPDN_OCT, NUM_PLAYMODE } ArpPlayMode_t; // The current play mode

#define NOTE_BUFFER_SIZE 20

typedef struct {
  uint8_t note;
  uint8_t velocity;
  uint8_t channel;
  uint8_t octaves;
  uint8_t lastOctave;
  uint8_t currOctave;
} ArpNote_t;

/* BEGIN ARPLIST CLASS */
class ArpList {
private:
  ArpNote_t noteList[NOTE_BUFFER_SIZE] = {}; // The list of notes being held down
  bool stepList[16];  // The 16 steps available for muting
  uint8_t size;    // The size of the noteList
  //uint8_t octaves; // The number of octaves to span

  uint8_t noteIdx; // The index of the next note to play
  uint8_t stepIdx; // The index of the next step
  int8_t prevNoteIdx; // The index of the previous note played
  int8_t prevStepIdx; // The index of the previous step

  int8_t directionFlag; // The current arp direction. up = 1, down = -1.

  ArpPlayMode_t playMode; // The current play mode for arpeggiator

public:
  ArpList();
  void add(midi::DataByte note, midi::DataByte velocity,
            midi::DataByte channel); // Add a note to the list depending on the arpFormat
  void del(midi::DataByte note);     // Delete a specific note from the list

  uint8_t getSize(); // Get list size
  ArpNote_t *getNote(); // Retrieve the note to play
  ArpNote_t *getPrevNote(); // Retrieve the previous note played

  void toggleStep(uint8_t index);
  bool getStep(uint8_t index);
  void setPlayMode(ArpPlayMode_t mode);
};
/* END ARPLIST CLASS */

class ArpEffect : public BaseEffect {
private:
  ArpList arpList; // The list of arpeggiator notes
  bool inProgramMode; // Is the effect in step program mode?

  /* Clock */
  volatile unsigned long lastClockMs; // The last clock pulse time
  volatile uint8_t clockCount; // The current clock step we're on
  uint8_t clocksPerStep; // How many clock steps we need before we trigger an arp play

  /* External footswitch tempo input */
  unsigned long extTapIntervalsMs[2]; // the last two (2) recorded ext footswitch tap intervals
  unsigned long lastExtTapMs; // Use this to calculate the intervals above
  unsigned long extClockIntervalMs; // Use this when no external clock is available
  unsigned long lastExtClockMs; // Use to calculate when to trigger next step on ext clock

  /* Clock LED */
  volatile bool turnOnLed; // Do we need to turn on the led?
  volatile bool turnOffLed; // Do we need to turn off the led? 
  bool clockLedOn; // 
  unsigned long lastLedOnTime;

  void advanceArpStep();
  void handleMidiMessage(bool isActive, midi::MidiType type, midi::DataByte data1,
                          midi::DataByte data2, midi::Channel channel);

public:
  ArpEffect();
  void process(State_t *state) override;
  void handleClock() override;
};

#endif // ARP_H