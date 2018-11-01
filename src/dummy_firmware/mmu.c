#include <stdint.h>
#include <stdbool.h>

#include "printf.h"
#include "regops.h"
#include "block.h"
#include "panic.h"
#include "mmu.h"

// References refer to either
//    ARM System MMU Architecture Specification (IHI0062D), or
//    ARM Architecture Reference Manual ARMv8 (DDI0487C) (Ch D4)

// TODO: pull from SMMU_IDR1
#define NUMCB         16
#define NUMPAGENDXB   0x3
#define PAGESIZE      0x1000

// TODO: register map in Qemu model assumes 0x1000 CB size, but minimum allowed by HW is 2 pages (see SMMU_IDR1)
#define NUMPAGES      1 // (2 * (NUMPAGENDXB + 1))

#define SMMU_BASE      0xf92f0000
#define SMMU_CB_SIZE   (NUMPAGES * PAGESIZE) // also size of global address space
#define SMMU_CB_BASE   (SMMU_BASE + 0x10000) // (SMMU_BASE + SMMU_CB_SIZE) // TODO: Qemu model does not agree with HW?
#define SMMU_CBn_BASE(n)  (SMMU_CB_BASE + (n) * SMMU_CB_SIZE)

#define SMMU__SCR0       0x00000
#define SMMU__SMR0       0x00800
#define SMMU__S2CR0      0x00c00
#define SMMU__CBAR0      0x01000
#define SMMU__CBA2R0     0x01800

#define SMMU__CB_SCTLR  0x00000
#define SMMU__CB_TTBR0  0x00020
#define SMMU__CB_TTBR1  0x00028
#define SMMU__CB_TCR    0x00030
#define SMMU__CB_TCR2   0x00010

#define SMMU__SMR0__VALID (1 << 31)
#define SMMU__SCR0__CLIENTPD 0x1
#define SMMU__S2CR0__EXIDVALID 0x400
#define SMMU__CBAR0__TYPE__MASK                      0x30000
#define SMMU__CBAR0__TYPE__STAGE1_WITH_STAGE2_BYPASS 0x10000
#define SMMU__CB_SCTLR__M 0x1
#define SMMU__CB_TCR2__PASIZE__40BIT 0b010
#define SMMU__CB_TCR__EAE__SHORT  0x00000000
#define SMMU__CB_TCR__EAE__LONG   0x80000000
#define SMMU__CB_TCR__VMSA8__TG0_4KB   (0b00 << 14)
#define SMMU__CB_TCR__VMSA8__TG0_16KB  (0b10 << 14)
#define SMMU__CB_TCR__VMSA8__TG0_64KB  (0b01 << 14)
#define SMMU__CB_TCR__T0SZ__SHIFT 0
#define SMMU__CB_TCR__TG0_4KB   0x0000
#define SMMU__CB_TCR__TG0_16KB  0x8000
#define SMMU__CB_TCR__TG0_64KB  0x4000
#define SMMU__CBA2R0__VA64 0x1

// VMSAv8 Long-descriptor fields
#define VMSA8_DESC__BITS   12 // Lower attributes in stage 1 (D4-2150)
#define VMSA8_DESC__VALID 0b1
#define VMSA8_DESC__TYPE_MASK   0b10
#define VMSA8_DESC__TYPE__BLOCK 0b00
#define VMSA8_DESC__TYPE__TABLE 0b10
#define VMSA8_DESC__TYPE__PAGE  0b10
#define VMSA8_DESC__UNPRIV 0x40
#define VMSA8_DESC__RO     0x80
#define VMSA8_DESC__AF     0x400
#define VMSA8_DESC__AP__EL0    0x40
#define VMSA8_DESC__AP__RO     0x80
#define VMSA8_DESC__AP__RW     0x00
#define VMSA8_DESC__NG     0x800
#define VMSA8_DESC__PXN    (1LL << 53)
#define VMSA8_DESC__XN     (1LL << 54)
#define VMSA8_DESC_BYTES 8 // each descriptor is 64 bits

// Fig D4-15: desc & ~(~0ULL << 48) & (MASK(granule->page_bits))
// but since this code runs on Aarch32 and can only address 32-bit ptrs (not
// 48-bit), we can just truncate the MSB part with a cast.
#define NEXT_TABLE_PTR(desc, granule) \
        (uint64_t *)((uint32_t)desc & ~((1 << granule->page_bits)-1)) // Fig D4-15

// Use Page Table 0 for 0x0 to 0x0000_0000_FFFF_FFFF
// NOTE: This the only config supported by this driver
#define T0SZ 32

#define GL_REG(reg)         ((volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + reg))
#define CB_REG32(ctx, reg)  ((volatile uint32_t *)((volatile uint8_t *)SMMU_CBn_BASE(ctx) + reg))
#define CB_REG64(ctx, reg)  ((volatile uint64_t *)((volatile uint8_t *)SMMU_CBn_BASE(ctx) + reg))
#define ST_REG(stream, reg) ((volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + reg) + stream)

#define MAX_LEVEL 3
#define MAX_CONTEXTS 8
#define MAX_STREAMS  8

struct level { // constant metadata about a level
    unsigned pt_size;
    unsigned pt_entries;
    unsigned block_size; // size of block or page
    unsigned idx_bits;
    unsigned lsb_bit;
};

struct granule { // constant metadata about a translation granule option
    unsigned tg;
    unsigned bits_per_level;
    unsigned bits_in_first_level;
    unsigned page_bits;
    unsigned start_level;
};

struct context {
    unsigned index; // of context in MMU instance
    const struct granule *granule;
    uint64_t *pt; // top-level page table (always allocated)
    struct level levels[MAX_LEVEL+1];
};

static const struct granule granules[] = {
        [MMU_PAGESIZE_4KB]  = {
                .tg = SMMU__CB_TCR__VMSA8__TG0_4KB,
                .page_bits = 12,    // Fig D4-15
                .bits_per_level = 9, // Table D4-9
#if (25 <= T0SZ) && (T0SZ <= 33)
                .start_level = 1,    // Table D4-26 (function of T0SZ)
                .bits_in_first_level = ((37 - T0SZ) + 26) - 30 + 1, // Table D4-26 (function of T0SZ): len([y:lsb])
#else
#error Driver does not support given value of T0SZ
#endif // T0SZ
        },
        [MMU_PAGESIZE_16KB]  = {
                .tg = SMMU__CB_TCR__VMSA8__TG0_16KB,
                .page_bits = 14,
                .bits_per_level = 11,
#if (28 <= T0SZ) && (T0SZ <= 38)
                .start_level = 2,    // Table D4-26 (function of T0SZ)
                .bits_in_first_level = ((42 - T0SZ) + 21) - 25 + 1, // Table D4-26 (function of T0SZ): len([y:lsb])
#else
#error Driver does not support given value of T0SZ
#endif // T0SZ
        },
        [MMU_PAGESIZE_64KB] = {
                .tg = SMMU__CB_TCR__VMSA8__TG0_64KB,
                .page_bits = 16,
                .bits_per_level = 13,
#if (22 <= T0SZ) && (T0SZ <= 34)
                .start_level = 2,    // Table D4-26 (function of T0SZ)
                .bits_in_first_level = ((38 - T0SZ) + 25) - 29 + 1, // Table D4-26 (function of T0SZ): len([y:lsb])
#else
#error Driver does not support given value of T0SZ
#endif // T0SZ
        },
};

static struct context contexts[MAX_CONTEXTS] = {0};
static bool streams[MAX_STREAMS] = {0}; // whether the indexed stream is allocated

static inline unsigned pt_index(struct level *levp, uint64_t vaddr) {
    return ((vaddr >> levp->lsb_bit) & ~(~0 << levp->idx_bits));
}

static uint64_t *pt_alloc(struct context *ctx, unsigned level)
{
    struct level *levp = &ctx->levels[level];
    printf("MMU: pt_alloc: ctx %u level %u size 0x%x align 0x%x\r\n",
           ctx->index, level, levp->pt_size, ctx->granule->page_bits);

    uint64_t *pt = block_alloc(levp->pt_size, ctx->granule->page_bits);
    if (!pt)
        return NULL;

    for (uint32_t i = 0; i < levp->pt_entries; ++i)
        pt[i] = ~VMSA8_DESC__VALID;

    printf("MMU: pt_alloc: allocated level %u pt at %p, cleared %u entries\r\n",
           level, pt, levp->pt_entries);
    return pt;
}

static int pt_free(struct context *ctx, uint64_t *pt, unsigned level)
{
    printf("MMU: pt_free: ctx %u level %u pt %p\r\n", ctx->index, level, pt);

    int rc;
    struct level *levp = &ctx->levels[level];
    uint64_t *next_pt;
    for (unsigned i = 0; i < levp->pt_entries; ++i) {
        uint64_t desc = pt[i];
        if (level < MAX_LEVEL && (pt[i] & VMSA8_DESC__VALID) &&
            ((desc & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__TABLE)) {
            next_pt = NEXT_TABLE_PTR(desc, ctx->granule);
            rc = pt_free(ctx, next_pt, level + 1);
            if (rc)
                return rc;
        }
    }
    return block_free(pt, levp->pt_size);
}

static void dump_pt(struct context *ctx, uint64_t *pt, unsigned level)
{
    struct level *levp = &ctx->levels[level];
    for (unsigned index = 0; index < levp->pt_entries; ++index) {
        uint64_t desc = pt[index];
        uint64_t *next_pt = NULL;
        if (desc & VMSA8_DESC__VALID) {
            const char *type;
            if (level < MAX_LEVEL &&
                (desc & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__TABLE) {
                type = "TABLE";
                next_pt = NEXT_TABLE_PTR(desc, ctx->granule);
            } else if ((desc & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__PAGE) {
                type = "PAGE";
            } else if ((desc & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__BLOCK) {
                type = "BLOCK";
            } else {
                type = "INVALID";
            }
            printf("MMU:");
            for (unsigned j = 0; j < level; ++j) // indent
                printf("    ");
            printf("L%u: %u: %p: %08x%08x [%s]\r\n",
                    level, index, &pt[index],
                    (uint32_t)(desc >> 32), (uint32_t)desc,
                    type);
            if (next_pt)
                dump_pt(ctx, next_pt, level + 1);
        }
    }
}

int mmu_init(uint64_t *ptaddr, unsigned ptspace)
{
    printf("MMU: init: addr %p size %x\r\n", ptaddr, ptspace);
    return block_init(ptaddr, ptspace);
}

int mmu_context_create(enum mmu_pagesize pgsz)
{
    printf("MMU: context_create: pgsz enum %u\r\n", pgsz);

    if (pgsz >= sizeof(granules) / sizeof(granules[0])) {
        printf("ERROR: MMU: unsupported page size: enum option #%u\r\n", pgsz);
        return -1;
    }

    // Alloc context state object
    unsigned ctx = 0;
    while (ctx < MAX_CONTEXTS && contexts[ctx].pt)
        ++ctx;
    if (ctx == MAX_CONTEXTS) {
        printf("ERROR: MMU: failed to create context: no more\r\n");
        return -1;
    }

    struct context *ctxp = &contexts[ctx];
    const struct granule *g = &granules[pgsz];
    ctxp->granule = g;
    ctxp->index = ctx;

    printf("MMU: context_create: alloced ctx %u start level %u\r\n",
           ctx, g->start_level);

    for (unsigned level = g->start_level; level <= MAX_LEVEL; ++level) {
        struct level *levp = &ctxp->levels[level];

        levp->idx_bits = (level == g->start_level) ?
                g->bits_in_first_level : g->bits_per_level;
        levp->lsb_bit = g->page_bits + 
            (MAX_LEVEL - g->start_level - (level - g->start_level)) * g->bits_per_level;

        levp->pt_entries = 1 << levp->idx_bits;
        levp->pt_size = levp->pt_entries * VMSA8_DESC_BYTES;
        levp->block_size = 1 << levp->lsb_bit; // value should match Table D4-21

        printf("MMU: context_create: level %u: pt entries %u pt size 0x%x "
               "block size 0x%x idx bits %u lsb_bit %u\r\n", level,
               levp->pt_entries, levp->pt_size, levp->block_size,
               levp->idx_bits, levp->lsb_bit);
    }

    ctxp->pt = pt_alloc(ctxp, ctxp->granule->start_level);
    if (!ctxp->pt)
        return -1;

    REG_WRITE32(CB_REG32(ctx, SMMU__CB_TCR),
              (T0SZ << SMMU__CB_TCR__T0SZ__SHIFT) | g->tg);
    REG_WRITE32(CB_REG32(ctx, SMMU__CB_TCR2), SMMU__CB_TCR2__PASIZE__40BIT);
    REG_WRITE64(CB_REG64(ctx, SMMU__CB_TTBR0), (uint32_t)ctxp->pt);
    REG_WRITE32(CB_REG32(ctx, SMMU__CB_SCTLR), SMMU__CB_SCTLR__M);

    printf("MMU: created ctx %u pt %p entries %u size 0x%x\r\n", ctx, ctxp->pt,
           ctxp->levels[ctxp->granule->start_level].pt_entries,
           ctxp->levels[ctxp->granule->start_level].pt_size);
    return ctx;
}

int mmu_context_destroy(unsigned ctx)
{
    int rc;

    printf("MMU: context_destroy: ctx %u\r\n", ctx);

    if (ctx >= MAX_CONTEXTS || !contexts[ctx].pt) {
        printf("ERROR: MMU invalid context index: %u\r\n", ctx);
        return -1;
    }

    REG_WRITE32(CB_REG32(ctx, SMMU__CB_TCR), 0);
    REG_WRITE32(CB_REG32(ctx, SMMU__CB_TCR2), 0);
    REG_WRITE64(CB_REG64(ctx, SMMU__CB_TTBR0), 0);
    REG_WRITE32(CB_REG32(ctx, SMMU__CB_SCTLR), 0);

    struct context *ctxp = &contexts[ctx];

    rc = pt_free(ctxp, ctxp->pt, ctxp->granule->start_level);

    // Free context state object
    ctxp->pt = NULL;

    printf("MMU: destroyed ctx %u\r\n", ctx);
    return rc;
}

int mmu_map(unsigned ctx, uint64_t vaddr, uint64_t paddr, unsigned sz)
{
    printf("MMU: map: ctx %u: 0x%08x%08x -> 0x%08x%08x size = %x\r\n",
            ctx, (uint32_t)(vaddr >> 32), (uint32_t)vaddr,
            (uint32_t)(paddr >> 32), (uint32_t)paddr, sz);

    struct context *ctxp = &contexts[ctx];

    if (!ALIGNED(sz, ctxp->granule->page_bits)) {
        printf("ERROR: MMU map: region size (0x%x) not aligned to TG of %u bits\r\n",
                sz, ctxp->granule->page_bits);
        return -1;
    }

    do { // iterate over pages or blocks

        printf("MMU: map: vaddr %08x%08x paddr %08x%08x sz 0x%x\r\n",
                (uint32_t)(vaddr >> 32), (uint32_t)vaddr,
                (uint32_t)(paddr >> 32), (uint32_t)paddr, sz);

        unsigned level = ctxp->granule->start_level;
        struct level *levp = &ctxp->levels[level];
        uint64_t *pt = ctxp->pt, *next_pt;
        unsigned index;
        uint64_t desc;

        while (level < MAX_LEVEL) {

            index = pt_index(levp, vaddr);
            desc = pt[index];
            next_pt = NULL;

            printf("MMU: map: walk: level %u pt %p: %u: %p: desc %08x%08x\r\n",
                    level, pt, index, &pt[index],
                    (uint32_t)(desc >> 32), (uint32_t)desc);

            if (ALIGNED(vaddr, levp->block_size) && sz >= levp->block_size) {
                printf("MMU: map: walk: level %u: block of size 0x%x\r\n",
                        level, levp->block_size);
                break; // insert a block mapping at this level
            }

            if  ((desc & VMSA8_DESC__VALID) &&
                    ((desc & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__TABLE)) {
                next_pt = NEXT_TABLE_PTR(desc, ctxp->granule);
                printf("MMU: map: walk: level %u: next pt %p\r\n", level, next_pt);
            }

            if (!next_pt) { // need to alloc a new PT
                next_pt = pt_alloc(ctxp, level + 1);
                pt[index] = (uint64_t)(uint32_t)next_pt |
                    VMSA8_DESC__VALID | VMSA8_DESC__TYPE__TABLE;
                printf("MMU: map: walk: level %u: pt pointer desc: %u: %p: <- %08x%08x\r\n",
                        level, index, &pt[index],
                        (uint32_t)(pt[index] >> 32), (uint32_t)pt[index]);
            }

            pt = next_pt;
            ++level;
            levp = &ctxp->levels[level];
        }

        index = pt_index(levp, vaddr);

        if ((pt[index] & VMSA8_DESC__VALID)) {
            printf("ERROR: MMU: overlapping/overwriting mappings not supported\r\n");
            // because we don't want to convert block <-> table when a smaller
            // region is overlapped onto a region mapped as a block, and simply
            // breaking previously created mappings is confusing semantics.
            return -1;
        }

        pt[index] = (paddr & (~0 << VMSA8_DESC__BITS)) |
            VMSA8_DESC__VALID |
            (level == MAX_LEVEL ? VMSA8_DESC__TYPE__PAGE : VMSA8_DESC__TYPE__BLOCK) |
            VMSA8_DESC__PXN | VMSA8_DESC__XN | // no execution permission
            VMSA8_DESC__AP__EL0 |
            VMSA8_DESC__AP__RW |
            VMSA8_DESC__AF; // set Access flag (otherwise we need to handle fault)

        printf("MMU: map: level %u: desc: %04u: 0x%p: <- 0x%08x%08x\r\n", level, index,
                &pt[index], (uint32_t)(pt[index] >> 32), (uint32_t)pt[index]);

        vaddr += levp->block_size;
        paddr += levp->block_size;
        sz -= levp->block_size;
    } while (sz > 0);

    dump_pt(ctxp, ctxp->pt, ctxp->granule->start_level);
    return 0;
}

int mmu_unmap(unsigned ctx, uint64_t vaddr, unsigned sz)
{
    printf("MMU: unmap: ctx %u: 0x%08x%08x size 0x%x\r\n",
            ctx, (uint32_t)(vaddr >> 32), (uint32_t)vaddr, sz);

    struct context *ctxp = &contexts[ctx];

    if (!ALIGNED(sz, ctxp->granule->page_bits)) {
        printf("ERROR: MMU unmap: region size (0x%x) not aligned to TG of %u bits\r\n",
                sz, ctxp->granule->page_bits);
        return -1;
    }

    struct walk_step {
        uint64_t *pt;
        unsigned index;
    };
    struct walk_step walk[MAX_LEVEL + 1];

    do { // iterate over pages or blocks

        int level = ctxp->granule->start_level;
        struct level *levp = &ctxp->levels[level];
        uint64_t *pt = contexts[ctx].pt;
        unsigned index;
        uint64_t desc;

        while (level < MAX_LEVEL) {

            index = pt_index(levp, vaddr);
            desc = pt[index];

            printf("MMU: unmap: walk: level %u pt %p %u: %p: %08x%08x\r\n",
                    level, pt, index, &pt[index],
                    (uint32_t)(desc >> 32), (uint32_t)desc);

            walk[level].pt = pt;
            walk[level].index = index;

            if (ALIGNED(vaddr, levp->block_size) && sz >= levp->block_size) {
                printf("MMU: unmap: walk: level %u: pt %p: block of size 0x%x\r\n",
                        level, pt, levp->block_size);
                break; // it's has to be a block mapping at this level, delete it below
            }

            if  ((desc & VMSA8_DESC__VALID) &&
                    ((desc & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__TABLE)) {
                pt = NEXT_TABLE_PTR(desc, ctxp->granule);
                printf("MMU: unmap: walk: level %u: next pt %p\r\n", level, pt);
            } else {
                printf("ERROR: MMU: expected page descriptor does not exist\r\n");
                return -1;
            }

            ++level;
            levp = &ctxp->levels[level];
        }

        index = pt_index(levp, vaddr);

        if (!(pt[index] & VMSA8_DESC__VALID)) {
            printf("ERROR: MMU: expected page or block descriptor does not exist\r\n");
            return -1;
        }

        printf("MMU: unmap: level %u: desc: %04u: 0x%p: 0x%08x%08x\r\n", level, index,
                &pt[index], (uint32_t)(pt[index] >> 32), (uint32_t)pt[index]);
        pt[index] = ~VMSA8_DESC__VALID;
        printf("MMU: unmap: level %u: desc: %04u: 0x%p: <- 0x%08x%08x\r\n", level, index,
                &pt[index], (uint32_t)(pt[index] >> 32), (uint32_t)pt[index]);

        // Walk the levels backwards and destroy empty tables
        while (--level >= ctxp->granule->start_level) {

            struct walk_step *step = &walk[level];

            desc = step->pt[step->index];

            ASSERT((desc & VMSA8_DESC__VALID) &&
                    ((desc & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__TABLE));

            uint64_t *next_pt = NEXT_TABLE_PTR(desc, ctxp->granule);

            printf("MMU: unmap: backwalk: level %u: %04u: 0x%p: 0x%08x%08x\r\n",
                    level, step->index, &step->pt[index],
                    (uint32_t)(desc >> 32), (uint32_t)desc);

            struct level *back_levp = &ctxp->levels[level + 1]; // child level checked for deletion
            bool pt_empty = true;
            for (unsigned i = 0; i < back_levp->pt_entries; ++i) {
                if (next_pt[i] & VMSA8_DESC__VALID) {
                    pt_empty = false;
                    break;
                }
            }
            if (!pt_empty)
                break;

            step->pt[step->index] = ~VMSA8_DESC__VALID;

            printf("MMU: unmap: backwalk: level %u: %04u: 0x%p: <- 0x%08x%08x\r\n",
                    level, step->index, &step->pt[index],
                    (uint32_t)(step->pt[step->index] >> 32),
                    (uint32_t)step->pt[step->index]);

            int rc = pt_free(ctxp, next_pt, level + 1);
            if (rc)
                return rc;
        }

        vaddr += levp->block_size;
        sz -= levp->block_size;
    } while (sz > 0);
    dump_pt(ctxp, ctxp->pt, ctxp->granule->start_level);
    return 0;
}

int mmu_stream_create(unsigned master, unsigned ctx)
{
    int stream = 0;

    while (stream < MAX_STREAMS && streams[stream])
        ++stream;
    if (stream == MAX_STREAMS) {
        printf("ERROR: failed to setup stream: no more match registers\r\n");
        return -1;
    }
    streams[stream] = true;

    printf("MMU: stream %u -> ctx %u\r\n", stream, ctx);

    REG_WRITE32(ST_REG(stream, SMMU__SMR0), SMMU__SMR0__VALID | master);
    REG_WRITE32(ST_REG(stream, SMMU__S2CR0), SMMU__S2CR0__EXIDVALID | ctx);
    REG_WRITE32(ST_REG(stream, SMMU__CBAR0), SMMU__CBAR0__TYPE__STAGE1_WITH_STAGE2_BYPASS);
    REG_WRITE32(ST_REG(stream, SMMU__CBA2R0), SMMU__CBA2R0__VA64);

    printf("MMU: created stream %u -> ctx %u\r\n", stream, ctx);
    return stream;
}

int mmu_stream_destroy(unsigned stream)
{
    if (stream >= MAX_STREAMS) {
        printf("ERROR: MMU: stream destroy: invalid stream %u >= %u\r\n",
               stream, MAX_STREAMS);
        return -1;
    }
    if (!streams[stream]) {
        printf("ERROR: MMU: stream destroy: stream %u does not exist\r\n",
               stream);
        return -1;
    }
    streams[stream] = false;

    REG_WRITE32(ST_REG(stream, SMMU__SMR0), 0);
    REG_WRITE32(ST_REG(stream, SMMU__S2CR0), 0);
    REG_WRITE32(ST_REG(stream, SMMU__CBAR0), 0);
    REG_WRITE32(ST_REG(stream, SMMU__CBA2R0), 0);

    printf("MMU: destroyed stream %u\r\n", stream);
    return 0;
}

void mmu_enable()
{
    printf("MMU: enable\r\n");
    REG_CLEAR32(GL_REG(SMMU__SCR0), SMMU__SCR0__CLIENTPD);
}

void mmu_disable()
{
    printf("MMU: disable\r\n");
    REG_SET32(GL_REG(SMMU__SCR0), SMMU__SCR0__CLIENTPD);
}
