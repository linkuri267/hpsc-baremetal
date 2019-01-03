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

char *strcat(char *s, const char *t)
{
    char *r = s;
    while (*r++);
    --r;
    while (*t)
        *r++ = *t++;
    *r = '\0';
    return s;
}
