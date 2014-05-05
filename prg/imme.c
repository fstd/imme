/* imme.c - (C) 2014, Timo Buhrmester
 * imme interfacer
 * See README for contact-, COPYING for license information. */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/cpufunc.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "uart_intr.h"
#include "adc.h"

/* sizes: bool: 1; short, int, ptr, size_t: 2;  long: 4;  long long: 8 */

//pwm0: timer0A, pwm1: timer0B, pwm2: timer1A, pwm3: timer1B
#define PORT_DIN PORTA
#define PIN_DIN PINA
#define PBIT_DIN PA0
#define DDR_DIN DDRA
#define DBIT_DIN DDA0

#define PORT_RST PORTD
#define PBIT_RST PD3
#define DDR_RST DDRD
#define DBIT_RST DDD3

#define PORT_DOUT PORTD
#define PBIT_DOUT PD4
#define DDR_DOUT DDRD
#define DBIT_DOUT DDD4

#define PORT_CLK PORTD
#define PBIT_CLK PD5
#define DDR_CLK DDRD
#define DBIT_CLK DDD5

#define PORT_LED PORTD
#define PBIT_LED PD6
#define DDR_LED DDRD
#define DBIT_LED DDD6

#define PORT_LED2 PORTD
#define PBIT_LED2 PD2
#define DDR_LED2 DDRD
#define DBIT_LED2 DDD2


#define RSTON PORT_RST &= ~(1<<PBIT_RST)
#define RSTOFF PORT_RST |= (1<<PBIT_RST)

#define DATAHI PORT_DOUT |= (1<<PBIT_DOUT)
#define DATALO PORT_DOUT &= ~(1<<PBIT_DOUT)

#define CLKHI PORT_CLK |= (1<<PBIT_CLK)
#define CLKLO PORT_CLK &= ~(1<<PBIT_CLK)


#include "stackana.h" //makes available: uint16_t stack_avail(void);

void wdt_off(void);
void wdt_on(void);
/* (re)enable debug mode */
void
debug_enable(void)
{
	RSTOFF;
	CLKLO;
	DATALO;
	_delay_ms(10);
	RSTON;
	_delay_ms(10);
	CLKHI;
	_delay_ms(10);
	CLKLO;
	_delay_ms(10);
	CLKHI;
	_delay_ms(10);
	CLKLO;
	_delay_ms(10);
	RSTOFF;
	_delay_ms(10);
}

uint8_t
getbyte(void)
{
	/* tristate data output */
	DDR_DOUT &= ~(1<<DBIT_DOUT);
	PORT_DOUT &= ~(1<<PBIT_DOUT);

	uint8_t r = 0;
	for (int8_t bit = 7; bit >= 0; bit--) {
		CLKHI; //devices drives
		//_delay_us(1);

		if (bsample())
			r |= (1<<bit);

		CLKLO;

		//_delay_us(1);
	}

	/* enable data output */
	DDR_DOUT |= (1<<DBIT_DOUT);

	return r;
}

void
putbyte(uint8_t b)
{
	for (int8_t bit = 7; bit >= 0; bit--) {
		CLKHI;
		if ((b >> bit) & 1)
			DATAHI;
		else
			DATALO;

		//_delay_us(1);

		CLKLO; //device samples

		//_delay_us(1);
	}

	DATALO;
}


uint8_t
putget(uint8_t data)
{
	putbyte(data);
	return getbyte();
}

uint8_t
cmd_read_status(void) {
	return putget(0x34);
}

uint8_t
cmd_read_config(void) {
	return putget(0x24);
}

void
cmd_chip_erase(void) {
	putget(0x14);
}

void
cmd_resume(void) {
	putget(0x4C);
}

void
cmd_halt(void) {
	putget(0x44);
}

uint8_t
cmd_step_instr(void) {
	return putget(0x5c);
}

uint16_t
cmd_get_pc(void)
{
	putbyte(0x28);
	uint16_t r = getbyte();
	r <<= 8;
	r |= getbyte();
	return r;
}

uint16_t
cmd_get_chip_id(void)
{
	putbyte(0x68);
	uint16_t r = getbyte();
	r <<= 8;
	r |= getbyte();
	return r;
}

void
cmd_write_config(uint8_t cfg)
{
	putbyte(0x1d);
	putbyte(cfg);
	getbyte(); //discard
}

uint8_t
cmd_exec(uint8_t *bytes, uint8_t numbytes) //1 <= numbytes <= 3
{
	putbyte(0x54 + numbytes);

	for (uint8_t i = 0; i < numbytes; i++)
		putbyte(bytes[i]);

	return getbyte(); //to be discarded?
}

uint8_t
cmd_set_hw_brkpnt(uint8_t *threebytes)
{
	putbyte(0x3f);

	for (uint8_t i = 0; i < 3; i++)
		putbyte(threebytes[i]);

	return getbyte(); //to be discarded
}

uint8_t
cmd_step_replace(uint8_t *bytes, uint8_t numbytes) //1 <= numbytes <= 3
{
	putbyte(0x64 + numbytes);

	for (uint8_t i = 0; i < numbytes; i++)
		putbyte(bytes[i]);

	return getbyte(); //to be discarded
}


void
debug_dump(void)
{
	uint16_t st = stack_avail();
	uart_put((st>>8)&0xff);
	uart_put(st&0xff);
}

void
reset_target(void)
{
	RSTOFF;
	_delay_ms(10);
	RSTON;
	_delay_ms(10);
}


int
main(void)
{
	cli();
	wdt_off();

	PORT_RST |= (1<<PBIT_RST);
	DDR_RST |= (1<<DBIT_RST);

	DDR_LED |= (1<<DBIT_LED);
	DDR_LED2 |= (1<<DBIT_LED2);
	DDR_CLK |= (1<<DBIT_CLK);
	DDR_DOUT |= (1<<DBIT_DOUT);

	//enable some pullups for unused pins
	PORTA = ~(1<<PBIT_DIN); //it's all on PORTD EXCEPT THE ADC IN
	PORTB = 0xff; //it's all on PORTD
	PORTC = 0xff; //it's all on PORTD
	PORTD |= ~((1<<PBIT_DOUT)|(1<<PBIT_CLK)|(1<<PBIT_LED)|(1<<PBIT_LED2)); //we want the pullup on PBIT_DIN
	//PD5-6 are UART
	//PD0-4 are what we use to access the thingy

	adc_init(2);
	uart_init(16); UCSRA |= (1<<U2X); //115200 baud

	sei();


	/* flash LED to signal we've booted */

	PORT_LED |= (1<<PBIT_LED);
	PORT_LED2 |= (1<<PBIT_LED2);
	_delay_ms(100);
	PORT_LED &= ~(1<<PBIT_LED);
	PORT_LED2 &= ~(1<<PBIT_LED2);
	_delay_ms(100);
	PORT_LED |= (1<<PBIT_LED);
	PORT_LED2 |= (1<<PBIT_LED2);
	_delay_ms(100);
	PORT_LED &= ~(1<<PBIT_LED);
	PORT_LED2 &= ~(1<<PBIT_LED2);
	_delay_ms(100);



	bool echo = false;

	for(;;) {
		PORT_LED2 |= (1<<PBIT_LED2);
		uint8_t c = 0;
		uint16_t db = 0;
		for (;;) {
			if (uart_avail()) {
				c = uart_get();
				break;
			}

			if (db == 0 && !(PINA & (1<<PA1))) {
				uart_put('x');
				db = 30000;
			} else if (db > 0)
				db--;
		}
		PORT_LED2 &= ~(1<<PBIT_LED2);
		if (echo)
			uart_put(c);

		switch (c) {
		case '?'://ping MCU
			uart_put('!');
			break;

		case '!'://debug dump
			debug_dump();
			break;

		case 'e'://toggle echo
			echo = !echo;
			break;

		case 'R'://reset ourself
			cli(); wdt_on(); for(;;);

		case 'r'://reset target
			reset_target();
			break;

		case 'D'://enable debug mode
			debug_enable();
			uart_put('D');
			break;

		case 'i':{//get chip id
			uint16_t id = cmd_get_chip_id();
			uart_put((id >> 8)&0xff);
			uart_put(id&0xff);
			break;}
		case 'p':{//get pc
			uint16_t pc = cmd_get_pc();
			uart_put((pc >> 8)&0xff);
			uart_put(pc&0xff);
			break;}
		case 's'://get status byte
			uart_put(cmd_read_status());
			break;
		case 'H'://halt target cpu
			cmd_halt(), uart_put('H');
			break;
		case 'h'://resume target cpu
			cmd_resume(), uart_put('h');
			break;
		case 'c'://read cfg byte
			uart_put(cmd_read_config());
			break;
		case 'C':{//write cfg byte
			uint8_t c = uart_get();
			cmd_write_config(c), uart_put(c);
			break;}
		case 'E'://chip erase
			cmd_chip_erase(), uart_put('E');
			break;
		case 'I':{//run instruction
			uint8_t num = uart_get();
			uint8_t c = 0;
			uint8_t d[3];
			while (c < 3 && num) {
				d[c++] = uart_get();
				num--;
			}

			uart_put(cmd_exec(d, c));

			break;}
		case 'N':{//step replace instruction
			uint8_t num = uart_get();
			uint8_t c = 0;
			uint8_t d[3];
			while (c < 3 && num) {
				d[c++] = uart_get();
				num--;
			}

			uart_put(cmd_step_replace(d, c));

			break;}
		case 'n'://step next instruction
			uart_put(cmd_step_instr());
			break;
		case 'B':{//set hw breakpoint
			uint8_t d[3];
			d[0] = uart_get();
			d[1] = uart_get();
			d[2] = uart_get();
			uart_put(cmd_set_hw_brkpnt(d));

			break;}
		case '>':{//put a raw byte
			uint8_t b = uart_get();
			putbyte(b), uart_put(b);
			break;}
		case '<'://get a raw byte
			uart_put(getbyte());
			break;

		case '\r':
		case '\n':
			break;

		default:
			uart_put('?');

		}
	}
}
