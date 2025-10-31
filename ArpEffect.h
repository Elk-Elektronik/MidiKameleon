#ifndef ARP_H
#define ARP_H

#include "Globals.h"
#include "BaseEffect.h"
#include "Switches.h"

typedef enum { ARPMODE_DEFAULT, ARPMODE_PROGRAM, NUM_ARPMODE } ArpMode_t; // The current mode the arp effect is in

typedef enum { AP, UP, DOWN, UPDOWN, RAND } ArpDirection_t; // The direction for the ArpPlayMode_t struct

typedef struct {
  uint8_t octaveSpan;
  bool chordMode;
  ArpDirection_t direction;
} ArpPlayMode_t;

#define NUM_PLAYMODE 16
const ArpPlayMode_t playModes[NUM_PLAYMODE] = {
  // 1 oct
  {1, false, AP},
  {1, false, UP},
  {1, false, DOWN},
  {1, false, UPDOWN},

  // 2 oct
  {2, false, AP},
  {2, false, UP},
  {2, false, DOWN},

  // 3 oct
  {3, false, AP},
  {3, false, UP},
  {3, false, DOWN},

  // Chord modes
  {1, true, AP},
  {2, true, AP},
  {3, true, AP},

  // Random modes
  {1, false, RAND},
  {2, false, RAND},
  {3, false, RAND},
};

#define NOTE_BUFFER_SIZE 20

typedef struct {
  uint8_t note;
  uint8_t velocity;
  uint8_t channel;
  uint8_t octaves;
  int8_t lastOctave;
  int8_t currOctave;
} ArpNote_t;

/* BEGIN ARPLIST CLASS */
class ArpList {
private:
  ArpNote_t noteList[NOTE_BUFFER_SIZE] = {}; // The list of notes being held down
  bool stepList[16];  // The 16 steps available for muting
  uint8_t size;    // The size of the noteList

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
  uint8_t getOctaves(); // Get the number of octaves to span
  int8_t getDirFlag(); // Get the current direction flag
  ArpNote_t *getNote(); // Retrieve the note to play
  ArpNote_t *getPrevNote(); // Retrieve the previous note played

  void toggleStep(uint8_t index);
  bool isChordMode();
  void incrementStep();
  bool getStep(uint8_t index);
  ArpNote_t *getNoteAt(uint8_t pos); // Get a note at a specific position, or if no note, return null
  void setPlayMode(uint8_t pos);
  void clear(); // Clear the list and send note off for all the remaining notes
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

  /* State copies */
  bool isStompActive; // Use this as a 'global' reference to isActive for the advanceArpStep function

  bool isInitialised;
 
  void advanceArpStep();
  void handleMidiMessage(bool isActive, midi::MidiType type, midi::DataByte data1,
                          midi::DataByte data2, midi::Channel channel);
  void checkOctaveBounds(ArpNote_t *ap);

public:
  ArpEffect();
  void process(State_t *state) override;
  void handlePanic() override;
  void handleClock() override;
};

#endif // ARP_H