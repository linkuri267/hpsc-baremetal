#include <stdint.h>

#include "printf.h"
#include "hwinfo.h"
#include "dram-map.h"

#include "test.h"

int test_rt_mmu()
{
    // In this test, TRCH accesses same location as RTPS but via a different virtual addr.
    // RTPS should read 0xc0000000 and find 0xbeeff00d, not 0xf00dbeef.

    // Mappings are setup in trch/mmus.c since no easy way to keep this test
    // truly standalone, since the mappings for this test and for the MMU
    // usecase need to be active at the same time (since RTPS needs to
    // excercise these mappings when it boots).

    volatile uint32_t *addr = (volatile uint32_t *)RT_MMU_TEST_DATA_HI_1_WIN_ADDR;
    uint32_t val = 0xbeeff00d;
    printf("%p <- %08x\r\n", addr, val);
    *addr = val;

    addr = (volatile uint32_t *)RT_MMU_TEST_DATA_HI_0_WIN_ADDR;
    val = 0xf00dbeef;
    printf("%p <- %08x\r\n", addr, val);
    *addr = val;

    return 0;
}
