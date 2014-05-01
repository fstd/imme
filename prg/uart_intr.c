/* uart_intr.c - (C) 2014, Timo Buhrmester
 * interupt-driven uart
 * See README for contact-, COPYING for license information. */

#include "uart_intr.h"

// permitted buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128
#ifndef UART_RXBUF_SZ
#warning using default UART_RXBUF_SZ 8
#define UART_RXBUF_SZ 8
#endif

#ifndef UART_TXBUF_SZ
#warning using default UART_TXBUF_SZ 8
#define UART_TXBUF_SZ 8
#endif

#define UART_RXBUF_MASK (UART_RXBUF_SZ - 1) //for cheap modulo operation
#define UART_TXBUF_MASK (UART_TXBUF_SZ - 1) // i.e. & instead of %

#if(UART_TXBUF_SZ & UART_TXBUF_MASK || UART_TXBUF_SZ > 128 || UART_TXBUF_SZ < 1)
#error UART_TXBUF_SZ must be a power of two and between 1 and 128
#endif

#if(UART_RXBUF_SZ & UART_RXBUF_MASK || UART_RXBUF_SZ > 128 || UART_RXBUF_SZ < 1)
#error UART_RXBUF_SZ must be a power of two and between 1 and 128
#endif

static uint8_t rxbuf[UART_RXBUF_SZ]; //receive ring buffer
static uint8_t txbuf[UART_TXBUF_SZ]; //transmit ring buffer

static uint8_t rx_cons_ind; //uart_recv() reads at this offset
static uint8_t rx_prod_ind; //RXC ISR stores at this offset
static uint8_t tx_prod_ind; //uart_send() stores at this offset
static uint8_t tx_cons_ind; //UDRE ISR transmits at this offset

static volatile uint8_t rx_count; //number of octets in receive buffer
static volatile uint8_t tx_count; //number of octets in transmit buffer

ISR(USART_RXC_vect) //receive complete, buffer received octet
{
	if (rx_count >= UART_RXBUF_SZ) {
		if(UDR){} //overflow, dummy access UDR to discard octet
	} else {
		rxbuf[rx_prod_ind++ & UART_RXBUF_MASK] = UDR;
		++rx_count;
	}
}

ISR(USART_UDRE_vect) //data reg empty, transmit next one, or mask this interrupt
{
	if (tx_count) {
		UDR = txbuf[tx_cons_ind++ & UART_TXBUF_MASK];
		tx_count--;
	} else //no more data to transmit for now, disable this interrupt
		UCSRB &= ~(1<<UDRIE);
}

void
uart_init(uint16_t ubrr)
{
	UBRRH = ((uint8_t)(ubrr>>8)) & ~(1<<URSEL); //ensure URSEL is 0 (see DS)
	UBRRL = (uint8_t)ubrr;
	
	UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0); // 8N1
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
}

uint8_t
uart_recv(uint8_t *data, uint8_t max)
{
	uint8_t num = 0; // number of octets read from rx buffer
	uint8_t cnt = rx_count; //cache volatile

	while(cnt-- && max--)
		data[num++] = rxbuf[rx_cons_ind++ & UART_RXBUF_MASK];

	cli();   rx_count -= num;   sei();

	return num; //return number of octets we actually got
}

uint8_t
uart_send(uint8_t *data, uint8_t max)
{
	uint8_t num = 0; // number of octets buffered for transmission
	uint8_t cnt = tx_count; //cache volatile

	while(max-- && cnt++ < UART_TXBUF_SZ)
		txbuf[tx_prod_ind++ & UART_TXBUF_MASK] = data[num++];

	cli();   tx_count += num;   sei();
	UCSRB |= (1<<UDRIE); //enable data reg empty interrupt

	return num; //return number of octets we actually buffered for tx
}

uint8_t
uart_avail(void)
{
	return rx_count;
}

void
uart_put(uint8_t data)
{
	while(tx_count >= UART_TXBUF_SZ || !uart_send(&data, 1));
}

uint8_t
uart_tryput(uint8_t data)
{
	if (tx_count >= UART_TXBUF_SZ)
		return 0;
	return uart_send(&data, 1);
}

uint8_t
uart_get(void) //waits
{
	uint8_t c;
	while(!rx_count || !uart_recv(&c, 1));
	return c;
}

uint8_t
uart_txbufusage(void)
{
	uint8_t usg = tx_count;
	return usg;
}

//----- convenience ---------

void
uart_num8(uint8_t i)
{
	uint8_t hi = (i & 0xf0) >> 4;
	uint8_t lo = i & 0x0f;

	if (hi >= 10)
		hi += ('A' - 10);
	else
		hi += '0';

	if (lo >= 10)
		lo += ('A' - 10);
	else
		lo += '0';
	
	uart_put(hi);
	uart_put(lo);
}

void
uart_num16(uint16_t i)
{
	uart_num8(i>>8);
	uart_num8(i&0xff);
}

void
uart_num32(uint32_t i)
{
	uart_num16(i>>16);
	uart_num16(i&0xffff);
}

void
uart_puts(char *str)
{
	while(*str)
		uart_put(*str++);
}
