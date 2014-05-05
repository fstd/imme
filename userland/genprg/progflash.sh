#!/bin/sh

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

pagesz=$((${2}*${3}))

./writexdata.sh -a 0xf000 -n $pagesz
./mkprogroutine.sh $1 $2 $3 $4 | ./writexdata.sh $((0xf000+${pagesz}))

echo "RUNINSTR -d 0x75 0xC7 0x51"
./setpc.sh $((0xf000+${pagesz}))
echo "RESUME"
echo "WAITHALT"
