#!/bin/sh

ROOT=/data/local/root

adb -d push root $ROOT
adb -d shell chmod 755 $ROOT
adb -d shell $ROOT
adb -d shell ls /data/local/tmp
adb -d shell cat /data/local/tmp/loading

