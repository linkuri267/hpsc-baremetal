#define DEBUG 0

#include <stdbool.h>
#include <stdint.h>

#include "arm.h"
#include "board.h"
#include "console.h"
#include "hwinfo.h"
#include "debug.h"
#include "smc.h"
#include "sha256.h"
#include "ecc.h"

#include "ns16550.h"

#define BL1_COPIES	0x3
#define VECTOR_TABLE_SIZE 0x400
#define STACK_POINTER	0xffffc

#define CONFIG_BLOB_COPIES 0x3
#define SHA256_CHECKSUM_SIZE 32

extern void relocate_code (uint32_t);
extern void clean_and_jump (uint32_t, uint32_t);

#ifdef DEBUG_BL0 
#define DPRINTF(fmt, ...) \
        printf(fmt, ## __VA_ARGS__)
#else
#define DPRINTF(fmt, ...)
#endif

/* Failover: For initial draft */
#define NUM_FAILOVER_MEM_RANKS 2
#define BLOB_BASE 0x0
#define BLOB_OFFSET	0x100000
#define NUM_BLOB_COPIES	0x2
uint32_t blob_offsets[NUM_BLOB_COPIES] = {BLOB_BASE, BLOB_BASE + BLOB_OFFSET};

typedef struct {
    uint32_t bl1_size;
    uint32_t bl1_offset;     /* where copies of bl1 image is stored in NVRAM */
    uint32_t bl1_load_addr; /* where bl1 image is loaded */
    uint32_t bl1_entry_offset; /* entry offset from the start of the bl1 image */
    unsigned char checksum[SHA256_CHECKSUM_SIZE]; /* checksum of the bl1 image */
    unsigned char ecc[ECC_512_SIZE]; /* ecc of the bl0_blob except ecc */
} bl0_blob;

static inline int diff_checksum(unsigned char * a, unsigned char * b) {
    int i;
    for (i = 0; i < SHA256_CHECKSUM_SIZE; i++) {
        if (a[i] != b[i]) return 1;
    }
    printf("checksum success\r\n");
    return 0;
}

/* copied and modified from lib/memfs.c */
static int load_memcpy(uint32_t *mem_addr, uint32_t *load_addr, unsigned size)
{
    unsigned w, b;

    uint32_t nwords = size / sizeof(uint32_t);
    uint32_t rem_bytes = (size) % sizeof(uint32_t);

    for (w = 0; w < nwords; w++) {
        * load_addr = * mem_addr;
        load_addr++;
        mem_addr++;
    }
    uint8_t *load_addr_8 = (uint8_t *) load_addr;
    uint8_t *mem_addr_8 = (uint8_t *) mem_addr;
    for (b = 0; b < rem_bytes; b++) {
        * load_addr_8 = * mem_addr_8;
        load_addr_8++;
        mem_addr_8++;
    }
    return 0;
}

/* 
   Boot Select Code:
       bs[0]: 0: SRAM; 1: SPI
       bs[1]: 0: rank0; 1: rank1
       bs[2]: 0: NOR; 1: MRAM
       bs[3]: 0: offset1; 1: offset2
       bs[4]: 0: failover disabled; 1: failover enabled
       bs[5]: 0: parity (odd); 1: parity (even)
*/
#define BS_INTERFACE_SHIFT 0
#define BS_RANK_SHIFT 1
#define BS_SRAM_TYPE_SHIFT 2
#define BS_CONFIGURATION_OFFSET_SHIFT 3
#define BS_FAILOVER_SHIFT 4
#define BS_PARITY_SHIFT 5

#define BS_SIZE 6
#define BS_MASK(a, b) (a & (1 << b))
#define BS_SRAM_INTERFACE 		(0 << BS_INTERFACE_SHIFT)
#define BS_SPI_INTERFACE 		(1 << BS_INTERFACE_SHIFT)
#define BS_SRAM_NOR 			(0 << BS_SRAM_TYPE_SHIFT)
#define BS_SRAM_MRAM 			(1 << BS_SRAM_TYPE_SHIFT)
#define BS_MEM_RANK0 			(0 << BS_RANK_SHIFT)
#define BS_MEM_RANK1 			(1 << BS_RANK_SHIFT)
#define BS_CONFIGURATION_OFFSET0 	(0 << BS_CONFIGURATION_OFFSET_SHIFT)
#define BS_CONFIGURATION_OFFSET1 	(1 << BS_CONFIGURATION_OFFSET_SHIFT)
#define BS_FAILOVER_DISABLE		(0 << BS_FAILOVER_SHIFT)
#define BS_FAILOVER_ENABLE		(1 << BS_FAILOVER_SHIFT)
#define BS_PARITY_ODD 			(0 << BS_PARITY_SHIFT)
#define BS_PARITY_EVEN			(1 << BS_PARITY_SHIFT)

#define FAILOVER_ENABLED(a)		(BS_MASK(a, BS_FAILOVER_SHIFT) > 0)
int parity_check(uint8_t data)
{
    int i, j, count;
    for (i = count = 0, j = 1; i < BS_SIZE; i++) {
            count += (data & j) >> i;
            j <<= 1;
    }
    if ((count & 1) == 0) return -1;
    return 0;
}

void invalidate_icache()
{
    uint32_t *iciallu = (uint32_t *) 0xE000EF50;

    *iciallu = 0x0;

    asm("DSB");
    asm("ISB");
}

int main ( void )
{
    int mem_rank;
    uint8_t * load_addr, * mem_addr, * mem_base_addr;
    uint8_t boot_select;

    /* TODO: interrupt must be disabled here if it was enabled */
    /* we may not use console, for debugging purpose only */
    console_init();
    printf("ENTER PRIVELEGED MODE: svc #0\r\n");
    asm("svc #0");

#ifdef HW_EMULATION
    /* TODO: read Boot Select Code */
    boot_select = readb(BOOT_SLECT_CODE_ADDR);
#else
    boot_select = BS_SRAM_INTERFACE | BS_SRAM_NOR | BS_MEM_RANK0 | BS_CONFIGURATION_OFFSET0 | BS_FAILOVER_DISABLE | BS_PARITY_EVEN;
#endif
    if (parity_check(boot_select) < 0) {
        printf("Boot selector code parity check failed\r\n");
        return 0;
    }
    /* 1. select the interface */
    if (BS_MASK(boot_select, BS_INTERFACE_SHIFT) == BS_SRAM_INTERFACE) {
        /* SMC */
        printf("Initialize SMC\r\n");
        /* 2. Rank (0/1) */
        mem_rank = BS_MASK(boot_select, BS_RANK_SHIFT) >> BS_RANK_SHIFT;
        /* 3. Memory type (NOR or MRAM) */
        if (BS_MASK(boot_select, BS_SRAM_TYPE_SHIFT)) /* MRAM: +2 */
            mem_rank += 2;
        smc_boot_init((uintptr_t)SMC_CSR_LSIO_SRAM_BASE, mem_rank, &trch_smc_boot_init, FAILOVER_ENABLED(boot_select));

        /* 4. base address depends on rank */
#ifdef HW_EMULATION
        mem_base_addr = (uint8_t *) smc_get_base_addr((uintptr_t)SMC_CSR_LSIO_SRAM_BASE, interface, mem_rank);
#else
        mem_base_addr = (uint8_t *) SMC_LSIO_SRAM_BASE;
#endif
        printf("interface(SRAM), rank(%d), base_addr(0x%x)\r\n", mem_rank, mem_base_addr);

        printf("Read Configuration blob\r\n");
    } else {
        /* TODO: SPI */
    }

    /* load configuration blob */
    bl0_blob config_blob;
    unsigned char * ptr1 = (unsigned char *)&config_blob;
    unsigned char * ptr2 = (unsigned char *)config_blob.ecc;
    unsigned int data_size = (unsigned long) ptr2 - (unsigned long) ptr1; 
    unsigned char ecc_calc[ECC_512_SIZE];
    int i, j;
    int mem_ranks_trial = 1;
    int mem_rank_used = mem_rank;

    if (FAILOVER_ENABLED(boot_select))
        mem_ranks_trial = NUM_FAILOVER_MEM_RANKS;
    for (i = 0; i < mem_ranks_trial; i++) {
        if (i > 0) 
            mem_rank_used = (mem_rank % 0x1 == 0 ? mem_rank + 1 : mem_rank - 1);
#ifdef HW_EMULATION
        mem_base_addr = (uint8_t *) smc_get_base_addr((uintptr_t)SMC_CSR_LSIO_SRAM_BASE, interface, mem_rank_used);
#else
        if (mem_rank_used > 0)
            printf("memory rank(%d) is ignored in software emulation\r\n", mem_rank_used);
        mem_base_addr = (uint8_t *) SMC_LSIO_SRAM_BASE;
#endif
        /* 
           fail-over within a memory rank. 
           two copies in a memory rank are assumed to exist.
         */
        for (j = 0; j < NUM_BLOB_COPIES; j++) {
            mem_addr = mem_base_addr + blob_offsets[j];
            printf("cp config_blob from 0x%x to %p\r\n", mem_addr, &config_blob);
            load_memcpy((uint32_t *)(mem_addr), (uint32_t *)&config_blob, sizeof(config_blob));
            calculate_ecc((unsigned char *)&config_blob, data_size, ecc_calc);
            if (correct_data((unsigned char *)&config_blob, config_blob.ecc, ecc_calc, data_size) < 0) 
                continue;

            /* 
                load BL1 image to temporary address (bl1_load_addr + bl1_entry_offset).
                This image is to be copied again.
                TODO: reduce the extra copy except the vector table.
             */
            load_addr = (uint8_t *) (config_blob.bl1_load_addr);
            load_addr += config_blob.bl1_entry_offset;
            mem_addr = mem_base_addr + config_blob.bl1_offset; /* + vtbl_size; */
            printf("Load BL1 image (0x%x) to (0x%x), size(0x%x)\r\n", mem_addr, load_addr, config_blob.bl1_size);
            load_memcpy((uint32_t *) mem_addr, (uint32_t *)load_addr, config_blob.bl1_size);
   
            /* checksum */   
            unsigned char output[SHA256_CHECKSUM_SIZE];
            mbedtls_sha256_ret((unsigned char *) load_addr, config_blob.bl1_size, output, false);
            if (diff_checksum(output, config_blob.checksum)) {
                printf("Checksum Failure\r\n");
                continue;
            }
            break;
        }
        if ((j == NUM_BLOB_COPIES) && (i == (mem_ranks_trial - 1))) {
            printf("Failed in ECC check: no other copy of config_blob in a memory rank (%d)\r\n", mem_rank_used);
            return (-1);
        }
    }

    /* move bl1 image to the actual bl1_load_addr */ 
    load_addr = (uint8_t *) config_blob.bl1_load_addr;
    mem_addr = (uint8_t *) config_blob.bl1_load_addr;
    mem_addr += config_blob.bl1_entry_offset;
    printf("Move BL1 image from (0x%x) to (0x%x), size(0x%x)\r\n", mem_addr, load_addr, config_blob.bl1_size);
    load_memcpy((uint32_t *) mem_addr, (uint32_t *)load_addr, config_blob.bl1_size);
   
 
    /* jump to new entry_offset of BL1 */
    printf("Jump to the bootloader (0x%x)\r\n", config_blob.bl1_entry_offset );
    // invalidate_icache();
    clean_and_jump(config_blob.bl1_entry_offset, STACK_POINTER);

    return 0;
}
