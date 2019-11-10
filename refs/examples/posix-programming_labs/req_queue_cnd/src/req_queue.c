#include "req_queue.h"

void init_requests_queue(struct requests_queue *queue)
{
    STAILQ_INIT(queue);
}

void destroy_requests_queue(struct requests_queue *queue)
{
    struct queued_request *cur;
    while ((cur = STAILQ_FIRST(queue))) {
        STAILQ_REMOVE_HEAD(queue, link);
        free(cur);
    }
}

void queue_request(struct requests_queue *queue, struct queued_request *request)
{
    STAILQ_INSERT_TAIL(queue, request, link);
}

struct queued_request *unqueue_request(struct requests_queue *queue)
{
    struct queued_request *request = STAILQ_FIRST(queue);
    STAILQ_REMOVE_HEAD(queue, link);
    return request;
}

int no_requests(struct requests_queue *queue)
{
    return STAILQ_EMPTY(queue);
}
