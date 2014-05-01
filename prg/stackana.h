//stack usage analyzation
void stack_cinit(void) __attribute__ ((naked)) __attribute__ ((section (".init1")));

void
stack_cinit(void)
{
	asm volatile("eor r1, r1");
	//r1 cleared

	asm volatile("out 0x3f, r1");
	//SREG cleared

	asm volatile("ldi r28, 0x04");
	asm volatile("out 0x3e, r28");
	asm volatile("ldi r28, 0x5f");
	asm volatile("out 0x3d, r28");
	//SP initialized (set to RAMEND)

	asm volatile("wdr");
	asm volatile("in r24, 0x34 ; 52");
	asm volatile("andi r24, 0xF7 ; 247");
	asm volatile("out 0x34, r24 ; 52");
	asm volatile("in r24, 0x21 ; 33");
	asm volatile("ori r24, 0x18 ; 24");
	asm volatile("out 0x21, r24 ; 33");
	asm volatile("out 0x21, r1 ; 33");
	//watchdog disabled
	
	uint8_t *ptr = (uint8_t*)0x45F;
	while(ptr > (uint8_t*)0x60)
		*ptr-- = 0x37;

	asm volatile("rjmp .+8");
	//continuing at __do_copy_data
}

uint16_t
stack_avail(void)
{
	uint8_t *ptr = (uint8_t*)0x45F;
	uint16_t c = 0;
	while(ptr > (uint8_t*)0x60) {
		if (*ptr == 0x37)
			c++;
		ptr--;
	}
	return c;
}
