#ifndef CHORDGEN_H
#define CHORDGEN_H

#include "BaseEffect.h"

#define MAX_CHORD_TONES 5
#define NUM_CHORDS 16

typedef struct {
  uint8_t count;                     // Number of intervals
  int8_t intervals[MAX_CHORD_TONES]; // Semitone offsets from root
} Chord_t;

// Used to compare with for turning off old notes when stomp disabled
const Chord_t CHORD_OFF = {
  5,
  {0, 0, 0, 0, 0}
};

/* CHORD BANKS */
const Chord_t CHORD_BANK1[NUM_CHORDS] = {
    // Q1
    {3, {0, 4, 7}},        // Major triad
    {4, {0, 4, 7, 14}},    // Add9
    {4, {0, 4, 7, 9}},     // 6
    {5, {0, 4, 7, 9, 14}}, // 6/9

    // Q2
    {4, {0, 4, 7, 11}}, // Maj7
    {4, {0, 4, 8, 11}}, // Maj7#5 (AugMaj7)
    {4, {0, 4, 7, 10}}, // Dom7
    {4, {0, 4, 8, 10}}, // 7#5 (Aug7)

    // Q3
    {3, {0, 3, 7}},     // Minor triad
    {4, {0, 3, 7, 10}}, // m7
    {4, {0, 3, 7, 11}}, // mMaj7
    {4, {0, 3, 6, 10}}, // m7b5 (half-diminished)

    // Q4
    {4, {0, 3, 6, 9}}, // Dim7
    {3, {0, 3, 6}},    // Diminished triad
    {3, {0, 5, 7}},    // Sus4
    {3, {0, 2, 7}},    // Sus2
};

const Chord_t CHORD_BANK2[NUM_CHORDS] = {
    // Q1
    {3, {0, 4, 7}}, // Major triad
    {3, {0, 3, 7}}, // Minor triad
    {3, {0, 3, 6}}, // Diminished triad
    {3, {0, 4, 8}}, // Augmented triad

    // Q2
    {4, {0, 4, 7, 11}}, // Maj7
    {4, {0, 3, 7, 10}}, // Min7
    {4, {0, 4, 7, 10}}, // Dom7
    {4, {0, 3, 6, 10}}, // m7b5 (half-dim)

    // Q3
    {4, {0, 3, 6, 9}}, // Dim7
    {3, {0, 2, 7}},    // Sus2
    {3, {0, 5, 7}},    // Sus4
    {4, {0, 2, 4, 7}}, // Add9

    // Q4
    {5, {0, 4, 7, 11, 14}}, // Maj9
    {5, {0, 4, 7, 10, 14}}, // Dom9
    {4, {0, 4, 8, 10}},     // Dom7#5
    {4, {0, 3, 7, 11}},     // Min(maj7)
};

const Chord_t CHORD_BANK3[NUM_CHORDS] = {
    // Imaj7 (Cmaj7)
    {4, {0, 4, 7, 11}},
    {4, {-8, -5, -1, 0}},
    {4, {-5, -1, 0, 4}},
    {4, {-1, 0, 4, 7}},

    // ii7 (Dmin7)
    {4, {0, 3, 7, 10}},
    {4, {-9, -5, -2, 0}},
    {4, {-5, -2, 0, 3}},
    {4, {-2, 0, 3, 7}},

    // V7 (G7)
    {4, {0, 4, 7, 10}},
    {4, {-8, -5, -2, 0}},
    {4, {-5, -2, 0, 4}},
    {4, {-2, 0, 4, 7}},

    // vi9 (Amin9)
    {5, {0, 3, 7, 10, 14}},
    {5, {-10, -9, -5, -2, 0}},
    {5, {-9, -5, -2, 0, 2}},
    {5, {-5, -2, 0, 2, 7}},
};

class ChordGenEffect: public BaseEffect {
private:
  uint8_t chordIdx; // The current chord index
  Chord_t *chordBank; // The current chord bank being used

  uint8_t lastNote; // The last note played
  uint8_t lastVelocity; // The velocity the last note played with
  uint8_t lastChannel; // The channel the last note played on

  bool hasOldNotes; // Does the effect have old notes?
  int8_t oldNotes[MAX_CHORD_TONES] = {0};

  void handleMidiMessage(bool isActive, midi::MidiType type, midi::DataByte data1,
                          midi::DataByte data2, midi::Channel channel);

  void sendChord(midi::MidiType type, midi::DataByte data1, midi::DataByte data2,
                  midi::Channel channel);

  void saveOldNotes(uint8_t root, const Chord_t oldChord,
                      const Chord_t newChord);

  void removeOldNotes();
  void handleSwitchEvent(State_t *state, SwEvent_t event);

public:
  ChordGenEffect(uint8_t bankNum);
  void process(State_t *state) override;
  void handleClock() override;
};

#endif // CHORDGEN_H