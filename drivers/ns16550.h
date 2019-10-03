/*
 * NS16550 Serial Port
 * originally from linux source (arch/powerpc/boot/ns16550.h)
 *
 * Cleanup and unification
 * (C) 2009 by Detlev Zundel, DENX Software Engineering GmbH
 *
 * modified slightly to
 * have addresses as offsets from CONFIG_SYS_ISA_BASE
 * added a few more definitions
 * added prototypes for ns16550.c
 * reduced no of com ports to 2
 * modifications (c) Rob Taylor, Flying Pig Systems. 2000.
 *
 * added support for port on 64-bit bus
 * by Richard Danter (richard.danter@windriver.com), (C) 2005 Wind River Systems
 */

#include <stdint.h>

int ns16550_startup(uintptr_t base, int clock, int baudrate);
void ns16550_putchar(char ch);
