#!/bin/sh

ROOT=/data/local/root

adb -d push root $ROOT
adb -d shell chmod 755 $ROOT
adb -d shell $ROOT
adb -d shell ls /data/local/tmp
adb -d shell cat /data/local/tmp/loading

adb -d remount
adb -d push /assets/su /system/bin/su
adb -d shell "chmod 6755 /system/bin/su"
adb -d shell "ln -s /system/bin/su /system/xbin/su 2>/dev/null"
adb -d push /assets/busybox /system/xbin/busybox
adb -d shell "chmod 755 /system/xbin/busybox"
adb -d shell "/system/xbin/busybox --install /system/xbin"
adb -d push /assets/Superuser.apk /system/app/Superuser.apk
