
#ifndef MUTEX_H
#define MUTEX_H
#define locked    1
#define unlocked  0

extern void lock_mutex(void * mutex);
extern void unlock_mutex(void * mutex);
#endif
