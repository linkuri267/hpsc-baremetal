#include <stdbool.h>
#include <stdint.h>

#include "mem.h"
#include "object.h"
#include "panic.h"
#include "shmem.h"

#define MAX_SHMEMS 16

struct shmem {
    struct object obj;
    volatile void *addr;
};

static struct shmem shmems[MAX_SHMEMS] = {0};

#define IS_ALIGNED(p) (((uintptr_t)(const void *)(p) % sizeof(uint32_t)) == 0)

struct shmem *shmem_open(volatile void *addr)
{
    struct shmem *s = OBJECT_ALLOC(shmems);
    ASSERT(IS_ALIGNED(addr));
    if (s)
        s->addr = addr;
    return s;
}

void shmem_close(struct shmem *s)
{
    OBJECT_FREE(s);
}

size_t shmem_send(struct shmem *s, void *msg, size_t sz)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    size_t sz_rem = SHMEM_MSG_SIZE - sz;
    ASSERT(IS_ALIGNED(msg));
    ASSERT(sz <= SHMEM_MSG_SIZE);
    vmem_cpy(shm->data, msg, sz);
    if (sz_rem)
        vmem_set(shm->data + sz, 0, sz_rem);
    shm->status = shm->status | HPSC_SHMEM_STATUS_BIT_NEW;
    return sz;
}

uint32_t shmem_get_status(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    return shm->status;
}

bool shmem_is_new(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    return shm->status & HPSC_SHMEM_STATUS_BIT_NEW;
}

bool shmem_is_ack(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    return shm->status & HPSC_SHMEM_STATUS_BIT_ACK;
}

void shmem_clear_ack(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    shm->status &= ~HPSC_SHMEM_STATUS_BIT_ACK;
}

size_t shmem_recv(struct shmem *s, void *msg, size_t sz)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    ASSERT(sz >= SHMEM_MSG_SIZE);
    ASSERT(IS_ALIGNED(msg));
    mem_vcpy(msg, shm->data, SHMEM_MSG_SIZE);
    shm->status = shm->status & ~HPSC_SHMEM_STATUS_BIT_NEW; // clear new
    shm->status = shm->status | HPSC_SHMEM_STATUS_BIT_ACK; // set ACK
    return SHMEM_MSG_SIZE;
}
