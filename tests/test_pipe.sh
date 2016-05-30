#!/bin/bash

# stuff for tracking test case counts
FAILS=0
OKS=0
TOTAL=0

fail(){
	((ERRS++))
	((TOTAL++))
	echo "#$TOTAL: FAILED"
}

ok(){
	((OKS++))
	((TOTAL++))
	echo "#$TOTAL: PASSED"
}

# dlogctrl (tests 1-10)
dlogctrl                                             &> /dev/null && ok || fail # no args (display help)
dlogctrl -g                                          &> /dev/null && fail || ok # no -pk
dlogctrl -p                                          &> /dev/null && ok || fail # valid -p
dlogctrl -k                                          &> /dev/null && fail || ok # invalid -k
dlogctrl -p -k some_test_key                         &> /dev/null && fail || ok # both -pk
dlogctrl -k some_test_key                            &> /dev/null && fail || ok # no -gs with -k
dlogctrl -k some_test_key -g                         &> /dev/null && fail || ok # non-existent key
dlogctrl -k some_test_key -s                         &> /dev/null && fail || ok # no value
dlogctrl -k some_test_key -s some_value              &> /dev/null && ok || fail # valid -s
dlogctrl -k some_test_key -g                         &> /dev/null && ok || fail # valid -g

# tests 11-18
dlogctrl -k some_test_key -g -s some_other_value     &> /dev/null && fail || ok # both -gs
dlogctrl -k some_test_key -g -c                      &> /dev/null && fail || ok # both -gc
dlogctrl -k some_test_key -c -s some_other_value     &> /dev/null && fail || ok # both -cs
dlogctrl -k some_test_key -g | grep some_value       &> /dev/null && ok || fail # checks whether the value is what we passed
dlogctrl -k some_test_key -g | grep some_other_value &> /dev/null && fail || ok # ditto
dlogctrl -k some_test_key -c                         &> /dev/null && ok || fail # clear the test key
dlogctrl -k some_test_key -c                         &> /dev/null && fail || ok # clear the now non-existent key
dlogctrl -k some_test_key -g                         &> /dev/null && fail || ok # get the now non-existent key

# Start the daemon, add some logs
dlog_logger -b 99 -t 600 &
LOGGER=$!
test_libdlog 100

# 19-22: test -d and -t
dlogutil -d &> /dev/null && ok || fail
if [ $(dlogutil -t  20 | wc -l) -eq  20 ]; then ok; else fail; fi # less logs than currently in buffer
if [ $(dlogutil -t 200 | wc -l) -eq 100 ]; then ok; else fail; fi # more
if [ $(dlogutil -d     | wc -l) -eq 100 ]; then ok; else fail; fi # exactly

# 23-24: test -b
dlogutil -b nonexistent_buffer &> /dev/null && fail || ok
if [ $(dlogutil -d -b apps | wc -l) -eq 0 ]; then ok; else fail; fi # the logs should be in another buffer

# 25-26: test -c
dlogutil -c && ok || fail
test_libdlog 10
if [ $(dlogutil -d | wc -l) -eq 10 ]; then ok; else fail; fi # should be 10, not 110

# 27: test filters
if [ $(dlogutil -d *:E | wc -l) -eq 5 ]; then ok; else fail; fi # half of current logs (test_libdlog alternates between error and info log levels)

# 28: test -s
if [ $(dlogutil -s -d | wc -l) -eq 0 ]; then ok; else fail; fi

# 29: test -g
dlogutil -g &> /dev/null && ok || fail

# 30-38: test -f, -r and -n
mkdir /tmp/dlog_tests
dlogutil -f /tmp/dlog_tests/dlog_test_file -d &> /dev/null && ok || fail
dlogutil -f /tmp/dlog_tests/dlog_rotating_file -r 12 -n 3 && ok || fail # 3 files at 12 KB each
test_libdlog 100000
if [ -e /tmp/dlog_tests/dlog_test_file ]; then ok; else fail; fi
if [ -e /tmp/dlog_tests/dlog_rotating_file.1 ]; then ok; else fail; fi
if [ -e /tmp/dlog_tests/dlog_rotating_file.2 ]; then ok; else fail; fi
if [ -e /tmp/dlog_tests/dlog_rotating_file.3 ]; then ok; else fail; fi
if [ -e /tmp/dlog_tests/dlog_rotating_file.4 ]; then fail; else ok; fi
if [ $(wc -l < /tmp/dlog_tests/dlog_test_file) -eq 10 ]; then ok; else fail; fi
if [ $(du /tmp/dlog_tests/dlog_rotating_file.3 | sed 's/\t\/tmp\/dlog_tests\/dlog_rotating_file\.3//g') -eq 16 ]; then ok; else fail; fi # the actual size is one sector more (so 12 -> 16) because the limit is checked after reaching it, not before

# Test -v
# TODO

# 39: test library
dlogutil -f /tmp/dlog_tests/dlog_mt_test
MT_TEST=$!
test_libdlog && ok || fail

# 40: test multithreading
sleep 1
if [ $(grep "Multithreading test 9999" < /tmp/dlog_tests/dlog_mt_test | wc -l) -eq 50 ]; then ok; else fail; fi

# show results and clean up

echo "$OKS / $TOTAL tests passed"
echo "$FAILS / $TOTAL tests failed"

kill $LOGGER
rm -rf /tmp/dlog_tests

