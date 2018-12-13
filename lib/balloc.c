#include <stdint.h>
#include <stdbool.h>

#include "printf.h"
#include "panic.h"
#include "mem.h"
#include "object.h"
#include "balloc.h"

#define MAX_ALLOCATORS 4

#define MAX_BLOCKS 128 // max free blocks

// Extra bytes needed to add to the addres to make the address aligned
#define PADDING(addr, align_bits) \
        (((1 << align_bits) - ((uint32_t)addr & ((1 << align_bits) - 1))) & \
                ~(~0 << align_bits))

struct block {
    uint8_t *addr;
    unsigned size;
};

struct balloc {
    bool valid; // for storing this object in an array
    const char *name;
    struct block free_blocks[MAX_BLOCKS];
};

static struct balloc ballocs[MAX_ALLOCATORS] = {0};

static void dump_balloc(struct balloc *ba)
{
    printf("BALLOC %s: free blocks: ", ba->name);
    for (unsigned i = 0; i < MAX_BLOCKS; ++i)
        if (ba->free_blocks[i].addr)
            printf("(%p,+%x) ", ba->free_blocks[i].addr, ba->free_blocks[i].size);
    printf("\r\n");
}

struct balloc *balloc_create(const char *name, void *addr, unsigned size)
{
    struct balloc *ba = OBJECT_ALLOC(ballocs);
    if (!ba)
        return NULL;
    ba->name = name;
    ba->free_blocks[0].addr = addr;
    ba->free_blocks[0].size = size;

    printf("BALLOC %s: init: zero-initing free block 0 (%p,0x%x)\r\n",
            ba->name, ba->free_blocks[0].addr, ba->free_blocks[0].size);
    bzero(addr, size);
    printf("BALLOC %s: ready\r\n", ba->name);
    return ba;
}

void balloc_destroy(struct balloc *ba)
{
     ASSERT(ba);
     printf("BALLOC %s: destroy\r\n", ba->name);
     OBJECT_FREE(ba);
}

void *balloc_alloc(struct balloc *ba, unsigned sz, unsigned align_bits)
{
    ASSERT(ba);

    printf("BALLOC %s: balloc_alloc: sz 0x%x align 0x%x\r\n",
           ba->name, sz, align_bits);
    dump_balloc(ba);

    struct block *free_blocks = &ba->free_blocks[0];
    unsigned i = 0;
    void *b;

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
            printf("ERROR: BALLOC %s: alloc failed: out of mem\r\n", ba->name);
            return NULL;
        }

        // Split this block into two: padding and aligned space (to be used shortly below)

        unsigned j = 0;
        while (j < MAX_BLOCKS && free_blocks[j].addr)
            ++j;

        if (j == MAX_BLOCKS) {
            printf("ERROR: BALLOC %s: alloc failed: no space for free block\r\n",
                   ba->name);
            return NULL;
        }

        unsigned padding = PADDING(free_blocks[i].addr, align_bits);

        free_blocks[j].addr = free_blocks[i].addr + padding;
        free_blocks[j].size = free_blocks[i].size - padding;

        printf("BALLOC %s: blocks: split block %u (%p,+%x) into %u (%p,+%x) and %u (%p,+%x)\r\n",
               ba->name,
               i, free_blocks[i].addr, free_blocks[i].size,
               i, free_blocks[i].addr, padding,
               j, free_blocks[j].addr, free_blocks[j].size);

        free_blocks[i].size = padding;

        i = j;
    }

    b = free_blocks[i].addr;
    ASSERT(ALIGNED(b, align_bits));


    printf("BALLOC %s: balloc_alloc: block %p sz 0x%x from block %u (%p,+%x)\r\n",
           ba->name, b, sz, i, free_blocks[i].addr, free_blocks[i].size);

    free_blocks[i].size -= sz;
    if (free_blocks[i].size)
        free_blocks[i].addr += sz;
    else
        free_blocks[i].addr = NULL;

    dump_balloc(ba);
    return b;
}

int balloc_free(struct balloc *ba, void *addr, unsigned sz)
{
    ASSERT(ba);

    printf("BALLOC %s: balloc_free: addr %p sz %x\r\n", ba->name, addr, sz);
    dump_balloc(ba);

    struct block *free_blocks = &ba->free_blocks[0];

    unsigned i = 0;
    while (i < MAX_BLOCKS &&
          (free_blocks[i].addr && free_blocks[i].addr - sz != addr))
        ++i;

    if (i < MAX_BLOCKS && free_blocks[i].addr - sz == addr) {
        printf("BALLOC %s: balloc_free: coalesced into block %u (%p,+%x)\r\n",
               ba->name, i, free_blocks[i].addr, free_blocks[i].size);
        free_blocks[i].addr -= sz;
        free_blocks[i].size += sz;
    } else {
        i = 0;
        while (i < MAX_BLOCKS && free_blocks[i].addr)
            ++i;
        if (i == MAX_BLOCKS) {
            printf("ERROR: BALLOC %s: free failed: no space for free block\r\n",
                   ba->name);
            return -1;
        }
        free_blocks[i].addr = addr;
        free_blocks[i].size = sz;
        printf("BALLOC %s: balloc_free: new free block %u (%p,+%x)\r\n",
               ba->name, i, free_blocks[i].addr, sz);
    }

    dump_balloc(ba);
    // TODO: could coallesce all blocks here for O(n^2)
    return 0;
}
