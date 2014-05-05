#!/bin/sh

set -e
#set -x

opts="h"

usage() {
	echo "usage: $0 [$opts]" >&2
	    exit 1
}

addr=0

while getopts "$opts" i; do
	case $i in
	h|\?) usage ;;
	esac
done

#
#DEBUG_INSTR(IN: 0x75, 0xC6, 0x00);                                     MOV CLKCON, #00H; 
#do { 
#DEBUG_INSTR(IN: 0xE5, 0xBE, OUT: sleepReg);                              MOV A, SLEEP; (sleepReg = A) 
#} while (!(sleepReg & 0x40)); 

echo "RUNINSTR -d 0x75 0xC6 0x00"
echo "DELAY 2000"
