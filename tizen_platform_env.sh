#!/bin/sh

if [ -e "/opt/etc/dlog/.platformloggingon" ]; then
	export TIZEN_PLATFORMLOGGING_MODE=1
fi

# I'm not clear why "-a" doesn't work here
if [ "x$TIZEN_PLATFORMLOGGING_MODE" != "x" ] &&
	[ "$TIZEN_PLATFORMLOGGING_MODE" -eq "1" ]; then
	dlevel=$(cat /opt/etc/.dloglevel)
	if [ "$dlevel" -eq "0" ]; then
		export TIZEN_DLOG_LEVEL=0
	elif [ "$dlevel" -eq "1" ]; then
		export TIZEN_DLOG_LEVEL=1
	elif [ "$dlevel" -eq "2" ]; then
		export TIZEN_DLOG_LEVEL=2
	elif [ "$dlevel" -eq "3" ]; then
		export TIZEN_DLOG_LEVEL=3
	elif [ "$dlevel" -eq "4" ]; then
		export TIZEN_DLOG_LEVEL=4
	elif [ "$dlevel" -eq "5" ]; then
		export TIZEN_DLOG_LEVEL=5
	elif [ "$dlevel" -eq "6" ]; then
		export TIZEN_DLOG_LEVEL=6
	elif [ "$dlevel" -eq "7" ]; then
		export TIZEN_DLOG_LEVEL=7
	elif [ "$dlevel" -eq "8" ]; then
		export TIZEN_DLOG_LEVEL=8
	else
		export TIZEN_DLOG_LEVEL=8
	fi
fi
