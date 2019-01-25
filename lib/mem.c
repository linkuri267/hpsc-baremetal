#include <stdint.h>

#include "mem.h"

void bzero(void *p, int sz)
{
    // assume p is word-aligned
    uint32_t *wp = p;
    while (sz >= sizeof(uint32_t)) {
        *wp++ = 0;
        sz -= sizeof(uint32_t);
    }
    uint8_t *bp = (uint8_t *)wp;
    while (sz-- >= 0)
        *bp++ = 0;
}

void *mem_set(void *s, int c, unsigned n)
{
    uint8_t *bs = s;
    while (n--)
        *bs++ = (unsigned char) c;
    return s;
}

void *mem_cpy(void *dest, void *src, unsigned n)
{
    // assume dest and src are word-aligned
    uint32_t *wd;
    uint32_t *ws;
    uint8_t *bd;
    uint8_t *bs;
    for (wd = dest, ws = src; n >= sizeof(*wd); n -= sizeof(*wd))
        *wd++ = *ws++;
    for (bd = (uint8_t *) wd, bs = (uint8_t *) ws; n > 0; n--)
        *bd++ = *bs++;
    return dest;
}
