#include <stdio.h>

#include "errors.h"
#include "misc_macros.h"
#include "req_queue_cnd.h"

#define QUEUE(vqueue) \
    containerof(vqueue, struct cnd_db_queue, iqueue)

#define REQUEST(vrequest) \
    containerof(vrequest, struct queued_request, ireq)

static void destroy(struct vqueue *vqueue)
{
    struct cnd_db_queue *queue = QUEUE(vqueue);
    destroy_requests_queue(&queue->queue);
    destroy_requests_queue(&queue->local_queue);
    int res;
    if ((res = pthread_mutex_destroy(&queue->mutex)))
        errmsg(res, "pthread_mutex_destroy()");
    if ((res = pthread_cond_destroy(&queue->condition)))
        errmsg(res, "pthread_cond_destroy()");
}

static void free_queue(struct vqueue *vqueue)
{
    free(QUEUE(vqueue));
}

static void stop(struct vqueue *vqueue)
{
    struct cnd_db_queue *queue = QUEUE(vqueue);
    fatal(pthread_mutex_lock(&queue->mutex),
          "pthread_mutex_lock()");
    queue->running = 0;
    // pthread_cond_signal() never returns an error code.
    pthread_cond_signal(&queue->condition);
    fatal(pthread_mutex_unlock(&queue->mutex),
          "pthread_mutex_unlock()");
}

static struct db_request *pop(struct vqueue *vqueue)
{
    struct cnd_db_queue *queue = QUEUE(vqueue);
    struct requests_queue *local = &queue->local_queue;
    int running = 1;
    while (1) {
        if (!no_requests(local))
            return (struct db_request *) unqueue_request(local);
        if (!running && no_requests(&queue->queue))
            return NULL;
        pthread_mutex_lock(&queue->mutex);
        running = queue->running;
        if (running && no_requests(&queue->queue))
            pthread_cond_wait(&queue->condition, &queue->mutex);
        while (!no_requests(&queue->queue))
            queue_request(local, unqueue_request(&queue->queue));
        pthread_mutex_unlock(&queue->mutex);
    }
}

static int push(struct vqueue *vqueue, struct db_request *request)
{
    struct cnd_db_queue *queue = QUEUE(vqueue);
    fatal(pthread_mutex_lock(&queue->mutex),
          "pthread_mutex_lock()");
    queue_request(&queue->queue, REQUEST(request));
    pthread_cond_signal(&queue->condition);
    fatal(pthread_mutex_unlock(&queue->mutex),
          "pthread_mutex_unlock()");
    return 0;
}

struct vqueue *init_cnd_db_queue(struct cnd_db_queue *queue)
{
    queue->iqueue.destroy = destroy;
    queue->iqueue.free = free_queue;
    queue->iqueue.push = push;
    queue->iqueue.pop = pop;
    queue->iqueue.stop = stop;
    queue->iqueue.new_request = new_cnd_request;
    init_requests_queue(&queue->queue);
    init_requests_queue(&queue->local_queue);
    queue->running = 1;
    int res;
    if ((res = pthread_mutex_init(&queue->mutex, NULL))) {
        errmsg(res, "pthread_mutex_init()");
        return NULL;
    }
    if ((res = pthread_cond_init(&queue->condition, NULL))) {
        errmsg(res, "pthread_mutex_init()");
        if ((res = pthread_mutex_destroy(&queue->mutex)))
            errmsg(res, "pthread_mutex_destroy()");
        return NULL;
    }
    return &queue->iqueue;
}

struct vqueue *create_cnd_queue(void)
{
    struct cnd_db_queue *queue = malloc(sizeof(*queue));
    if (!queue) {
        errnomsg("malloc()");
        return NULL;
    }
    return init_cnd_db_queue(queue);
}

