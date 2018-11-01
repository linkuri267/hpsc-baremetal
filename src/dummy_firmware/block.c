#include "printf.h"
#include "panic.h"
#include "block.h"

#define MAX_BLOCKS 128 // max free blocks

// Extra bytes needed to add to the addres to make the address aligned
#define PADDING(addr, align_bits) \
        (((1 << align_bits) - ((uint32_t)addr & ((1 << align_bits) - 1))) & \
                ~(~0 << align_bits))

struct block {
    uint8_t *addr;
    unsigned size;
};

static struct block free_blocks[MAX_BLOCKS] = {0};

static void dump_free_blocks()
{
    printf("BLOCK: free blocks: ");
    for (unsigned i = 0; i < MAX_BLOCKS; ++i)
        if (free_blocks[i].addr)
            printf("(%p,+%x) ", free_blocks[i].addr, free_blocks[i].size);
    printf("\r\n");
}

int block_init(void *addr, unsigned size)
{
    free_blocks[0].addr = addr;
    free_blocks[0].size = size;
    printf("BLOCK: init: new free block 0 (%p,0x%x)\r\n",
           free_blocks[0].addr, free_blocks[0].size);
    return 0;
}

void *block_alloc(unsigned sz, unsigned align_bits)
{
    void *b;

    printf("BLOCK: block_alloc: sz 0x%x align 0x%x\r\n", sz, align_bits);
    dump_free_blocks();

    unsigned i = 0;

    // first look for a block that is already aligned
    while (i < MAX_BLOCKS &&
           !(free_blocks[i].addr && free_blocks[i].size >= sz &&
             ALIGNED(free_blocks[i].addr, align_bits)))
        ++i;

    if (i == MAX_BLOCKS) {
        i = 0;
        while (i < MAX_BLOCKS &&
               !(free_blocks[i].addr &&
                 free_blocks[i].size >= sz + PADDING(free_blocks[i].addr, align_bits) ))
            ++i;

        if (i == MAX_BLOCKS) {
            printf("ERROR: BLOCK: alloc failed: out of mem\r\n");
            return NULL;
        }

        // Split this block into two: padding and aligned space (to be used shortly below)

        unsigned j = 0;
        while (j < MAX_BLOCKS && free_blocks[j].addr)
            ++j;

        if (j == MAX_BLOCKS) {
            printf("ERROR: BLOCK: alloc failed: no space for free block\r\n");
            return NULL;
        }

        unsigned padding = PADDING(free_blocks[i].addr, align_bits);

        free_blocks[j].addr = free_blocks[i].addr + padding;
        free_blocks[j].size = free_blocks[i].size - padding;

        printf("BLOCK: blocks: split block %u (%p,+%x) into %u (%p,+%x) and %u (%p,+%x)\r\n",
               i, free_blocks[i].addr, free_blocks[i].size,
               i, free_blocks[i].addr, padding,
               j, free_blocks[j].addr, free_blocks[j].size);

        free_blocks[i].size = padding;

        i = j;
    }

    b = free_blocks[i].addr;
    ASSERT(ALIGNED(b, align_bits));


    printf("BLOCK: block_alloc: block %p sz 0x%x from block %u (%p,+%x)\r\n",
           b, sz, i, free_blocks[i].addr, free_blocks[i].size);

    free_blocks[i].size -= sz;
    if (free_blocks[i].size)
        free_blocks[i].addr += sz;
    else
        free_blocks[i].addr = NULL;

    dump_free_blocks();
    return b;
}

int block_free(void *addr, unsigned sz)
{
    printf("BLOCK: block_free: addr %p sz %x\r\n", addr, sz);
    dump_free_blocks();

    unsigned i = 0;
    while (i < MAX_BLOCKS &&
          (free_blocks[i].addr && free_blocks[i].addr - sz != addr))
        ++i;

    if (i < MAX_BLOCKS && free_blocks[i].addr - sz == addr) {
        printf("BLOCK: block_free: coalesced into block %u (%p,+%x)\r\n",
               i, free_blocks[i].addr, free_blocks[i].size);
        free_blocks[i].addr -= sz;
        free_blocks[i].size += sz;
    } else {
        i = 0;
        while (i < MAX_BLOCKS && free_blocks[i].addr)
            ++i;
        if (i == MAX_BLOCKS) {
            printf("ERROR: BLOCK: free failed: no space for free block\r\n");
            return -1;
        }
        free_blocks[i].addr = addr;
        free_blocks[i].size = sz;
        printf("BLOCK: block_free: new free block %u (%p,+%x)\r\n",
               i, free_blocks[i].addr, sz);
    }

    dump_free_blocks();
    // TODO: could coallesce all blocks here for O(n^2)
    return 0;
}
