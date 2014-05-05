#!/bin/sh

#program-routine generator, backend to writeflashpage.sh

set -e
#set -x

opts="h"

usage() {
	echo "usage: $0 [$opts] <address> <flash_word_size> <words_per_flash_page> <erase_page (0 or 1)>" >&2
	    exit 1
}

addr=0

while getopts "$opts" i; do
	case $i in
	h|\\?) usage ;;
	esac
done

if [ "$#" -ne 4 ]; then
	usage
fi

printf "$(printf "\\\\%o\\\\%o\\\\%o" 0x75 0xAD $((((${1}/256)/${2})&0x7E)))"
printf "$(printf "\\\\%o\\\\%o\\\\%o" 0x75 0xAC 0x00)"

if [ "$4" -ne 0 ]; then
	printf "$(printf "\\\\%o\\\\%o\\\\%o" 0x75 0xAE 0x01)"

	printf "$(printf "\\\\%o\\\\%o" 0xE5 0xAE)"
	printf "$(printf "\\\\%o\\\\%o\\\\%o" 0x20 0xE7 0xFB)"
fi

printf "$(printf "\\\\%o\\\\%o\\\\%o" 0x90 0xF0 0x00)"

printf "$(printf "\\\\%o\\\\%o" 0x7F $((${3}/256)))"
printf "$(printf "\\\\%o\\\\%o" 0x7E $((${3}%256)))"
printf "$(printf "\\\\%o\\\\%o\\\\%o" 0x75 0xAE 0x02)"

printf "$(printf "\\\\%o\\\\%o" 0x7D ${2})"
printf "$(printf "\\\\%o" 0xE0)"
printf "$(printf "\\\\%o" 0xA3)"
printf "$(printf "\\\\%o\\\\%o" 0xF5 0xAF)"
printf "$(printf "\\\\%o\\\\%o" 0xDD 0xFA)"

printf "$(printf "\\\\%o\\\\%o" 0xE5 0xAE)"
printf "$(printf "\\\\%o\\\\%o\\\\%o" 0x20 0xE6 0xFB)"
printf "$(printf "\\\\%o\\\\%o" 0xDE 0xF1)"
printf "$(printf "\\\\%o\\\\%o" 0xDF 0xEF)"

printf "$(printf "\\\\%o" 0xA5)"
