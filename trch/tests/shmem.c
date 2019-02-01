#include "mem-map.h"
#include "printf.h"
#include "shmem.h"

#include "test.h"

static char *msg = "Test Message";
// double the size just to make sure it works
static char buf[SHMEM_MSG_SIZE * 2] = {0};

int test_shmem()
{
    size_t sz;
    int ret = 0;
    uint32_t status;
    struct shmem *shm = shmem_open((void *)HPPS_SHM_ADDR__TRCH_HPPS);
    if (!shm)
        return 1;
    status = shmem_get_status(shm);
    if (status) {
        printf("ERROR: TEST: shmem: initial status failed\r\n");
        ret = 1;
        goto out;
    }
    sz = shmem_send(shm, msg, sizeof(msg));
    if (sz != sizeof(msg)) {
        printf("ERROR: TEST: shmem: send failed\r\n");
        ret = 1;
        goto out;
    }
    status = shmem_get_status(shm);
    if (!status) {
        printf("ERROR: TEST: shmem: intermediate status failed\r\n");
        ret = 1;
        goto out;
    }
    sz = shmem_recv(shm, buf, sizeof(buf));
    if (sz != SHMEM_MSG_SIZE) {
        printf("ERROR: TEST: shmem: recv failed\r\n");
        ret = 1;
        goto out;
    }
    status = shmem_get_status(shm);
    if (status) {
        printf("ERROR: TEST: shmem: final status failed\r\n");
        ret = 1;
        goto out;
    }
    printf("TEST: shmem: success\r\n");

out:
    shmem_close(shm);
    return ret;
}
