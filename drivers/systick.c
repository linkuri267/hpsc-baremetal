#define DEBUG 0

#include "hwinfo.h"
#include "regops.h"
#include "printf.h"
#include "panic.h"

#include "systick.h"

#define REG__STCSR              0x010
#define REG__STRVR              0x014
#define REG__STCVR              0x018
#define REG__STCR               0x01c

#define REG__STCSR__ENABLE      (1 <<  0)
#define REG__STCSR__TICKINT     (1 <<  1)
#define REG__STCSR__CLKSOURCE   (1 <<  2)
#define REG__STCSR__COUNTFLAG   (1 << 16)

struct systick {
    systick_cb_t *cb;
    void *cb_arg;
};

static struct systick systick;

void systick_config(uint32_t interval, systick_cb_t *cb, void *cb_arg)
{
    printf("SYSTICK: config: interval %u\r\n", interval);
    systick.cb = cb;
    systick.cb_arg = cb_arg;
    REGB_WRITE32(TRCH_SCS_BASE, REG__STRVR, interval);
    REGB_WRITE32(TRCH_SCS_BASE, REG__STCVR, 0);
    REGB_WRITE32(TRCH_SCS_BASE, REG__STCSR, REG__STCSR__TICKINT);
}

void systick_clear()
{
    printf("SYSTICK: clear\r\n");
    REGB_WRITE32(TRCH_SCS_BASE, REG__STCVR, 0);
}

uint32_t systick_count()
{
    return REGB_READ32(TRCH_SCS_BASE, REG__STCVR);
}

void systick_enable()
{
    printf("SYSTICK: enable\r\n");
    REGB_SET32(TRCH_SCS_BASE, REG__STCSR, REG__STCSR__ENABLE);
}

void systick_disable()
{
    printf("SYSTICK: disable\r\n");
    REGB_CLEAR32(TRCH_SCS_BASE, REG__STCSR, REG__STCSR__ENABLE);
}

void systick_isr()
{
    DPRINTF("SYSTICK: ISR\r\n");
    if (systick.cb)
        systick.cb(systick.cb_arg);
}
