#include "Globals.h"
#include "EEPROM.h"
#include "Utils.h"
#include "Switches.h"

#include "BaseEffect.h"
#include "MidiMuteEffect.h"
#include "ChordGenEffect.h"
#include "DelayEffect.h"
#include "ArpEffect.h"

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

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} Rgb_t;

static const Rgb_t effectColours[NUM_EFFECTS] = {
  {255, 0, 0},
  {255, 0, 85},
  {255, 0, 170},
  {255, 0, 255},
  {0, 255, 255},
  {255, 255, 0}
};

static const Rgb_t midiColours[16] = {
  {255,0,0},   {255,128,0}, {255,255,0}, {128,255,0},
  {0,255,0},   {0,255,128}, {0,255,255}, {0,128,255},
  {0,0,255},   {128,0,255}, {255,0,255}, {255,0,128},
  {255,255,255},{128,128,128},{64,64,64},{255,64,64}
};

void pulseMidiColour(uint8_t index) {
  static uint8_t pulse = 0;
  static int8_t dir = 1;

  Rgb_t c = midiColours[index % 16];

  uint8_t r = (c.r * pulse) >> 8;
  uint8_t g = (c.g * pulse) >> 8;
  uint8_t b = (c.b * pulse) >> 8;

  setLed(r, g, b);

  pulse += dir * 1;

  if (pulse == 0 || pulse == 255)
      dir = -dir;
}

void handleClock() {
  if (currentEffect) currentEffect->handleClock();
}

void indicateModeChange(uint16_t flashTimeMs) {
  setLed(255, 0, 0);
  delay(flashTimeMs);
  setLed(0, 0, 0);
  delay(flashTimeMs);
  setLed(255, 0, 0);
  delay(flashTimeMs);
  setLed(0, 0, 0);
  delay(flashTimeMs);
  setLed(255, 0, 0);
  delay(flashTimeMs);
  setLed(0, 0, 0);
}

void indicateBoot() {
  uint8_t r = 255, g = 0, b = 0;
  uint8_t phase = 0;
  unsigned long currTime = millis();
  while (millis() - currTime < 3000) { // 3 seconds
    switch (phase) {
    case 0: // red -> yellow (increase green)
      if (g < 255) g++; else phase = 1;
      break;
    case 1: // yellow -> green (decrease red)
      if (r > 0) r--; else phase = 2;
      break;
    case 2: // green -> cyan (increase blue)
      if (b < 255) b++; else phase = 3;
      break;
    case 3: // cyan -> blue (decrease green)
      if (g > 0) g--; else phase = 4;
      break;
    case 4: // blue -> magenta (increase red)
      if (r < 255) r++; else phase = 5;
      break;
    case 5: // magenta -> red (decrease blue)
      if (b > 0) b--; else phase = 0;
      break;
    }

    setLed(r, g, b);
    delay(2); // This just looks correct when looking at led
  }
  setLed(0, 0, 0);
}

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

  // Set event handler
  hardwareMIDI.setHandleActiveSensing(handleActiveSense);

  // Turn thru off to control midi flow
  hardwareMIDI.turnThruOff();
  
  bool hasBeenInMidiSetup = false;
  // Fetch midi channel
  pedalState.midiChannel = EEPROM.read(EEPROM_MIDI_CHANNEL); // default to channel in eeprom
  if (pedalState.midiChannel > 16) {
    pedalState.midiChannel = 1;
  }

  /* SETUP MODE */
  // Enter if stomp held, otherwise fetch effect from EEPROM
  if (!stompSwitch.getValue()) {

    // Setup mode led indicator
    indicateModeChange(500);

    // Setup mode loop
    bool inSetup = true;
    bool inMidiSetup = false;

    while (inSetup) {

      // Check if we need to exit setup mode
      pedalState.stompEvent = stompSwitch.getEvent();
      switch(pedalState.stompEvent) {
        case Click: // Exit setup mode
          inSetup = false;
          inMidiSetup = false;
          break;
        case LongPress: // Enter MIDI channel setup
          inMidiSetup = !inMidiSetup;
          hasBeenInMidiSetup = true;
          break;
        default:
          break;
      }

      // Update and get position
      rotarySwitch.refresh();
      uint8_t pos = rotarySwitch.getPosition();

      if (inMidiSetup) {
        pedalState.midiChannel = pos + 1;
        pulseMidiColour(pos);
      } else {
        if (pos < NUM_EFFECTS) {
          setLed(effectColours[pos].r, effectColours[pos].g, effectColours[pos].b);
          pedalState.effectIdx = pos;
          EEPROM.write(EEPROM_EFFECT, pos);
        } else {
          setLed(0, 0, 0);
        }
      }
      
    }
  } else {
    uint8_t effect = EEPROM.read(EEPROM_EFFECT);
    if (effect < NUM_EFFECTS) pedalState.effectIdx = effect;
    else pedalState.effectIdx = E_MIDIMUTE;
    
    if (hasBeenInMidiSetup) EEPROM.write(EEPROM_MIDI_CHANNEL, pedalState.midiChannel);
  }

  // On boot, the pedal isn't active
  pedalState.isActive = false;

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
  case 4:
    currentEffect = new DelayEffect();
    break;
  case 5:
    currentEffect = new ArpEffect();
    break;
  default:
    currentEffect = new MidiMuteEffect(); // Default to MidiMute if out of range
    break;
  }

  // Set the clock event handler since this is unique to each effect
  hardwareMIDI.setHandleClock(handleClock);

  // Indicate boot led sequence
  indicateBoot();
}

void loop() {
  pedalState.stompEvent = stompSwitch.getEvent();
  pedalState.extEvent = extSwitch.getEvent();
  pedalState.rotaryMoved = rotarySwitch.refresh();
  pedalState.rotaryPos = rotarySwitch.getPosition();

  // Midi panic
  if (pedalState.stompEvent == ResetPress || pedalState.extEvent == ResetPress) {
    indicateModeChange(100);

    if (currentEffect) {
      currentEffect->handlePanic();
    }
  }

  // Process midi
  if (currentEffect) {
    currentEffect->process(&pedalState);
  }
}
