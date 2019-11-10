#include <assert.h>

#include "async_arg.h"
#include "cnct.h"
#include "errors.h"

#define BUF_SIZE (128 * 1024)

int init_fd_cnct(struct cnct *cnct, int fd)
{
    cnct->async_arg.before_async_get = (async_notify_t) inc_queue_cnt;
    int res;
    if ((res = pthread_mutex_init(&cnct->lock, NULL))) {
        errmsg(res, "pthread_mutex_init()");
        return -1;
    }
    if (init_buffer(&cnct->buf, BUF_SIZE)) {
        if ((res = pthread_mutex_destroy(&cnct->lock)))
            errmsg(res, "pthread_mutex_destroy()");
        return -1;
    }
    cnct->pollfd = NULL;
    reset_fd_cnct(cnct, fd);
    return 0;
}

int init_poll_cnct(struct cnct *cnct, struct pollfd *pollfd)
{
    if (init_fd_cnct(cnct, pollfd->fd))
        return -1;
    cnct->fd = -1;
    cnct->pollfd = pollfd;
    return 0;
}

void reset_fd_cnct(struct cnct *cnct, int fd)
{
    assert(!cnct->pollfd);
    cnct->fd = fd;
    cnct->queue_cnt = 0;
    cnct->state = fd < 0 ? CNCT_SHUT : CNCT_OPEN;
    reset_buffer(&cnct->buf);
}

void reset_poll_cnct(struct cnct *cnct)
{
    cnct->queue_cnt = 0;
    cnct->state = cnct->pollfd->fd < 0 ? CNCT_SHUT : CNCT_OPEN;
    reset_buffer(&cnct->buf);
}

void destroy_cnct(struct cnct *cnct)
{
    assert(cnct->queue_cnt == 0);
    destroy_buffer(&cnct->buf);
    int res;
    if ((res = pthread_mutex_destroy(&cnct->lock)))
        errmsg(res, "pthread_mutex_destroy()");
}

void inc_queue_cnt(struct cnct *cnct)
{
    fatal(pthread_mutex_lock(&cnct->lock), "pthread_mutex_lock()");
    assert(cnct->state == CNCT_OPEN);
    cnct->queue_cnt++;
    fatal(pthread_mutex_unlock(&cnct->lock), "pthread_mutex_unlock()");
}

void dec_queue_cnt(struct cnct *cnct)
{
    fatal(pthread_mutex_lock(&cnct->lock), "pthread_mutex_lock()");
    cnct->queue_cnt--;
    assert(cnct->queue_cnt >= 0);
    if (cnct->state == CNCT_WAIT && cnct->queue_cnt == 0)
        cnct->state = CNCT_SHUT;
    fatal(pthread_mutex_unlock(&cnct->lock), "pthread_mutex_unlock()");
}

int is_cnct_queue_empty(struct cnct *cnct)
{
    fatal(pthread_mutex_lock(&cnct->lock), "pthread_mutex_lock()");
    int r = cnct->queue_cnt == 0;
    fatal(pthread_mutex_unlock(&cnct->lock), "pthread_mutex_unlock()");
    return r;
}

cnct_state_t cnct_state(struct cnct *cnct)
{
    fatal(pthread_mutex_lock(&cnct->lock), "pthread_mutex_lock()");
    cnct_state_t s = cnct->state;
    fatal(pthread_mutex_unlock(&cnct->lock), "pthread_mutex_unlock()");
    return s;
}

cnct_state_t shutdown_cnct(struct cnct *cnct)
{
    cnct_state_t s;
    fatal(pthread_mutex_lock(&cnct->lock), "pthread_mutex_lock()");
    if (cnct->queue_cnt == 0)
        cnct->state = (s = CNCT_SHUT);
    else
        cnct->state = (s = CNCT_WAIT);
    fatal(pthread_mutex_unlock(&cnct->lock), "pthread_mutex_unlock()");
    return s;
}
