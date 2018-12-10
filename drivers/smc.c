#include <stdint.h>

#include "printf.h"
#include "hwinfo.h"
#include "str.h"

#include "smc.h"

#define FILE_NAME_LENGTH 200

typedef struct {
    uint32_t valid;
    uint32_t offset;	/* offset in SRAM image */
    uint32_t size;
    uint32_t load_addr;		/* 32bit load address in DRAM at run-time */
    uint32_t load_addr_high;	/* high 32bit of 64 bit load address in DRAM at run-time */
    char  name[FILE_NAME_LENGTH];
} file_descriptor;

typedef struct {
    uint32_t low_mark_data;	/* low mark of data */
    uint32_t high_mark_fd;	/* high mark of file descriptors */
    uint32_t n_files;		/* number of files */
    uint32_t fsize;		/* sram file size */
} global_table;

int smc_sram_load(const char *fname)
{
    global_table gt;
    unsigned i, j;
    file_descriptor * fd_buf;
    unsigned char * sram_start_addr = (unsigned char *)SMC_SRAM_BASE;
    unsigned char * load_addr;
    unsigned char * sram_addr;
    unsigned char * ptr = (unsigned char *) &gt;
    uint32_t * load_addr_32;
    uint32_t * sram_addr_32;

    for(i = 0; i < sizeof(global_table); i++) {
        ptr[i] = * (sram_start_addr + i);
    }
    printf("SMC: SRAM: #files : %u, low_mark_data(0x%lx), high_mark_fd(0x%x)\r\n",
           gt.n_files, gt.low_mark_data, gt.high_mark_fd);

    for (i = 0; i < gt.n_files ; i++) {
        fd_buf = (file_descriptor *)(sram_start_addr + sizeof(gt) + sizeof(file_descriptor)*i);
        if (!fd_buf->valid)
            continue;
        if (!strcmp(fd_buf->name, fname))
            break;
    }
    if (i == gt.n_files) {
        printf("SMC: ERROR: file not found in SRAM: %s\r\n", fname);
        return 1;
    }

    uint32_t offset = fd_buf->offset;
    sram_addr = (unsigned char *) (sram_start_addr + offset);
    load_addr = (unsigned char *)fd_buf->load_addr;
    sram_addr_32 = (uint32_t *) (sram_start_addr + offset);
    load_addr_32 = (uint32_t *)fd_buf->load_addr;
    uint32_t iter = fd_buf->size / sizeof(uint32_t);
    uint32_t rem = fd_buf->size % sizeof(uint32_t);

    printf("SMC: loading file #%u: %s (sram addr 0x%0x) -> 0x%x (sz 0x%x): "
           "iter 0x%x rem 0x%x\r\n", i, fd_buf->name, sram_start_addr + offset,
           fd_buf->load_addr, fd_buf->size, iter, rem);

    for (j = 0; j < iter; j++) {
        * load_addr_32 = * sram_addr_32;
        load_addr_32++;
        sram_addr_32++;
    }
    load_addr = (unsigned char *) load_addr_32;
    sram_addr = (unsigned char *) sram_addr_32;
    for (j = 0; j < rem; j++) {
        * load_addr = * sram_addr;
        load_addr++;
        sram_addr++;
    }
    printf("SMC: loading completed\r\n");
    return 0;
}
