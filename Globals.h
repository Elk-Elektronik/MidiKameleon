#ifndef GLOBALS_H
#define GLOBALS_H

#include <MIDI.h>
#include <USB-MIDI.h>
#include <stdint.h>

/* SWITCHES */
#define SW_PIN 2
#define EXT_SW_PIN 3

/* ROTARY SWITCH */
#define ROT_A_PIN 10
#define ROT_B_PIN 14
#define ROT_C_PIN 15
#define ROT_D_PIN 16

#define ROT_A_BIT 3
#define ROT_B_BIT 2
#define ROT_C_BIT 1
#define ROT_D_BIT 0

/* LEDS */
#define LED_R_PIN 5
#define LED_G_PIN 9
#define LED_B_PIN 6

/* TIMEOUTS */
#define CLOCK_TIMEOUT 1000
#define EXT_TIMEOUT 5000

/* EFFECTS */
enum Effects {
  E_MIDIMUTE,
  E_CHORDGEN_B1,
  E_CHORDGEN_B2,
  E_CHORDGEN_B3,
  E_DELAY,
  E_ARP,
  NUM_EFFECTS
};

/* SWITCH EVENTS */
typedef enum {
  NoEvent,
  Click,
  LongPress,
  ResetPress,
} SwEvent_t;

/* PEDAL STATE STRUCT */
typedef struct {
  uint8_t effectIdx; // The current effect's rotary number (0-15)
  bool isActive; // Is the pedal active?
  SwEvent_t stompEvent; // The current event emitted by the stomp switch
  SwEvent_t extEvent; // The current event emitted by the external switch
  bool rotaryMoved; // Is the rotary position different from last time?
  uint8_t rotaryPos; // The current position of the rotary switch

} State_t;

/* EEPROM SAVE LOCATIONS */
// NOTE: The save locations are 1 byte (8 bits) long
#define EEPROM_EFFECT 0x00 // The currently active effect
#define EEPROM_MUTE_BASE 0x10 // Midi mute location (takes 16 * 8bits space)
#define EEPROM_ARP_BASE 0x50 // Step save location

/* TIMERS */
#define LONG_PRESS 1000
#define RESET_PRESS 3000
#define LED_TIME_MS 1000

/* MIDI */
#define MIDI_CLOCKS_PER_QUARTER 24

/* HARDWARE MIDI */
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> hardwareMIDI;

/* USB MIDI */
extern midi::MidiInterface<usbMidi::usbMidiTransport> usbMIDI;

#endif // GLOBALS_H
