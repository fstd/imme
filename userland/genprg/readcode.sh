#!/bin/sh

unset what
unset addr
unset num

set -e
#set -x

opts="a:n:CXbh"

usage() {
	echo "usage: $0 [$opts]" >&2
	    exit 1
}

addr=0
binary=

while getopts "$opts" i; do
	case $i in
	a) addr="$OPTARG" ;;
	b) binary="-b" ;;
	n) num="$OPTARG" ;;
	C) what="CODE" ;;
	X) what="XDATA" ;;
	h|\?) usage ;;
	esac
done

if [ -z "$what" ]; then
	echo "one of -C or -X is required" >&2
	usage
fi

if [ -z "$num" ]; then
	echo "no -n specified" >&2
	usage
fi

addrhi="0x$(printf '%02x' $((${addr}/256)))"
addrlo="0x$(printf '%02x' $((${addr}%256)))"

echo "RUNINSTR -d 0x90 $addrhi $addrlo"

while [ "$num" -gt 0 ]; do

	if [ "$what" = "CODE" ]; then
		echo 'RUNINSTR -d 0xE4'
		echo "RUNINSTR $binary 0x93"
		echo 'RUNINSTR -d 0xA3'
	else
		echo "RUNINSTR $binary 0xE0"
		echo 'RUNINSTR -d 0xA3'
	fi

	num=$(($num-1))

done
