#!/usr/bin/python

import sys
from elftools.elf.elffile import ELFFile

files = sys.argv[1:]

SECTIONS = [
    '.text',
    '.bss',
    '.data',
    '.rodata',
]

for fname in files:
    f = open(fname, "rb")
    ef = ELFFile(f)
    print(fname)
    for s in ef.iter_sections():
        if s.name in SECTIONS:
            # data_size not yet available in pyelftools 0.22
            print("%10s: %4.3f KB" % (s.name, len(s.data()) / 1024))
    print
