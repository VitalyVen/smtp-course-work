#include <stdio.h>

#include "errors.h"
#include "misc_macros.h"
#include "req_queue_sem.h"

#define QUEUE(vqueue) \
    containerof(vqueue, struct sem_db_queue, iqueue)

static void destroy(struct vqueue *vqueue)
{
    destroy_queue_sem(&(QUEUE(vqueue)->queue));
}

static void free_queue(struct vqueue *vqueue)
{
    free(QUEUE(vqueue));
}

static void stop(struct vqueue *vqueue)
{
    queue_sem_push(&(QUEUE(vqueue)->queue), NULL);
}

static int push(struct vqueue *vqueue, struct db_request *request)
{
    queue_sem_push(&(QUEUE(vqueue)->queue), request);
    return 0;
}

static struct db_request *pop(struct vqueue *vqueue)
{
    return queue_sem_pop(&(QUEUE(vqueue)->queue));
}

static struct db_request *new_request(void)
{
    struct db_request *request = malloc(sizeof(*request));
    if (!request) {
        errnomsg("malloc()");
        return NULL;
    }
    // We can use usual free() function.
    request->free = (void (*)(struct db_request *)) free;
    request->destroy = destroy_request;
    return request;
}

struct vqueue *init_sem_db_queue(struct sem_db_queue *queue, 
                                 ssize_t size)
{
    if (init_queue_sem(&queue->queue, size) == -1)
        return NULL;
    queue->iqueue.destroy = destroy;
    queue->iqueue.free = free_queue;
    queue->iqueue.push = push;
    queue->iqueue.pop = pop;
    queue->iqueue.stop = stop;
    queue->iqueue.new_request = new_request;
    return &queue->iqueue;
}

#define DEFAULT_QUEUE_SIZE 0x0100

struct vqueue *create_sem_queue(void)
{
    struct sem_db_queue *queue = malloc(sizeof(*queue));
    if (!queue) {
        errnomsg("malloc()");
        return NULL;
    }
    return init_sem_db_queue(queue, DEFAULT_QUEUE_SIZE);
}
