/* This program is used to control the RobotDyn AC Light Dimmer Module.

   It is meant to run on an Arduino with an ATmega 328P/168 chip (Uno, Nano,
   etc.).

   See LICENSE for license and copyright info.

   Copyright: 2020
   Author: Nick Gunderson
*/

#include <stdint.h>
#include "power_lut.h"


const byte PIN_TRIAC = 2;
// PIN_AC_ZERO_CROSSING needs to be on either pin 2 or 3.
const byte PIN_AC_ZERO_CROSSING = 3;

// variables related to the interrupts
volatile uint32_t t1_hit = 0;
volatile uint32_t zc_hit = 0;
volatile unsigned long t2_start = 0;
volatile unsigned long t1_sum = 0;
volatile unsigned long t2_sum = 0;


void setup()
{
    // The built-in LED doesn't need to be on.
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    pinMode(PIN_TRIAC, OUTPUT);
    digitalWrite(PIN_TRIAC, LOW);

    // There is already a Pull-up resistor on the dimmer board.
    pinMode(PIN_AC_ZERO_CROSSING, INPUT);

    Serial.begin(9600);

    setup_interrupts();

    set_power(250);
}

void loop()
{
    // Uncomment this to have the dimmer loop in a low to high and back
    // pattern.
    //full_cycle();
}

void full_cycle()
{
    set_power(0);
    Serial.print(F("0 - Off\n"));

    print_hits();
    delay(500);

    for (word i=25; i<POWER_SAMPLES-10; i+=5)
    {
        set_power(i);
        Serial.println(i);
        print_hits();
        delay(50);
    }
    Serial.print(F("501 - On\n"));
    set_power(POWER_SAMPLES);
    delay(500);
    for (word i=POWER_SAMPLES-6; i>30; i-=5)
    {
        set_power(i);
        Serial.println(i);
        print_hits();
        delay(50);
    }
}

void print_hits()
{
    noInterrupts();
    uint32_t zc = zc_hit;
    zc_hit = 0;
    uint32_t t1 = t1_hit;
    t1_hit = 0;
    unsigned long sum = t1_sum;
    t1_sum = 0;
    interrupts();

    Serial.print(F("ZC:T1 Hits: "));
    Serial.print(zc);
    Serial.print(":");
    Serial.print(t1);
    Serial.print(F(" Ave Time: "));
    if (t1)
        Serial.println(sum / t1);
    else
        Serial.println(F("NaN"));

}

void set_power(uint16_t index)
{
    if (index == 0)
    {
        noInterrupts();
        digitalWrite(PIN_TRIAC, LOW);
        // disable timer 1 compare interrupt and zero-c interrupt
        TIMSK1 &= ~(1 << OCIE1A);
        detachInterrupt(digitalPinToInterrupt(PIN_AC_ZERO_CROSSING));
        interrupts();
    }
    else if (index >= POWER_SAMPLES)
    {
        noInterrupts();
        digitalWrite(PIN_TRIAC, HIGH);
        // disable timer 1 compare interrupt and zero-c interrupt
        TIMSK1 &= ~(1 << OCIE1A);
        detachInterrupt(digitalPinToInterrupt(PIN_AC_ZERO_CROSSING));
        interrupts();
    }
    else
    {
        uint16_t p = power_lut[index];
        noInterrupts();
        // set compare match register to correspond to the given power
        OCR1A = p;
        // enable zero crossing interrupt
        attachInterrupt(
                digitalPinToInterrupt(PIN_AC_ZERO_CROSSING),
                isr_ac_zero_crossing,
                RISING);
        interrupts();
    }
}

void setup_interrupts()
{
    // See the following page for a really useful introduction to Arduino Timer
    // interrupts. This code was based off of Amanda Ghassaei's code.
    //
    // https://www.instructables.com/id/Arduino-Timer-Interrupts/
    //
    // These are specific to the ATMEL 328/168 (Arduino Uno, nano, etc.).

    noInterrupts();   // stop interrupts

    // set timer1 interrupt at 1Hz
    TCCR1A = 0;   // set entire TCCR1A register to 0
    TCCR1B = 0;   // same for TCCR1B
    TCNT1  = 0;   // initialize counter value to 0 -- page 111
    // set compare match register too large
    OCR1A = 65535;   // = (16*10^6) / (1*8) - 1 (must be <65536)
    // turn on CTC mode -- page 109
    TCCR1B |= (1 << WGM12);
    // Set CS11 bits for 8 prescaler -- page 110
    TCCR1B |= (1 << CS11);
    // leave timer compare interrupt disabled -- page 112
    TIMSK1 &= ~(1 << OCIE1A);

    interrupts();   // allow interrupts
}


ISR(TIMER1_COMPA_vect)   // Interrupt Service Routine for Timer 1.
{
    // Turn on the TRIAC
    digitalWrite(PIN_TRIAC, HIGH);

    // disable timer compare interrupt
    TIMSK1 &= ~(1 << OCIE1A);

    // debugging stuff
    t1_hit++;
    t1_sum += (micros() - t2_start);
}

void isr_ac_zero_crossing()
{
    // Turn off the TRIAC
    digitalWrite(PIN_TRIAC, LOW);

    // debugging stuff
    zc_hit++;
    t2_start = micros();

    // clear and enable timer compare interrupt
    TCNT1  = (uint16_t) 0;   // initialize counter value to 0
    TIFR1 |= (1 << OCF1A);   // manually clear the interrupt vector.
    TIMSK1 |= (1 << OCIE1A);  // enable the interrupt.
}
