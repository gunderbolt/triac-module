/*****************************************
  Power Look-up Table

  Generated with the following settings:
      - UC_FREQ = 16000000
      - TIMER = 1
      - PRESCALER = 8
      - SAMPLES = 501
      - AC_FREQ = 60

  Generated on: Sat Jan 25 09:40:36 2020
  Author: Nick Gunderson
*****************************************/

#ifndef _POWER_LUT_H
#define _POWER_LUT_H

#include <stdint.h>

#define POWER_SAMPLES 501

extern const uint16_t power_lut[501];

#endif   /* _POWER_LUT_H */
