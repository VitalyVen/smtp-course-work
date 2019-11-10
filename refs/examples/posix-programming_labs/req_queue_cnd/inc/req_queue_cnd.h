#ifndef __REQ_QUEUE_CND_H__
#define __REQ_QUEUE_CND_H__

#include <pthread.h>

#include "req_queue_virtual.h"
#include "req_queue.h"


struct cnd_db_queue {
    struct vqueue iqueue;
    struct requests_queue queue;
    struct requests_queue local_queue;
    int running;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
};

struct vqueue *init_cnd_db_queue(struct cnd_db_queue *queue);

struct vqueue *create_cnd_queue(void);

#endif
