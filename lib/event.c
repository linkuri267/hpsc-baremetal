#include "console.h"
#include "panic.h"

#include "event.h"

void ev_loop_init(struct ev_loop *el, const char *name)
{
    ASSERT(el);
    DPRINTF("EV LOOP %s: init\r\n", name);
    el->name = name;
    el->evq_head = el->evq_tail = 0;
}

int ev_loop_process(struct ev_loop *el)
{
    if (el->evq_head != el->evq_tail) {
        el->evq_tail = (el->evq_tail + 1) % EV_QUEUE_LEN;
        struct ev_slot *evs = &el->evq[el->evq_head];
        printf("EV LOOP %s: dequeue (tail %u head %u)\r\n", el->name,
               el->evq_tail, el->evq_head);
        ASSERT(evs->actor && evs->actor->func);
        evs->actor->func(evs->actor->state, evs->event);
        return 0;
    }
    return 1;
}

int ev_loop_post(struct ev_loop *el, struct ev_actor *actor, void *event)
{
    ASSERT(el && actor);
    if (el->evq_head + 1 % EV_QUEUE_LEN == el->evq_tail) {
        printf("EVENT: post failed: queue full\r\n");
        return 1;
    }
    el->evq_head = (el->evq_head + 1) % EV_QUEUE_LEN;
    struct ev_slot *evs = &el->evq[el->evq_head];
    evs->actor = actor;
    evs->event = event;
    printf("EVENT: posted (tail %u head %u)\r\n", el->evq_tail, el->evq_head);

    /* TODO: SEV (when mail loop moves to WFE instead of WFI) */
    return 0;
}

bool ev_loop_pending(struct ev_loop *el)
{
    return !(el->evq_head == el->evq_tail);
}
