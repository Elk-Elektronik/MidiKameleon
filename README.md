# MidiKameleon
#### A MIDI effects pedal based on the MidiMute hardware


## General Flow

#### Start Up
1. Setup hardware, MCU pins and MIDI
2. If stomp is held down, Enter **Effect Selection**:
    - Check for stomp press to exit effect selection
    - If rotary position is within range of number of effects, select that mode
      and write it to EEPROM
3. Otherwise, just read the last used effect from EEPROM
4. Instantiate the correct effect object which in turn sets up it's default state
5. Set the midi clock handler

#### Loop
1. Update the state object with the hardware events and switch position
2. Check for a ResetPress (3 secs) to perform a MIDI panic
3. Call the current effect class' process function


## Architecture
There's a base effect class (BaseEffect) which all effects inherit from. This ensures that each effect
must implement panic and clock handlers, as well as a process function.

#### Panic Handler
Essentially, will be called when the footswitch has been held down for longer than 3 seconds. The purpose of this 
function is to stop all midi signals, as well as clear out any data structures that may hold anything related to sending midi,
for example, the ArpEffect's noteList, which holds note data for iterating through for arpeggiation.

#### Clock Handler
This allows a per-effect handling of clock signals, which is useful for clocked effects such as delay and arp

#### Process
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


## Switches
`Switches.cpp` and `Switches.h` contain all the code responsible for peripherals. There are 3 classes:
    - Switch: The base class that simply has the pin, input mode, and the current value.
    - EventSwitch: An extension on Switch that detects clicks, long presses, and extra long presses. NOTE: Long press is detected on release,
                   while extra long press is detected on holding for certain length (does not require release)
    - RotarySwitch: A switch that keeps track of the rotary position.


## Effects

#### MidiMute
This allows you to choose specific channels to mute midi data on. Has a ChannelMute (class) for each midi channel (1-16). This holds information about
the channels mute state. The process function then simply determines if the pedal is active and if so, will pass midi data through if the channel isnt muted,
and not pass it through if it is.

To mute channels, Hold and release the switch for long press. This will toggle the rotary switch position's channel mute's mute state. Turning the rotary will 
indicate the channels current state.

Note that it should always pass through clock and active sense midi signals no matter the channel mute's state.

External footswitch follows stomp switch.

#### Chord Generator
Generates chords from the base note. Basically, there are 3 chord banks defined with each chord note being the semitone offsets from the root. For example a major chord would be
[0, 4, 7] (root, 3, 5). The arrays in the chord banks have a number before this array, which is the length of the array (the number of notes).

When active, this will simply select the rotary switch's index and use that to index into the chord bank array. It then just plays all the note offsets as noteOn/noteOff midi signals.

When the rotary switch changes position, it calculates what notes are different between the **currently held chord** and the **new chord** and saves them to a list. Then when the currently held chord is released, these notes are sent note off. This is to avoid hanging notes when changing chord while playing.

External footswitch follows stomp switch.

#### Delay
Repeats the noteOn/noteOff midi signals based on the number of repeats, supplied clock speed (internal or external) and the note division. A `DelayNote_t` hold information about the note such as channel, velocity and the actual note, as well as the last play time and when the initial note was released. When a note is played, a delay note is created and added to the array (implemented as a circular buffer), or if the note is already present, it resets it. The main loop then checks each note in the circular buffer and triggers noteOn/noteOff midi signals based on if a delay should happen or not, and then reduces velocity based on the number of repeats.

The rotary selects number of repeats. Hold and release switch for long press to enter division mode where the rotary then selects note division. 

External footswitch functions as tap tempo ONLY when no external clock is supplied.

#### Arpeggiator
Arpeggiates the held notes in sync with the clock. The `ArpList` class holds the notes for arpeggiation as well as info about the current mode and the direction state (currently moving up/down). This class is responsible for sorting the notes as they're inserted, chosing the octave for a note, and providing the note required for arpeggiation. 

The process function basically just checks switch states and led colours. When external clock is supplied, the arp stepping is done in the clock event handler. This could probably be improved as a simple flag, which is then checked in the process function and all calculations are done there. If no clock is supplied, this is done in the main loop. 

It also has a step mute functionality. to get to that, hold and release for long press and then clicking the footswitch will mute/unmute the step at the rotary switch position (total of 16 steps). When a note on falls on a muted step, it does not play. 
