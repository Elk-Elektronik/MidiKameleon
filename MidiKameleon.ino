#include "Globals.h"
#include "EEPROM.h"
#include "Utils.h"
#include "Switches.h"

#include "BaseEffect.h"
#include "MidiMuteEffect.h"
#include "ChordGenEffect.h"

/* MIDI INIT */
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, hardwareMIDI);
USBMIDI_CREATE_INSTANCE(1, usbMIDI);

/* SWITCHES */
EventSwitch stompSwitch(SW_PIN, INPUT_PULLUP);
EventSwitch extSwitch(EXT_SW_PIN, INPUT_PULLUP);
RotarySwitch rotarySwitch(ROT_A_PIN, ROT_B_PIN, ROT_C_PIN, 
                          ROT_D_PIN, INPUT_PULLUP);

/* STATES */
State_t pedalState;
BaseEffect* currentEffect = nullptr;

/* EVENT HANDLERS */
void handleActiveSense() {
  hardwareMIDI.sendActiveSensing();
  usbMIDI.sendActiveSensing();
}

void handleClock() {
  if (currentEffect) currentEffect->handleClock();
}

/*
unsigned long currentTaps[2] = {0};
unsigned long lastTap = 0;
unsigned long lastBlink = 0;
bool ledOn = false;

void tap() {
  currentTaps[1] = currentTaps[0];
  currentTaps[0] = millis() - lastTap;
  lastTap = millis();
}
*/

void setup() {
  /* SWITCHES */
  stompSwitch.setup();
  extSwitch.setup();
  rotarySwitch.setup();

  /* LEDS */
  pinMode(LED_BUILTIN_RX, OUTPUT);
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);

  /* MIDI */
  // Begin DIN and USB midi 
  hardwareMIDI.begin(MIDI_CHANNEL_OMNI);
  usbMIDI.begin(MIDI_CHANNEL_OMNI);

  // Turn thru off to control midi flow
  hardwareMIDI.turnThruOff();

  // Set event handler
  hardwareMIDI.setHandleActiveSensing(handleActiveSense);

  /* SETUP MODE */
  // Enter if stomp held, otherwise fetch effect from EEPROM
  if (!stompSwitch.getValue()) {
    
    // Setup mode led indicator
    unsigned long now = millis();
    uint16_t i = 0;
    while (millis() - now < 3000) { // Change colour for 3sec
      if (i < 256) {
        setLed(160, 255, i); // Light green
      }
      i += 10;
      delay(200);
    }

    // Setup mode loop
    bool inSetup = true;
    while (inSetup) {

      // Check if we need to exit setup mode
      pedalState.stompEvent = stompSwitch.getEvent();
      switch(pedalState.stompEvent) {
        case Click:
          inSetup = false;
          break;
        default:
          break;
      }

      // Update the current effect
      rotarySwitch.refresh();
      uint8_t pos = rotarySwitch.getPosition();
      if (pos < NUM_EFFECTS) {
        setLed(15*pos, 15+pos, (4*pos)%15);
        pedalState.effectIdx = pos;
        EEPROM.write(EEPROM_EFFECT, pos);
      } else {
        setLed(0, 0, 0);
      }
    }
  } else {
    uint8_t effect = EEPROM.read(EEPROM_EFFECT);
    if (effect < NUM_EFFECTS) pedalState.effectIdx = effect;
    else pedalState.effectIdx = E_MIDIMUTE;
  }

  pedalState.isActive = false; // TODO: Change this to read from EEPROM

  // This probably isn't the best way to do this, but it
  // means that only one effect is instantiated at any one time
  switch (pedalState.effectIdx) {
  case 0:
    currentEffect = new MidiMuteEffect();
    break;
  case 1:
    currentEffect = new ChordGenEffect(1);
    break;
  case 2:
    currentEffect = new ChordGenEffect(2);
    break;
  case 3:
    currentEffect = new ChordGenEffect(3);
    break;
  default:
    currentEffect = new MidiMuteEffect(); // Default to MidiMute if out of range
    break;
  }

  // Set the clock event handler since this is unique to each effect
  hardwareMIDI.setHandleClock(handleClock);
}

void loop() {
  pedalState.stompEvent = stompSwitch.getEvent();
  pedalState.extEvent = extSwitch.getEvent();
  pedalState.rotaryMoved = rotarySwitch.refresh();
  pedalState.rotaryPos = rotarySwitch.getPosition();

  if (currentEffect) {
    currentEffect->process(&pedalState);
  }

/*
  SwEvent_t event = extSwitch.getEvent();
  switch (event) {
    case Click: 
      tap();
      break;
    default:
      break;
  }

  unsigned long average = ((currentTaps[0] + currentTaps[1]) / 2);
  unsigned long t = millis();
  if (t - lastBlink > average) {
    setLed(200, 200, 200);
    lastBlink = t;
    ledOn = true;
  }

  if (ledOn && t - lastBlink > average/2) {
    setLed(0, 0, 0);
    ledOn = false;
  }

  Serial.print(currentTaps[0]);
  Serial.print(" : ");
  Serial.print(currentTaps[1]);
  Serial.print(" : ");
  Serial.println((currentTaps[0] + currentTaps[1]) / 2);
*/
}
