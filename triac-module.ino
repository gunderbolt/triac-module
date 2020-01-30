/* This program is used to control the RobotDyn AC Light Dimmer Module.

   It is meant to run on an Arduino with an ATmega328P chip (Uno, Nano, etc.).

   Copyright: 2020
   Author: Nick Gunderson
*/

#include <stdint.h>
#include "power_lut.h"

volatile bool enable_isr = false;
volatile byte t2_scaler = 0;
volatile uint32_t t1_hit = 0;
volatile uint32_t t2_hit = 0;
volatile unsigned long t2_start = 0;
volatile unsigned long t1_sum = 0;

void setup()
{
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    setup_interrupts();

    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
}

void loop()
{
    Serial.print(F("Off\n"));
    set_power(0);

    noInterrupts();
    print_hits();
    delay(5000);

    for (word i=0; i<15; i++)
    {
        set_power(i);
        Serial.println(i);
        print_hits();
        delay(100);
    }
    for (word i=15; i<POWER_SAMPLES; i++)
    {
        set_power(i);
        Serial.println(i);
        print_hits();
        delay(40);
    }
    Serial.print(F("On\n"));
    set_power(POWER_SAMPLES);
    delay(5000);
    for (word i=POWER_SAMPLES; i>15; i--)
    {
        set_power(i);
        Serial.println(i);
        print_hits();
        delay(40);
    }
    for (word i=15; i>0; i--)
    {
        set_power(i);
        Serial.println(i);
        print_hits();
        delay(100);
    }
}

void print_hits()
{
    noInterrupts();
    uint32_t t2 = t2_hit;
    t2_hit = 0;
    uint32_t t1 = t1_hit;
    t1_hit = 0;
    unsigned long sum = t1_sum;
    t1_sum = 0;
    interrupts();
    Serial.print(F("T2 Hits: "));
    Serial.println(t2);
    Serial.print(F("T1 Hits: "));
    Serial.println(t1);
    Serial.print(F("Ave Time: "));
    if (t1)
        Serial.println(sum / t1);
    else
        Serial.print(F("NaN\n"));

}

void set_power(uint16_t index)
{
    if (index == 0)
    {
        noInterrupts();
        // disable both timer compare interrupts
        TIMSK1 &= ~(1 << OCIE1A);
        TIMSK2 &= ~(1 << OCIE2A);
        interrupts();
        digitalWrite(LED_BUILTIN, LOW);
    }
    else if (index >= POWER_SAMPLES)
    {
        noInterrupts();
        // disable both timer compare interrupts
        TIMSK1 &= ~(1 << OCIE1A);
        TIMSK2 &= ~(1 << OCIE2A);
        interrupts();
        digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
        uint16_t p = power_lut[index];
        noInterrupts();
        // set compare match register to correspond to the given power
        OCR1A = p;
        // make sure the interrupt is enabled
        // enable timer 2 compare interrupt
        TIMSK2 |= (1 << OCIE2A);
        interrupts();
    }
}

void setup_interrupts()
{
    // https://www.instructables.com/id/Arduino-Timer-Interrupts/
    // These are specific to the ATMEL 328/168 (Arduino Uno, nano, etc.).

    noInterrupts();   // stop interrupts

    /*
    // set timer0 interrupt at 2kHz
    TCCR0A = 0;   // set entire TCCR0A register to 0
    TCCR0B = 0;   // same for TCCR0B
    TCNT0  = 0;   // initialize counter value to 0
    // set compare match register for 2khz increments
    OCR0A = 124;   // = (16*10^6) / (2000*64) - 1 (must be <256)
    // turn on CTC mode
    TCCR0A |= (1 << WGM01);
    // Set CS01 and CS00 bits for 64 prescaler
    TCCR0B |= (1 << CS01) | (1 << CS00);
    // enable timer compare interrupt
    TIMSK0 |= (1 << OCIE0A);
    */

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

    // set timer2 interrupt at 8kHz
    TCCR2A = 0;   // set entire TCCR2A register to 0
    TCCR2B = 0;   // same for TCCR2B
    TCNT2  = 0;   // initialize counter value to 0
    // set compare match register for ~1920 (1923) Hz increments
    OCR2A = 129;   // = (16*10^6) / (1920*8) - 1 (must be <256)
    // turn on CTC mode
    TCCR2A |= (1 << WGM21);
    // Set CS22 bit for 64 prescaler
    TCCR2B |= (1 << CS22);
    // leave timer compare interrupt disabled
    TIMSK2 &= ~(1 << OCIE2A);

    interrupts();   // allow interrupts
}

/*
ISR(TIMER0_COMPA_vect)   // Interrupt Service Routine for Timer 0.
{
}

*/

ISR(TIMER1_COMPA_vect)   // Interrupt Service Routine for Timer 1.
{
    digitalWrite(LED_BUILTIN, HIGH);
    // disable timer compare interrupt
    TIMSK1 &= ~(1 << OCIE1A);
    t1_hit++;
    t1_sum += (micros() - t2_start);
}

ISR(TIMER2_COMPA_vect)   // Interrupt Service Routine for Timer 2.
{
    // This code needs to run at ~120 Hz: 1923 Hz / 16
    //static unsigned byte t2_scaler = 0;
    t2_scaler++;
    t2_scaler &= 0xF;   // t2_scaler = t2_scaler % 16;
    if (t2_scaler == 0)
    {
        // Turn off the LED and enable Timer 1
        digitalWrite(LED_BUILTIN, LOW);
        // clear and enable timer compare interrupt
        TCNT1  = 0;   // initialize counter value to 0
        TIFR1 |= (1 << OCF1A);   // manually clear the interrupt vector.
        TIMSK1 |= (1 << OCIE1A);  // enable the interrupt.
        t2_hit++;
        t2_start = micros();
    }
}
