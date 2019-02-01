#include <stdint.h>

#include "mem.h"
#include "object.h"
#include "panic.h"
#include "shmem.h"

#define MAX_SHMEMS 16

// All subsystems must understand this structure and its protocol
struct hpsc_shmem_region {
    uint8_t data[SHMEM_MSG_SIZE];
    uint32_t is_new;
};

struct shmem {
    struct object obj;
    void *addr;
};

static struct shmem shmems[MAX_SHMEMS] = {0};

struct shmem *shmem_open(void *addr)
{
    struct shmem *s = OBJECT_ALLOC(shmems);
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
    ASSERT(sz <= SHMEM_MSG_SIZE);
    vmem_cpy(shm->data, msg, sz);
    if (sz_rem)
        vmem_set(shm->data + sz, 0, sz_rem);
    shm->is_new = 1;
    return sz;
}

uint32_t shmem_get_status(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    return shm->is_new;
}

size_t shmem_recv(struct shmem *s, void *msg, size_t sz)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    ASSERT(sz >= SHMEM_MSG_SIZE);
    mem_vcpy(msg, shm->data, SHMEM_MSG_SIZE);
    shm->is_new = 0;
    return SHMEM_MSG_SIZE;
}
