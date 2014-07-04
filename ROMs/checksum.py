#!/usr/bin/python

"""
Checks that the sum of all bytes passed to the program modulo 0xff
is zero. Used to ensure that our ROM dumps are correct.

Note that each ROM checksum starts with the accumulator set to the
number of the ROM (1-4 inclusive).
"""

import sys
import argparse

def rlc(i):
    "Rotate 8-bit integer i left one bit"
    return ((i << 1)&0xff) | ((i >> 7)&0x01)

p = argparse.ArgumentParser()
p.add_argument('-r','--rn',help='ROM number',default=0)
args=p.parse_args()
rn = args.rn

# initialize sum with rom number
s = int(rn)
c = 0
while True:
    n = sys.stdin.read(1)
    if len(n) != 1:
        break
    s = rlc(s)
    s = s ^ ord(n[0])
    c += 1


s = s % 0xff

if s == 0:
    print("Checksum OK")
    sys.exit(0)
else:
    print("Checksum {0:x} (expected 0)".format(s))
    sys.exit(1)
