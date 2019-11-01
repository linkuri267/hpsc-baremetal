#ifndef CONSOLE_H
#define CONSOLE_H

#if CONFIG_CONSOLE

int console_init();

#include "printf.h"

#else /* !CONFIG_CONSOLE */

#define console_init()

#define printf(...)
#define sprintf(...)
#define snprintf(...)
#define vsnprintf(...)
#define fctprintf(...)

#endif /* CONFIG_CONSOLE */

#endif /* CONSOLE_H */
