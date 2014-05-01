/* adc.h - (C) 2014, Timo Buhrmester
 * atmega16 ADC control interface
 * See README for contact-, COPYING for license information. */

#ifndef ADC_H
#define ADC_H 1
#include <stdint.h>
uint16_t sample(uint8_t chan);
uint16_t xsample(uint8_t chan);
void adc_init(uint8_t adps); //0-7
#endif /* ADC_H */

