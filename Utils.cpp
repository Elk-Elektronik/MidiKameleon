#include "Utils.h"

void setLed(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(LED_R_PIN, r);
  analogWrite(LED_G_PIN, g);
  analogWrite(LED_B_PIN, b);
}

void sendMidiBoth(midi::MidiType type, uint8_t note, uint8_t velocity,
                  uint8_t channel) {
  hardwareMIDI.send(type, note, velocity, channel);
  usbMIDI.send(type, note, velocity, channel);
}

void sendMidiClock() {
  hardwareMIDI.sendClock();
  usbMIDI.sendClock();
}

void sendMidiStart() {
  hardwareMIDI.sendStart();
  usbMIDI.sendStart();
}

void sendMidiStop() {
  hardwareMIDI.sendContinue();
  usbMIDI.sendContinue();
}

void sendMidiContinue() {
  hardwareMIDI.sendContinue();
  usbMIDI.sendContinue();
}
