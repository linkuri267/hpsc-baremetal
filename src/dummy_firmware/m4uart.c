
void PUT32 ( unsigned int, unsigned int );
//#define UART0BASE 0x4000C000
#define UART0BASE 0xffd40000
#define APU_RESET_ADDR 0xfd1a0104
#define APU_RESET_VALUE 0x800000fe

int notmain ( void )
{
    unsigned int rx;
    for(rx=0;rx<8;rx++)
    {
        PUT32(UART0BASE+0x00,0x30+(rx&7));
    }
    PUT32(APU_RESET_ADDR, APU_RESET_VALUE);
    return(0);
}

int myirq0 (void) {
    PUT32(UART0BASE+0x00,0x90);
    return(0);
}

int myirq1 (void) {
    PUT32(UART0BASE+0x00,0x91);
    return(0);
}

int myirq2 (void) {
    PUT32(UART0BASE+0x00,0x92);
    return(0);
}

int myirq3 (void) {
    PUT32(UART0BASE+0x00,0x93);
    return(0);
}

int myirq4 (void) {
    PUT32(UART0BASE+0x00,0x94);
    return(0);
}

int myirq5 (void) {
    PUT32(UART0BASE+0x00,0x95);
    return(0);
}

int myirq6 (void) {
    PUT32(UART0BASE+0x00,0x96);
    return(0);
}

int myirq7 (void) {
    PUT32(UART0BASE+0x00,0x97);
    return(0);
}

int myirq8 (void) {
    PUT32(UART0BASE+0x00,0x98);
    return(0);
}

int myirq9 (void) {
    PUT32(UART0BASE+0x00,0x99);
    return(0);
}

int myirq10 (void) {
    PUT32(UART0BASE+0x00,0x9a);
    return(0);
}

int myirq11 (void) {
    PUT32(UART0BASE+0x00,0x9b);
    return(0);
}

int myirq12 (void) {
    PUT32(UART0BASE+0x00,0x9c);
    return(0);
}

int myirq13 (void) {
    PUT32(UART0BASE+0x00,0x9d);
    return(0);
}

int myirq14 (void) {
    PUT32(UART0BASE+0x00,0x9e);
    return(0);
}

int myirq15 (void) {
    PUT32(UART0BASE+0x00,0x9f);
    return(0);
}

int myirq16 (void) {
    PUT32(UART0BASE+0x00,0xa0);
    return(0);
}

int myirq17 (void) {
    PUT32(UART0BASE+0x00,0xa1);
    return(0);
}

int myirq18 (void) {
    PUT32(UART0BASE+0x00,0xa2);
    return(0);
}

int myirq19 (void) {
    PUT32(UART0BASE+0x00,0xa3);
    return(0);
}

