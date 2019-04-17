#ifndef SHMEM_LINK_H
#define SHMEM_LINK_H

#include "link.h"

struct link *shmem_link_connect(const char *name, volatile void *addr_out,
                                volatile void *addr_in);

#endif // SHMEM_LINK_H
