#!/bin/sh

set -e
#set -x

opts="h"

usage() {
	echo "usage: $0 [$opts] <addr (16bit)>" >&2
	    exit 1
}

while getopts "$opts" i; do
	case $i in
	h|\?) usage ;;
	esac
done

if [ "$#" -ne 1 ]; then
	usage
fi

addrhi="0x$(printf '%02x' $((${1}/256)))"
addrlo="0x$(printf '%02x' $((${1}%256)))"

echo "RUNINSTR -d 0x02 $addrhi $addrlo"
