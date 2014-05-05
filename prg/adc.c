/* adc.c - (C) 2014, Timo Buhrmester
 * atmega16 ADC control
 * See README for contact-, COPYING for license information. */

#include "adc.h"
#include <avr/io.h>
#include <avr/cpufunc.h>

uint8_t
bsample(void)
{
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC));
	return ADCH;
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
