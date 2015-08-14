#!/bin/sh

gcc -c IC.c -I../../
gcc -c sampleIM.c -I../../
gcc -g -O2 -o sampleIM sampleIM.o IC.o ../../.libs/libIMdkit.a -L/usr/lib -L/usr/X11R6/lib -lX11
