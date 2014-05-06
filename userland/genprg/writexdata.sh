#!/bin/sh

unset addr
unset num

set -e
#set -x

opts="a:n:h"

usage() {
	echo "usage: $0 [$opts]" >&2
	    exit 1
}

addr=0

while getopts "$opts" i; do
	case $i in
	a) addr="$OPTARG" ;;
	n) num="$OPTARG" ;;
	h|\?) usage ;;
	esac
done

if [ -z "$num" ]; then
	num=-1
fi

addrhi="0x$(printf '%02x' $((${addr}/256)))"
addrlo="0x$(printf '%02x' $((${addr}%256)))"
num=$(($num)) #might be in hex

echo "RUNINSTR -d 0x90 $addrhi $addrlo"

hexdump -vC | sed -e 's/^[0-9a-f]* *//' -e 's/  / /' -e 's/   *.*$//' | grep -v '^ *$' | while read line; do
	for x in $line; do
		if [ $num -ne -1 ]; then
			if [ $num -eq 0 ]; then
				break;
			fi
			num=$((${num}-1))
		fi
		echo "RUNINSTR -d 0x74 0x${x}"
		echo 'RUNINSTR -d 0xF0'
		echo 'RUNINSTR -d 0xA3'
	done
done
