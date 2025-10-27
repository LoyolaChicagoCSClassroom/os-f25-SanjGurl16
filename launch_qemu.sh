#!/bin/bash

# This function kills qemu when we're finished debugging
cleanup() {
  killall qemu-system-i386
}

trap cleanup EXIT

# Start QEMU in a detached screen session with monitor support
screen -S qemu -d -m qemu-system-i386 \
    -S -s \
    -hda rootfs.img \
    -monitor stdio

screen -S qemu -d -m qemu-system-i386 -S -s -hda rootfs.img

TERM=xterm gdb-multiarch -x gdb_os.txt


