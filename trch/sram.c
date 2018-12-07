#include <stdint.h>
#include "sram.h"
#include "printf.h"

#define SRAM_START_ADDR 0x28000000
#define MAX_SRAM_SIZE   0x08000000


void file_load_from_sram()
{
    global_table gt;
    uint64_t i, j;
    file_descriptor * fd_buf;
    unsigned char * sram_start_addr = (unsigned char *)SRAM_START_ADDR;
    unsigned char * load_addr;
    unsigned char * sram_addr;
    unsigned char * ptr = (unsigned char *) &gt;
    uint32_t * load_addr_32;
    uint32_t * sram_addr_32;
 
    for(i = 0; i < sizeof(global_table); i++) {
        ptr[i] = * (sram_start_addr + i);
    }
    printf("Files in SRAM: 0x%lx, low_mark_data(0x%lx), high_mark_fd(0x%x)\n", gt.n_files, gt.low_mark_data, gt.high_mark_fd);
    for (i = 0; i < gt.n_files ; i++) {
        uint32_t offset;
        fd_buf = (file_descriptor *)(sram_start_addr + sizeof(gt) + sizeof(file_descriptor)*i);
        if (!fd_buf->valid) { i--; continue;}
        offset = fd_buf->offset;
        sram_addr = (unsigned char *) (sram_start_addr + offset);
        load_addr = (unsigned char *)fd_buf->load_addr;
        sram_addr_32 = (uint32_t *) (sram_start_addr + offset);
        load_addr_32 = (uint32_t *)fd_buf->load_addr;
        uint32_t iter = fd_buf->size / sizeof(uint32_t);
        uint32_t rem = fd_buf->size % sizeof(uint32_t);
        printf("Loading %d-th file\n", i);
        printf("     @ 0x%x\n", fd_buf->load_addr);
        printf("     size(0x%x)\n", fd_buf->size);
        printf("     sramaddr(0x%x)\n", sram_start_addr + offset);
        printf("     iter (0x%x)\n", iter);
        printf("     rem (0x%x)\n", rem);
        for (j = 0; j < iter; j++) {
            * load_addr_32 = * sram_addr_32;
            load_addr_32++;
            sram_addr_32++;
            printf("%-th\n", j);
        }
        load_addr = (unsigned char *) load_addr_32;
        sram_addr = (unsigned char *) sram_addr_32;
        for (j = 0; j < rem; j++) {
            * load_addr = * sram_addr;
            load_addr++;
            sram_addr++;
        }

    }
}

