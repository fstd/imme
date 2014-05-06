#!/bin/sh

set -e
#set -x
opts="he"
usage() {
	echo "usage: $0 [-$opts] <pageno (0-31)>" >&2
	echo "  -e: erase page before writing" >&2
	echo "input must be a complete page (i.e. 1024 bytes)" >&2
	    exit 1
}

sramstart=0xf000
wordsz=2
wordsperpage=512

pagesz=$((${wordsz}*${wordsperpage}))
erase=0

while getopts "$opts" i; do
	case $i in
	e) erase=1 ;;
	h|\\?) usage ;;
	esac
done

shift $((${OPTIND}-1))

if [ "$#" -ne 1 ]; then
	usage
fi

pageaddr=$((${1}*${pagesz}))

./writexdata.sh -a $sramstart -n $pagesz
./mkprogroutine.sh $pageaddr $wordsz $wordsperpage $erase | ./writexdata.sh -a $((${sramstart}+${pagesz}))

echo "RUNINSTR -d 0x75 0xC7 0x51"
./setpc.sh $((${sramstart}+${pagesz}))
echo "RESUME"
echo "WAITHALT"
