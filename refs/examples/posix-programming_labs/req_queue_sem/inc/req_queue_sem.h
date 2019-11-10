#ifndef __REQ_QUEUE_SEM_H__
#define __REQ_QUEUE_SEM_H__

#include "req_queue_virtual.h"
#include "queue_sem.h"

struct sem_db_queue {
    struct vqueue iqueue;
    struct queue_sem queue;
};

struct vqueue *init_sem_db_queue(struct sem_db_queue *requests,
                                 ssize_t size);

struct vqueue *create_sem_queue(void);

#endif
