#ifndef PSCI_H
#define PSCI_H

/* returns length of reply */
int handle_psci(struct cmd * cmd, void *reply_buf, unsigned reply_sz);

#endif
