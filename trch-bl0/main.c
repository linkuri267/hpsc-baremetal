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

/* For initial draft */
#define BLOB_ADDR0	0x0
#if 0
#define BLOB_ADDR1	0x1000000
#define BLOB_ADDR2	0x2000000
uint32_t blob_offsets[CONFIG_BLOB_COPIES] = {BLOB_ADDR0, BLOB_ADDR1, BLOB_ADDR2};
#else
uint32_t blob_offsets = BLOB_ADDR0;
#endif

typedef struct {
    uint32_t bl1_size;
    uint32_t bl1_start;  /* where copies of bl1 is stored in NVRAM */
    uint32_t bl1_load_addr;
    uint32_t bl1_entry_offset;
    unsigned char checksum[SHA256_CHECKSUM_SIZE];
    unsigned char ecc[ECC_512_SIZE];
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

int main ( void )
{
    uint8_t * load_addr, * mem_addr, * mem_base_addr;

    /* TODO: interrupt must be disabled here if it was enabled */
    /* we may not use console at BL0, for debugging purpose only */
    console_init();
    printf("ENTER PRIVELEGED MODE: svc #0\r\n");
    asm("svc #0");

    /* TODO: read Boot Select Code */
    /* TODO: select the interface */
    int interface = 1;	/* SMC */
    if (interface) {
        /* SMC */
        printf("Initialize SMC\r\n");
        smc_init(SMC_BASE, &trch_smc_mem_cfg);
        mem_base_addr = (uint8_t *) SMC_SRAM_BASE;
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

    load_memcpy((uint32_t *)(mem_base_addr), (uint32_t *)&config_blob, sizeof(bl0_blob));
    calculate_ecc((unsigned char *)&config_blob, data_size, ecc_calc);
    
    if (correct_data((unsigned char *)&config_blob, config_blob.ecc, ecc_calc, data_size) < 0) {
        /* TODO: implement fail-over scheme */
        printf("Failed in ECC check: no other copy of config_blob\r\n");
        return (-1);
    }

    /* load BL1 image to bl1_load_addr + bl1_entry_offset */
    load_addr = (uint8_t *) (config_blob.bl1_load_addr);
    load_addr += config_blob.bl1_entry_offset;
    mem_addr = mem_base_addr + config_blob.bl1_start; /* + vtbl_size; */
    printf("Load BL1 image (0x%x) to (0x%x), size(0x%x)\r\n", mem_addr, load_addr, config_blob.bl1_size);
    load_memcpy((uint32_t *) mem_addr, (uint32_t *)load_addr, config_blob.bl1_size);
   
    /* checksum */   
    unsigned char output[SHA256_CHECKSUM_SIZE];
    mbedtls_sha256_ret((unsigned char *) load_addr, config_blob.bl1_size, output, false);
    if (diff_checksum(output, config_blob.checksum)) {
        /* TODO: implement fail-over scheme */
        printf("Checksum Failure\r\n");
    }

    /* move to bl1_load_addr */ 
    load_addr = (uint8_t *) config_blob.bl1_load_addr;
    mem_addr = (uint8_t *) config_blob.bl1_load_addr;
    mem_addr += config_blob.bl1_entry_offset;
    printf("Move BL1 image from (0x%x) to (0x%x), size(0x%x)\r\n", mem_addr, load_addr, config_blob.bl1_size);
    load_memcpy((uint32_t *) mem_addr, (uint32_t *)load_addr, config_blob.bl1_size);
   
 
    /* jump to new entry_offset of BL1 */
    printf("Jump to the bootloader (0x%x)\r\n", config_blob.bl1_entry_offset );
    /* TODO: invalidate ICACHE */
    clean_and_jump(config_blob.bl1_entry_offset, STACK_POINTER);

    return 0;
}
