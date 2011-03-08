#!/bin/sh

#ROOT=/data/local/root
ROOT=/data/local/rlimit

adb -d shell ps | grep adbd | awk ' { print "pid: ", $2 }'
adb -d push $(basename $ROOT) $ROOT >/dev/null 2>&1
adb -d shell chmod 755 $ROOT
adb -d shell $ROOT
adb -d shell ps | grep adbd

