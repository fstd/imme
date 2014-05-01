/* adc.c - (C) 2014, Timo Buhrmester
 * atmega16 ADC control
 * See README for contact-, COPYING for license information. */

#include "adc.h"
#include <avr/io.h>
#include <avr/cpufunc.h>

uint16_t
sample(uint8_t chan)
{
	ADMUX = (ADMUX & 0xf0) | chan;
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC));
	uint16_t res = ADCL;
	res |= (ADCH<<8);
	return res;
}

uint16_t xsample(uint8_t chan) {
	uint16_t res = 0;
	for(uint8_t i = 0; i < 4; i++) {
		res += sample(chan);
	}
	return res >> 2;
}

void
adc_init(uint8_t adps)
{
	ADMUX = (1<<REFS1)|(1<<REFS0); //selects 2.56V on atmega16
	_NOP();
	ADCSRA = 0;
	_NOP();
	ADCSRA = (1<<ADEN) | (adps<<ADPS0);
	_NOP();
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC));
}
