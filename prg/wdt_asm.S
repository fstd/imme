.text
.global wdt_off
.global wdt_on

wdt_off:
	wdr
	in r24, 0x34
	andi r24, 0xF7
	out 0x34, r24
	in r24, 0x21
	ori r24, 0x18
	out 0x21, r24
	out 0x21, r1
	ret

wdt_on:
	wdr
	in r24, 0x21
	ori r24, 0x18
	out 0x21, r24
	ldi r24, 0x08
	out 0x21, r24
	ret
