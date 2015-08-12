#!/bin/sh

ARGS=`getopt -o "cdt:gsf:r:n:v:b:" -- "$@"`
[ $? -ne 0 ] && exit 1

eval set -- "$ARGS"

OPT_C=0
OPT_G=0

while [ "$1" != "--" ] ; do
	case "$1" in
	-c)
		OPT_C=1
		;;
	-g)
		OPT_G=1
		;;
	-s|-d)
		;;
	-f|-r|-v|-b|-t)
		shift
		;;
	*)
		echo "Invalid argument $1"
		exit 1
		;;
	esac

	shift
done

if [ $OPT_G -eq 1 ]; then
	echo 1024
	exit 0
fi

if [ $OPT_C -eq 1 ]; then
	exit 0
fi

journalctl -f
