#!/bin/sh
if [ -z "$1" ]; then
	exit
else
	LOG_DST_DIR=$1
fi
#--------------------------------------
#    dump dlog logs /dev/log_main, log_system, log_radio
#--------------------------------------
DLOG_LOG_DIR=${LOG_DST_DIR}/dlog_log
mkdir -p "${DLOG_LOG_DIR}"
dlogutil -b radio -d -v time -f ${DLOG_LOG_DIR}/dump_dlog_radio.log -r 512 -n 1 &
dlogutil -b system -d -v time -f ${DLOG_LOG_DIR}/dump_dlog_system.log -r 1024 -n 1 &
dlogutil -b main -d -v time -f ${DLOG_LOG_DIR}/dump_dlog_main.log -r 4096 -n 1 &
