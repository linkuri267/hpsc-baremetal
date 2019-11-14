#ifndef RIO_SVC_H
#define RIO_SVC_H

#include <stdbool.h>

struct rio_svc {
    struct rio_switch *sw;
    struct rio_ep *ep0, *ep1;
};

struct rio_svc *rio_svc_create(bool master);
void rio_svc_destroy(struct rio_svc *svc);

#endif /* RIO_SVC_H */
