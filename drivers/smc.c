#include <stdint.h>

#include "printf.h"
#include "hwinfo.h"
#include "str.h"

#include "smc.h"

#define FILE_NAME_LENGTH 200

#define PAGE_SIZE (1 << 14) // 16KB (used for progress display only)

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
    unsigned i, p, w, b;
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

    printf("SMC: loading file #%u: %s: 0x%0x -> 0x%x (%u KB)\r\n",
           i, fd_buf->name, sram_start_addr + offset,
           fd_buf->load_addr, fd_buf->size / 1024);

    sram_addr_32 = (uint32_t *) (sram_start_addr + offset);
    load_addr_32 = (uint32_t *)fd_buf->load_addr;
    uint32_t pages = fd_buf->size / PAGE_SIZE;
    uint32_t rem_words = (fd_buf->size % PAGE_SIZE) / sizeof(uint32_t);
    uint32_t rem_bytes = (fd_buf->size % PAGE_SIZE) % sizeof(uint32_t);


    // Split into pages, in order to print progress not too frequently and
    // without having to add a conditional (for whether to print progress
    // on every iteration of in the inner loop over words.
    for (p = 0; p < pages; p++) {
        for (w = 0; w < PAGE_SIZE / sizeof(uint32_t); w++) {
            * load_addr_32 = * sram_addr_32;
            load_addr_32++;
            sram_addr_32++;
        }
        printf("SMC: loading... %3u%%\r", p * 100 / pages);
    }
    for (w = 0; w < rem_words; w++) {
        * load_addr_32 = * sram_addr_32;
        load_addr_32++;
        sram_addr_32++;
    }
    load_addr = (unsigned char *) load_addr_32;
    sram_addr = (unsigned char *) sram_addr_32;
    for (b = 0; b < rem_bytes; b++) {
        * load_addr = * sram_addr;
        load_addr++;
        sram_addr++;
    }
    printf("SMC: loading... 100%%\r\n");
    return 0;
}
