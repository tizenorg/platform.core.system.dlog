#!/bin/sh
source /etc/tizen-platform.conf
cp ${TZ_SYS_ETC}/platformlog.conf /tmp/.platformlog.conf
chmod 660 /tmp/.platformlog.conf
chown root:app_logging /tmp/.platformlog.conf
chsmack -a '_' -e 'none' /tmp/.platformlog.conf
/usr/bin/dlogutil -b system -r 5120 -n 1 -f /var/log/dlog_system -v threadtime *:I &
/usr/bin/dlogutil -b main -r 3072 -n 1 -f /var/log/dlog_main -v threadtime *:W &
