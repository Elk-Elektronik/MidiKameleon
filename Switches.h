#ifndef SWITCHES_H
#define SWITCHES_H

#include "Globals.h"

class Switch {
private:
  uint8_t pin;
  uint8_t mode;
  bool value;

public:
  Switch(uint8_t _pin, uint8_t _mode);
  void setup();
  void refresh();
  bool getValue();
};

class EventSwitch {
private:
  Switch sw;
  bool lastValue;
  unsigned long pressStartMs;
  unsigned long lastDebounceTime;
  const unsigned long debounceDelay = 50;
  bool isLongPress;

public:
  EventSwitch(uint8_t _pin, uint8_t _mode);
  void setup();
  SwEvent_t getEvent();
  bool getValue();
};

class RotarySwitch {
private:
  uint8_t pinA;
  uint8_t pinB;
  uint8_t pinC;
  uint8_t pinD;
  uint8_t mode;
  uint8_t position;
  uint8_t stablePosition;
  unsigned long lastChange;
  const unsigned long stabilityTime = 50;

  uint8_t getRawPos();

public:
  RotarySwitch(uint8_t _pinA, uint8_t _pinB, uint8_t _pinC, uint8_t _pinD,
               uint8_t _mode);
  void setup();
  bool refresh();
  uint8_t getPosition();
};

/* ROTARY SWITCH LOOKUP TABLE */
const uint8_t ROTARY_POS_MAP[16] = {
    0,  // 0x00
    14, // 0x01
    15, // 0x02
    13, // 0x03
    12, // 0x04
    10, // 0x05
    11, // 0x06
    9,  // 0x07
    8,  // 0x08
    6,  // 0x09
    7,  // 0x0A
    5,  // 0x0B
    4,  // 0x0C
    2,  // 0x0D
    3,  // 0x0E
    1   // 0x0F
};

#endif // SWITCHES_H