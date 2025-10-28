# MidiKameleon
#### A MIDI effects pedal based on the MidiMute hardware

## General Flow

### Start Up
1. Setup hardware, MCU pins and MIDI
2. If stomp is held down, Enter **Effect Selection**:
    - Check for stomp press to exit effect selection
    - If rotary position is within range of number of effects, select that mode
      and write it to EEPROM
3. Otherwise, just read the last used effect from EEPROM
4. Instantiate the correct effect object which in turn sets up it's default state
5. Set the midi clock handler

### Loop
1. Update the state object with the hardware events and switch position
2. Check for a ResetPress (3 secs) to perform a MIDI panic
3. Call the current effect class' process function


## Architecture
There's a base effect class (BaseEffect) which all effects inherit from. This ensures that each effect
must implement panic and clock handlers, as well as a process function.

### Panic Handler
Essentially, will be called when the footswitch has been held down for longer than 3 seconds. The purpose of this 
function is to stop all midi signals, as well as clear out any data structures that may hold anything related to sending midi,
for example, the ArpEffect's noteList, which holds note data for iterating through for arpeggiation.

### Clock Handler
This allows a per-effect handling of clock signals, which is useful for clocked effects such as delay and arp

### Process
The main function responsible for processing the incoming MIDI data


## State Struct
This struct allows for a centralised location of all hardware states such as the current pedal state (active/bypass),
switch events and rotary switch postion. It simply makes it clean and easy to pass this information to the effect class' process
function. It's also flexible as it means that events can be handled differently for different effects. 


## Clock handling
MIDI hardware is interesting because some devices send clock constantly (Korg Minilogue), Some send it when a sequence is 
playing (Moog Grandmother, Korg Drumlogue), And some won't send clock at all. This means that if clock isn't present on the
MIDI in, The MidiKameleon has to generate it itself. To achieve this, I use the following general process:
1. If no clock has been received for X amount of time, Switch to using an internal timer, and use the external footswitch as a
tap tempo.
2. Process MIDI based on the currenly clock source

Clock speed is indicated with the LED, and you should see it switch over if clock is stopped, or supplied. Obviously, the internal
timer clock isn't as accurate, but it allows people without access to a synth with clock to use the clocked effects.

NOTE: Due to the limitations of MIDI, we recommend NOT creating a MIDI loop
(ie. device MIDI OUT -> MidiKameleon MIDI IN, MidiKameleon MIDI OUT -> device MIDI IN). It just creates weird problems and isn't how the
pedal is meant to be used anyway.
    

