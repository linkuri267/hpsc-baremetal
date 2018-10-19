#!/usr/bin/python3

import sys

NUM_BLOCKS = 2
NUM_INSTANCES = 32
IRQ_START = {0 : 72,   # LSIO_MAILBOX_IRQ_START
             1 : 136}  # HPPS_MAILBOX_IRQ_START

fout = open(sys.argv[1], "a")

for b in range(NUM_BLOCKS):
    for i in range(NUM_INSTANCES):
        fout.write("%u:mbox_%u_%u_rcv_isr\n" % (IRQ_START[b] + 2 * i, b, i))
        fout.write("%u:mbox_%u_%u_ack_isr\n" % (IRQ_START[b] + 2 * i + 1, b, i))
