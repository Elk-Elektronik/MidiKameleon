#include "Switches.h"

Switch::Switch(uint8_t _pin, uint8_t _mode) {
  pin = _pin;
  mode = _mode;
}

void Switch::setup() { pinMode(pin, mode); }

void Switch::refresh() { value = digitalRead(pin); }

bool Switch::getValue() { return value; }

EventSwitch::EventSwitch(uint8_t _pin, uint8_t _mode) : sw(_pin, _mode) {
  // probably need to do something clever based on _mode and default values here
  // or pass a param to say which is pressed and not pressed Set _last_value to
  // true so you don't get a phantom click on boot
  lastValue = true;
}

void EventSwitch::setup() { sw.setup(); }

SwEvent_t EventSwitch::getEvent() {
  sw.refresh();
  SwEvent_t event = NoEvent;
  bool currentValue = sw.getValue();
  unsigned long now = millis();

  if (lastValue != currentValue && (now - lastDebounceTime > debounceDelay)) { // There's a change
    lastDebounceTime = now;
    lastValue = currentValue;

    // Switch is pressed
    if (!currentValue) {
      pressStartMs = now;
      isResetPress = false;

     // Switch is released and was held down for long press but wasn't a reset press
    } else if (!isResetPress && (now - pressStartMs) > LONG_PRESS) {
      event = LongPress;
      
    // Switch is released but wasn't a reset press
    } else  if (!isResetPress) {
      event = Click;
    }
  // There's no change and no event occured yet and switch is held for reset press time
  } else if (!currentValue && (now - pressStartMs) > RESET_PRESS && !isResetPress) {
    event = ResetPress;
    isResetPress = true;
  }

  return event;
}

bool EventSwitch::getValue() {
  sw.refresh();
  return sw.getValue();
}

RotarySwitch::RotarySwitch(uint8_t _pinA, uint8_t _pinB, uint8_t _pinC,
                           uint8_t _pinD, uint8_t _mode) {
  pinA = _pinA;
  pinB = _pinB;
  pinC = _pinC;
  pinD = _pinD;
  mode = _mode;
}

void RotarySwitch::setup() {
  pinMode(pinA, mode);
  pinMode(pinB, mode);
  pinMode(pinC, mode);
  pinMode(pinD, mode);
  position = getRawPos();
  stablePosition = position;
  lastChange = millis();
}

bool RotarySwitch::refresh() {
  unsigned long now = millis();
  uint8_t currentPos = getRawPos();

  // Update the position and timer if
  // pos and the read pos is different
  if (position != currentPos) {
    position = currentPos;
    lastChange = now;
  }

  // If the position is different from the stable position,
  // check when it changed and update accordingly
  if (position != stablePosition && now - lastChange > stabilityTime) {
    stablePosition = position;
    lastChange = now;
    return true;
  }
  return false;
}

uint8_t RotarySwitch::getRawPos() {
  uint8_t pos = 0;
  uint8_t mask = 0;

  mask = digitalRead(pinA) << ROT_A_BIT;
  pos |= mask;

  mask = digitalRead(pinB) << ROT_B_BIT;
  pos |= mask;

  mask = digitalRead(pinC) << ROT_C_BIT;
  pos |= mask;

  mask = digitalRead(pinD) << ROT_D_BIT;
  pos |= mask;

  return pos;
}

uint8_t RotarySwitch::getPosition() { return ROTARY_POS_MAP[stablePosition]; }
