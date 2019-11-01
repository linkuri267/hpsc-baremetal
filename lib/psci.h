#ifndef LIB_PSCI_H
#define LIB_PSCI_H

#include "subsys.h"

struct link;

int psci_release_reset(struct link *psci_link, comp_t requester, comp_t comp);

#endif /* LIB_PSCI_H */
