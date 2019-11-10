#ifndef __CNCT_H__
#define __CNCT_H__

#include <poll.h>
#include <string.h>

#include <pthread.h>

#include "async_arg.h"
#include "buffer.h"

enum cnct_state {
    CNCT_SHUT, CNCT_OPEN, CNCT_WAIT
};

typedef enum cnct_state cnct_state_t;

struct cnct {
    struct async_arg async_arg;
    int fd;
    struct pollfd *pollfd;
    struct buffer buf;
    int queue_cnt;
    cnct_state_t state;
    pthread_mutex_t lock;
};

static inline
int cnct_fd(struct cnct *cnct)
{
    if (cnct->pollfd)
        return cnct->pollfd->fd;
    return cnct->fd;
}

int init_fd_cnct(struct cnct *cnct, int fd);
int init_poll_cnct(struct cnct *cnct, struct pollfd *pollfd);

void reset_fd_cnct(struct cnct *cnct, int fd);
void reset_poll_cnct(struct cnct *cnct);

void destroy_cnct(struct cnct *cnct);

void inc_queue_cnt(struct cnct *cnct);
void dec_queue_cnt(struct cnct *cnct);
int is_cnct_queue_empty(struct cnct *cnct);

cnct_state_t cnct_state(struct cnct *cnct);
cnct_state_t shutdown_cnct(struct cnct *cnct);

#endif
