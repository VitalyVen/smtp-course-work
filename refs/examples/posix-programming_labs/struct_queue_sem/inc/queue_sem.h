#ifndef __QUEUE_SEM_H__
#define __QUEUE_SEM_H__

#include <assert.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "errors.h"

struct queue_sem {
    sem_t full;
    sem_t empty;
    ssize_t head;
    ssize_t tail;
    ssize_t size;
    void **q;
};

static inline
int init_queue_sem(struct queue_sem *queue, ssize_t size)
{
    assert(size > 0);
    queue->head = 0;
    queue->tail = 0;
    queue->size = size;
    if (sem_init(&queue->empty, 0, 0) == -1) {
        errnomsg("sem_init()");
        goto exit;
    }
    if (sem_init(&queue->full, 0, size) == -1) {
        errnomsg("sem_init()");
        goto free_empty;
    }
    queue->q = calloc(size, sizeof(queue->q[0]));
    if (!queue->q) {
        errnomsg("calloc()");
        goto free_full;
    }
    return 0;
free_full:
    if (sem_destroy(&queue->full))
        errnomsg("sem_destroy()");
free_empty:
    if (sem_destroy(&queue->empty))
        errnomsg("sem_destroy()");
exit:
    return -1;
}

static inline
void destroy_queue_sem(struct queue_sem *queue)
{
    queue->size = 0;
    if (sem_destroy(&queue->empty) == -1)
        errnomsg("sem_destroy()");
    if (sem_destroy(&queue->full) == -1)
        errnomsg("sem_destroy()");
    free(queue->q);
}

static inline
void queue_sem_push(struct queue_sem *queue, void *entry)
{
    while (sem_wait(&queue->full))
        CHECK_EINTR(continue, exit(EXIT_FAILURE), "sem_wait(full)");
    assert(queue->q[queue->head] == NULL);
    queue->q[queue->head] = entry;
    if (++queue->head == queue->size)
        queue->head = 0;
    fatal(sem_post(&queue->empty), "sem_post(empty)");
}

static inline
void *queue_sem_pop(struct queue_sem *queue)
{
    while (sem_wait(&queue->empty))
        CHECK_EINTR(continue, exit(EXIT_FAILURE), "sem_wait(empty)");
    void *entry = queue->q[queue->tail];
    queue->q[queue->tail] = NULL;
    if (++queue->tail == queue->size)
        queue->tail = 0;
    fatal(sem_post(&queue->full), "sem_post(full)");
    return entry;
}

#endif
