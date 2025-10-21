#ifndef UTILS_H
#define UTILS_H

#include "Globals.h"
#include <stdint.h>

void setLed(uint8_t r, uint8_t g, uint8_t b);

void sendMidiBoth(midi::MidiType type, uint8_t note, uint8_t velocity,
                  uint8_t channel);

void sendMidiClock();

void sendMidiStart();

void sendMidiStop();

void sendMidiContinue();

#endif // UTILS_H