#!/bin/sh

unset device
unset baud

set -e
#set -x

opts="hd:b:D"
usage() {
	echo "usage: $0 [-$opts] <blob>" >&2
	echo " -d, -D and -b are passed to immectl (see immectl -h)" >&2
	exit 1
}

sercommd=0

while getopts "$opts" i; do
	case $i in
	d) device="$OPTARG" ;;
	b) baud="$OPTARG" ;;
	D) sercommd=1 ;;
	h|\\?) usage ;;
	esac
done

shift $((${OPTIND}-1))

if [ "$#" -ne 1 ]; then
	usage
fi

immecmd="immectl"
if [ $sercommd -ne 0 ]; then
	immecmd="${immecmd} -D"
fi

if [ -n "$device" ]; then
	immecmd="${immecmd} -d $device"
fi

if [ -n "$baud" ]; then
	immecmd="${immecmd} -b $baud"
fi

flashsz=32768
pagesz=1024

if [ "$(uname)" == "Linux" ]; then
	sz=$(stat -c '%s' $1)
else
	sz=$(stat -f '%z' $1)
fi

if [ $sz -gt $flashsz ]; then
	echo "that wouldn't fit..." >&2
	exit 1
fi

pages=$(($sz/$pagesz))
remain=$(($sz%$pagesz))

if [ $remain -gt 0 ]; then
	pages=$(($pages+1))
fi

tmp=$(mktemp /tmp/progflash.XXXXXX)
vfy=$(mktemp /tmp/progflash.XXXXXX)

trap 'rm -f "$tmp" "$vfy"' EXIT


echo "resetting master" >&2
echo MSTRST | $immecmd
sleep .2
echo "reinit target" >&2
echo REINIT | $immecmd
sleep .2
echo "init clock" >&2
echo RUNINSTR -d 0x75 0xc6 0x00 | $immecmd
sleep .2
echo "waiting for proper status" >&2
while [ "$(echo STATUS | $immecmd)" != "0xb2" ]; do
	sleep .2
done
echo "erasing chip" >&2
echo CHIPERASE | $immecmd
sleep .2
echo "waiting for proper status" >&2
while [ "$(echo STATUS | $immecmd)" != "0xb2" ]; do
	sleep .2
done
echo "chip erase done" >&2


page=0
while [ $page -lt $pages ]; do
	head -c $pagesz </dev/zero | tr '\0' '\377' >$tmp
	dd if="$1" of="$tmp" bs=$pagesz skip=$page count=1 conv=notrunc 2>/dev/null

	echo "writing page $page" >&2
	./writeflashpage.sh $page <$tmp | $immecmd

	page=$(($page+1))
done
echo "finished writing, verifying..." >&2

head -c $(($pages*$pagesz)) </dev/zero | tr '\0' '\377' >$vfy
dd if="$1" of="$vfy" bs=$pagesz count=$pages conv=notrunc 2>/dev/null
./readcode.sh -X -a 0 -n $(($pages*$pagesz)) -b | $immecmd >$tmp 

if ! cmp $vfy $tmp; then
	echo "verification failed!" >&2
	exit 1
else
	echo "OK"
fi

