#!/bin/sh

if [ -z $1 ]
then
    # We print $0 so the script name will always be correct
    echo "Usage: ${0} [assembly file]"
else
    # arm-none-eabi-as -march="armv8-a" -mcpu="cortex-a57" -EL $1 -o $2
    zig build-obj -target aarch64-freestanding-none -cflags -Qunused-arguments -- $1
fi
