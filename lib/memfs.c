#include "printf.h"
#include "panic.h"
#include "str.h"
#include "dma.h"
#include "bit.h"
#include "object.h"

#include "memfs.h"

#define FILE_NAME_LENGTH 200
#define ECC_512_SIZE	3
#define PAGE_SIZE (1 << 14) // 16KB (used for progress display only)

typedef struct {
    uint32_t valid;
    uint32_t offset;	/* offset in memory */
    uint32_t size;
    uint32_t load_addr;		/* 32bit load address in DRAM at run-time */
    uint32_t load_addr_high;	/* high 32bit of 64 bit load address in DRAM at run-time */
    char  name[FILE_NAME_LENGTH];
    uint32_t entry_offset;	/* the offset of the entry point in the image */
    uint8_t chcksum[32];		/* SHA-256 checksum */
    uint8_t ecc[ECC_512_SIZE];	/* ecc of the struct */
} file_descriptor;

typedef struct {
    uint32_t low_mark_data;	/* low mark of data */
    uint32_t high_mark_fd;	/* high mark of file descriptors */
    uint32_t n_files;		/* number of files */
    uint32_t fsize;		/* sram file size */
    uint8_t ecc[ECC_512_SIZE];   /* ecc of the struct */
} global_table;

struct memfs {
    uintptr_t base;
    struct dma *dmac; // optional, for loading files via DMA
};

#define MAX_MEMFS 2
static struct memfs memfss[MAX_MEMFS];

static int load_dma(uint32_t *sram_addr, uint32_t *load_addr, unsigned size,
                    struct dma *dmac)
{
    printf("MEMFS: initiating DMA transfer\r\n");

    struct dma_tx *dtx = dma_transfer(dmac, /* chan */ 0, // TODO: allocate channel per user
        sram_addr, load_addr, ALIGN(size, DMA_MAX_BURST_BITS),
        NULL, NULL /* no callback */);
    int rc = dma_wait(dtx);
    if (rc) {
        printf("MEMFS: DMA transfer failed: rc %u\r\n", rc);
        return rc;
    }
    printf("MEMFS: DMA transfer succesful\r\n");
    return 0;
}


static int load_memcpy(uint32_t *mem_addr, uint32_t *load_addr, unsigned size)
{
    unsigned p, w, b;

    uint32_t pages = size / PAGE_SIZE;
    uint32_t rem_words = (size % PAGE_SIZE) / sizeof(uint32_t);
    uint32_t rem_bytes = (size % PAGE_SIZE) % sizeof(uint32_t);

    // Split into pages, in order to print progress not too frequently and
    // without having to add a conditional (for whether to print progress
    // on every iteration of in the inner loop over words.
    for (p = 0; p < pages; p++) {
        for (w = 0; w < PAGE_SIZE / sizeof(uint32_t); w++) {
            * load_addr = * mem_addr;
            load_addr++;
            mem_addr++;
        }
        printf("MEMFS: loading... %3u%%\r", p * 100 / pages);
    }
    for (w = 0; w < rem_words; w++) {
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
    printf("MEMFS: loading... 100%%\r\n");
    return 0;
}

struct memfs *memfs_mount(uintptr_t base, struct dma *dmac)
{
    struct memfs *fs;
    fs = OBJECT_ALLOC(memfss);
    fs->base = base;
    fs->dmac = dmac;
    // could load the file table here
    return fs;
}

void memfs_unmount(struct memfs *fs)
{
    ASSERT(fs);
    fs->base = 0;
    fs->dmac = NULL;
    OBJECT_FREE(fs);
}

int memfs_load(struct memfs *fs, const char *fname,
               uint32_t **addr, uint32_t **ep)
{
    global_table gt;
    unsigned i;
    uint32_t * load_addr_32;
    uint32_t * mem_addr_32;
    file_descriptor * fd_buf;
    unsigned char * mem_start_addr = (unsigned char *)fs->base;
    unsigned char * ptr = (unsigned char *) &gt;
    int rc;

    for(i = 0; i < sizeof(global_table); i++) {
        ptr[i] = * (mem_start_addr + i);
    }
    printf("MEMFS: #files : %u, low_mark_data(0x%lx), high_mark_fd(0x%x)\r\n",
           gt.n_files, gt.low_mark_data, gt.high_mark_fd);

    for (i = 0; i < gt.n_files ; i++) {
        fd_buf = (file_descriptor *)(mem_start_addr + sizeof(gt) + sizeof(file_descriptor)*i);
        if (!fd_buf->valid)
            continue;
        if (!strcmp(fd_buf->name, fname))
            break;
    }
    if (i == gt.n_files) {
        printf("MEMFS: ERROR: file not found: %s\r\n", fname);
        return 1;
    }

    uint32_t offset = fd_buf->offset;

    printf("MEMFS: loading file #%u: %s: 0x%0x -> 0x%x (%u KB)\r\n",
           i, fd_buf->name, mem_start_addr + offset,
           fd_buf->load_addr, fd_buf->size / 1024);

    mem_addr_32 = (uint32_t *) (mem_start_addr + offset);
    load_addr_32 = (uint32_t *)fd_buf->load_addr;

    if (fs->dmac)
        rc = load_dma(mem_addr_32, load_addr_32, fd_buf->size, fs->dmac);
    else
        rc = load_memcpy(mem_addr_32, load_addr_32, fd_buf->size);

    if (addr)
        *addr = load_addr_32;
    if (ep)
        *ep = (uint32_t *)(fd_buf->load_addr + fd_buf->entry_offset);
    return rc;
}
