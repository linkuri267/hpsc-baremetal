#!/usr/bin/python

import argparse

parser = argparse.ArgumentParser(
    description="Generate assembly source for vector table")
parser.add_argument('--internal-irqs', type=int, default=16,
    help='Number internal IRQs')
parser.add_argument('--external-irqs', type=int, default=240,
    help='Number external IRQs')
parser.add_argument('out',
    help='Output file with generated assembly source')
args = parser.parse_args()

f = open(args.out, "w")

f.write(
"""
.cpu cortex-m4
.thumb

.thumb_func
.global _start
_start:
stacktop: .word 0x20001000
.word reset
"""
)

for i in range(0, args.internal_irqs - 1 - 1): # - unused, - reset
    f.write(".word hang\n")

f.write("\n")

for i in range(0, args.external_irqs):
    f.write(".word irq%u\n" % i)

f.write(
"""
.thumb_func
reset:
    bl notmain
    b hang
    b hang

.thumb_func
hang:   b .
    b hang
"""
+ "\n");


for i in range(0, args.external_irqs):
    f.write((".thumb_func\n" + \
             "irq%u:\n" + \
             "    bl irq%u\n" + \
             "    b hang\n" + \
             "    b hang\n\n") % (i, i))
