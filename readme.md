# TRIAC Module

This project is designed to interface with a TRIAC Module using an Arduino Nano
or Uno. Modifications may need to be made for other versions of Arduino. The
module used may go under other names as well:

- Arduino Light Dimmer
- Arduino Dimmer Module
- Arduino AC Light Dimmer Controller

I purchased one by RobotDyn.

For interfacing with Arduino, it has a Zero-cross Detector signal and a pin to
the TRIAC's gate.

# TRIAC Operation

[Wikipedia](https://en.wikipedia.org/wiki/TRIAC) does a great job of explaining
TRIACs; in particular see the animation in the Application section.

Basically, when the TRIAC's gate is set, the TRIAC will allow current through it
until the voltage across it is too low (e.g.: Zero). In order to dim a light to
a given level, you need to look for the zero crossing of the AC input, wait the
desired amount of time, and then set the gate.

One of the signals to the Arduino is the AC Zero Crossing, which becomes set
(high) when the AC voltage is near zero. We set up an interrupt on that pin,
that way we can set another interrupt for exactly how long to wait for a given
power level.

## Set Up Interrupts

The AC Zero-crossing interrupt is set in the `set_power` function ([see Set
Power Level](#set-power-level)). Prior to that, the pin has to be set as an
input.

The interrupt after the wait time completes is done using one of the Arduino's
Timers. Timers can be used to guage how much time has passed, or to signal when
something should be done.

The Atmel 328P has three timers. Timer0 is already used by Arduino functions
such as `millis` and `delay`. Timer1 has a higher accuracy than Timer2, so it
was chosen.

Refer to the Atmel 328P Datasheet and the comments in the `setup_interrupts`
function for more details.

## Set Power Level

The `set_power` function first determines if the TRIAC output should be always
on (i.e.: full power) or off (i.e.: no power). If so, it disables the Timer1 and
Zero-crossing interrupts and sets the TRIAC Pin appropriately.

Otherwise, if a partial power is desired, the Timer1 interrupt is set as
follows. The desired power index (a range from 0-1023) is converted to the
Timer1 compare value and set to the Timer1 Compare register. It then turns on
the zero-crossing interrupt (the Timer1 interrupt will be turned on later).

## Pin Crossing Interrupt

This interrupt is enabled in the `set_power` function ([see Set Power
Level](#set-power-level)).

The function `isr_ac_zero_crossing` is called when the zero crossing interrupt
occurs.

Its job is to turn off the TRIAC, zero out Timer1 and enable its interrupt. That
is what is used to wait a given amount of time to achieve a given power level.

If there is noise on the AC line, this interrupt can be triggered multiple times
per zero-crossing.

## Timer1 Interrupt

This interrupt is enabled during the [Pin Crossing
Interrupt](#pin-crossing-interrupt) and disabled during its routine.

Its job is simply to turn on the TRIAC (i.e.: set the gate high) and disable
itself. It does this in the `ISR(TIMER1_COMPA_vect)` block. (This is the way to
specify what happens during the ISR, or Interrupt Service Routine, for the ATMEL
328P.)

# Notes / TODO

- For very low power levels, noise can produce a very noticeable flicker effect
  on the LED lightbulbs. Debouncing the zero-crossing input signal may even
  that out.

# Author

Nick Gunderson

Thanks for checking this project out!
