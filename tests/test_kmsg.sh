#!/bin/bash

echo "Testing dlogctrl..."
dlogctrl                                             &> /dev/null && echo "01 OK"  || echo "01 ERR" # no args (display help)
dlogctrl -g                                          &> /dev/null && echo "02 ERR" || echo "02 OK"  # no -pk
dlogctrl -p                                          &> /dev/null && echo "03 OK"  || echo "03 ERR" # valid -p
dlogctrl -k                                          &> /dev/null && echo "04 ERR" || echo "04 OK"  # invalid -k
dlogctrl -p -k some_test_key                         &> /dev/null && echo "05 ERR" || echo "05 OK"  # both -pk
dlogctrl -k some_test_key                            &> /dev/null && echo "06 ERR" || echo "06 OK"  # no -gs with -k
dlogctrl -k some_test_key -g                         &> /dev/null && echo "07 ERR" || echo "07 OK"  # non-existent key
dlogctrl -k some_test_key -s                         &> /dev/null && echo "08 ERR" || echo "08 OK"  # no value
dlogctrl -k some_test_key -s some_value              &> /dev/null && echo "09 OK"  || echo "09 ERR" # valid -s
dlogctrl -k some_test_key -g                         &> /dev/null && echo "10 OK"  || echo "10 ERR" # valid -g
dlogctrl -k some_test_key -g -s some_other_value     &> /dev/null && echo "11 ERR" || echo "11 OK"  # both -gs
dlogctrl -k some_test_key -g -c                      &> /dev/null && echo "12 ERR" || echo "12 OK"  # both -gc
dlogctrl -k some_test_key -c -s some_other_value     &> /dev/null && echo "13 ERR" || echo "13 OK"  # both -cs
dlogctrl -k some_test_key -g | grep some_value       &> /dev/null && echo "14 OK"  || echo "14 ERR" # checks whether the value is what we passed
dlogctrl -k some_test_key -g | grep some_other_value &> /dev/null && echo "15 ERR" || echo "15 OK"  # ditto
dlogctrl -k some_test_key -c                         &> /dev/null && echo "16 OK"  || echo "16 ERR" # clear the test key
dlogctrl -k some_test_key -c                         &> /dev/null && echo "17 ERR" || echo "17 OK"  # clear the now non-existent key
dlogctrl -k some_test_key -g                         &> /dev/null && echo "18 ERR" || echo "18 OK"  # get the now non-existent key

echo "Testing dlogutil..."
# General usage
echo " - initializing logs"
dlogutil -b main -c &> /dev/null
test_libdlog 100
echo " - logs added"
dlogutil -d &> /dev/null && echo "1 OK" || echo "1 ERR"
# Test -d and -t (note that dlogutil -d always adds an extra line for kmsg)
if [ $(dlogutil -b main -t  20 | wc -l) -eq  21 ]; then echo "2 OK"; else echo "2 ERR"; fi # less logs than currently in buffer
if [ $(dlogutil -b main -t 200 | wc -l) -eq 101 ]; then echo "3 OK"; else echo "3 ERR"; fi # more
if [ $(dlogutil -b main -d     | wc -l) -eq 101 ]; then echo "4 OK"; else echo "4 ERR"; fi # exactly

# Test -b
dlogutil -b apps -c &> /dev/null # do something with another buffer
dlogutil -b nonexistent_buffer &> /dev/null && echo "5 ERR" || echo "5 OK"
if [ $(dlogutil -d -b apps | wc -l) -eq 1 ]; then echo "6 OK"; else echo "6 ERR"; fi # check if really cleared

# Test -c
dlogutil -c -b main && echo "7 OK" || echo "7 ERR"
test_libdlog 10
if [ $(dlogutil -b main -d | wc -l) -eq 11 ]; then echo "8 OK"; else echo "8 ERR"; fi # should be 10, not 110

# Test filters
if [ $(dlogutil -b main -d *:E | wc -l) -eq 6 ]; then echo "9 OK"; else echo "9 ERR"; fi # half of current logs (test_libdlog alternates between error and info log levels)

# Test -s
if [ $(dlogutil -b main -s -d | wc -l) -eq 1 ]; then echo "10 OK"; else echo "10 ERR"; fi

# Test -g
dlogutil -b main -g &> /dev/null && echo "11 OK" || echo "11 ERR"

# Test -f
mkdir /tmp/dlog_tests
dlogutil -b main -f /tmp/dlog_tests/dlog_test_file -d &> /dev/null && echo "12 OK" || echo "12 ERR"

echo "Testing libdlog..."
dlogutil -f /tmp/dlog_tests/dlog_mt_test &
MT_TEST=$!
test_libdlog && echo "Lib OK" || echo "Lib ERR"
sleep 1
kill $MT_TEST
if [ $(grep "Multithreading test 9999" < /tmp/dlog_tests/dlog_mt_test | wc -l) -eq 50 ]; then echo "MT OK"; else echo "MT ERR"; fi

echo "Cleaning up..."
rm -rf /tmp/dlog_tests

