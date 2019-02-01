/**
 * Simple linked list backed by an array used for node allocation.
 */
#ifndef LLIST_H
#define LLIST_H

#include "object.h"

#define LLIST_MAX_NODES 16

struct llist_node {
    struct object obj;
    void *data;
    struct llist_node *next;
};

struct llist {
    struct llist_node nodes[LLIST_MAX_NODES];
    struct llist_node *head;
    struct llist_node *iter;
};

// initialize a list before use
void llist_init(struct llist *l);

// add to the front of the list, return 0 on success, -1 on failure
int llist_insert(struct llist *l, void *data);

// remove first matching entry from the list, return 0 on success, -1 on failure
int llist_remove(struct llist *l, void *data);

// reset the internal iterator
void llist_iter_init(struct llist *l);

// get the next data item in the list, or NULL
void *llist_iter_next(struct llist *l);

#endif // LLIST_H
