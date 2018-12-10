#include "str.h"

int strcmp(const char *s, const char *t)
{
    while (*s && *t && *s == *t) {
        s++;
        t++;
    }
    if (*s || *t)
        return *s < *t ? -1 : 1;
    return 0;
}
