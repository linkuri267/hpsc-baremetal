#include <stdlib.h>

#include "llist.h"
#include "object.h"
#include "console.h"

static int ll_remove(struct llist_node *node, void *data)
{
    // removes the first matching entry
    struct llist_node *tmp;
    if (!node || !node->next)
        return -1;
    if (node->next->data == data) {
        tmp = node->next;
        node->next = node->next->next;
        OBJECT_FREE(tmp);
        return 0;
    }
    return ll_remove(node->next, data);
}

void llist_init(struct llist *l)
{
    l->head = NULL;
    l->iter = NULL;
}

int llist_insert(struct llist *l, void *data)
{
    struct llist_node *node = OBJECT_ALLOC(l->nodes);
    if (!node) 
        return -1;
    l->iter = (struct llist_node *) -1;
    node->data = data;
    node->next = l->head;
    l->head = node;
    return 0;
}

int llist_remove(struct llist *l, void *data)
{
    struct llist_node *tmp;
    l->iter = (struct llist_node *) -1;
    if (!l->head)
        return -1;
    // head of list is special case
    if (l->head->data == data) {
        tmp = l->head;
        l->head = l->head->next;
        OBJECT_FREE(tmp);
        return 0;
    }
    return ll_remove(l->head, data);
}

void llist_iter_init(struct llist *l)
{
    l->iter = l->head;
}

// get the next data item in the list, or NULL
void *llist_iter_next(struct llist *l)
{
    void *data = NULL;
    if (l->iter == (struct llist_node *) -1) {
        printf("llist_iter_next: ERROR: concurrent modification\r\n");
        printf("\tHint: use llist_iter_init before iterating\r\n");
        return NULL;
    }
    if (l->iter) {
        // get data and advance the iterator
        data = l->iter->data;
        l->iter = l->iter->next;
    }
    return data;
}
