#!/bin/sh
mv /var/log/dlog /var/log/dlog.old
mv /var/log/dlog.1 /var/log/dlog.1.old
/usr/bin/dlogutil -r 128 -n 2 -f /var/log/dlog -v time *:W &
