#!/bin/bash

echo "Testing dlogctrl..."
dlogctrl                                           &> /dev/null && echo "1 ERR"  || echo "1 OK"  # No arguments given
dlogctrl get                                       &> /dev/null && echo "2 OK"   || echo "2 ERR" # Get all keys
dlogctrl get some_test_key                         &> /dev/null && echo "3 ERR"  || echo "3 OK"  # Get a non-existent key
dlogctrl set some_test_key                         &> /dev/null && echo "4 ERR"  || echo "4 OK"  # Set, but no value given
dlogctrl set some_test_key some_value              &> /dev/null && echo "5 OK"   || echo "5 ERR" # Set to a value
dlogctrl get some_test_key                         &> /dev/null && echo "6 OK"   || echo "6 ERR" # Get again, now should exist
dlogctrl get some_test_key | grep some_value       &> /dev/null && echo "7 OK"   || echo "7 ERR" # Also checks whether the value is what we passed
dlogctrl get some_test_key | grep some_other_value &> /dev/null && echo "8 ERR"  || echo "8 OK"  # Ditto
dlogctrl clear some_test_key                       &> /dev/null && echo "9 OK"   || echo "9 ERR" # Clear the test key
dlogctrl clear some_test_key                       &> /dev/null && echo "10 ERR" || echo "10 OK" # Clear a now non-existent key
dlogctrl get some_test_key                         &> /dev/null && echo "11 ERR" || echo "11 OK" # Get a now non-existent key

echo "Testing dlogutil..."
# General usage
dlog_logger -b 99 -t 50 &
LOGGER=$!
test_libdlog 100
dlogutil -d &> /dev/null && echo "1 OK" || echo "1 ERR"
# Test -d and -t
if [ $(dlogutil -t  20 | wc -l) -eq  20 ]; then echo "2 OK"; else echo "2 ERR"; fi # less logs than currently in buffer
if [ $(dlogutil -t 200 | wc -l) -eq 100 ]; then echo "3 OK"; else echo "3 ERR"; fi # more
if [ $(dlogutil -d     | wc -l) -eq 100 ]; then echo "4 OK"; else echo "4 ERR"; fi # exactly

# Test -b
dlogutil -b nonexistent_buffer &> /dev/null && echo "5 ERR" || echo "5 OK"
if [ $(dlogutil -d -b apps | wc -l) -eq 0 ]; then echo "6 OK"; else echo "6 ERR"; fi # the logs should be in another buffer

# Test -c
dlogutil -c && echo "7 OK" || echo "7 ERR"
test_libdlog 10
if [ $(dlogutil -d | wc -l) -eq 10 ]; then echo "8 OK"; else echo "8 ERR"; fi # should be 10, not 110

# Test filters
if [ $(dlogutil -d *:E | wc -l) -eq 5 ]; then echo "9 OK"; else echo "9 ERR"; fi # half of current logs (test_libdlog alternates between error and info log levels)

# Test -s
if [ $(dlogutil -s -d | wc -l) -eq 0 ]; then echo "10 OK"; else echo "10 ERR"; fi

# Test -g
dlogutil -g &> /dev/null && echo "11 OK" || echo "11 ERR"

# Test -f, -r and -n
mkdir /tmp/dlog_tests
dlogutil -f /tmp/dlog_tests/dlog_test_file -d &> /dev/null && echo "12 OK" || echo "12 ERR"
dlogutil -f /tmp/dlog_tests/dlog_rotating_file -r 12 -n 3 && echo "13 OK" || echo "13 ERR" # 3 files at 12 KB each
test_libdlog 100000

# check file existence
if [ -e /tmp/dlog_tests/dlog_test_file ]; then echo "14 OK"; else echo "14 ERR"; fi
if [ -e /tmp/dlog_tests/dlog_rotating_file.1 ]; then echo "15 OK"; else echo "15 ERR"; fi
if [ -e /tmp/dlog_tests/dlog_rotating_file.2 ]; then echo "16 OK"; else echo "16 ERR"; fi
if [ -e /tmp/dlog_tests/dlog_rotating_file.3 ]; then echo "17 OK"; else echo "17 ERR"; fi
if [ -e /tmp/dlog_tests/dlog_rotating_file.4 ]; then echo "18 ERR"; else echo "18 OK"; fi

# check whether the content actually ended up in the files
if [ $(wc -l < /tmp/dlog_tests/dlog_test_file) -eq 10 ]; then echo "19 OK"; else echo "19 ERR"; fi
if [ $(du /tmp/dlog_tests/dlog_rotating_file.3 | sed 's/\t\/tmp\/dlog_tests\/dlog_rotating_file\.3//g') -eq 16 ]; then echo "20 OK"; else echo "20 ERR"; fi # the actual size is one sector more (so 12 -> 16) because the limit is checked after reaching it, not before

# Test -v
# TODO

echo "Testing libdlog..."
test_libdlog && echo "OK" || echo "ERR"

echo "Cleaning up..."
kill $LOGGER
rm -rf /tmp/dlog_tests

