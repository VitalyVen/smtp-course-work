#ifndef __WRBUFFER_H__
#define __WRBUFFER_H__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "errors.h"
#include "queue.h"

struct queued_string {
    char *string;
    STAILQ_ENTRY(queued_string) link;
};

STAILQ_HEAD(string_queue, queued_string);

struct wrbuffer {
    struct string_queue queue;
    ssize_t pos;
};

static inline
void init_wrbuffer(struct wrbuffer *buf)
{
    buf->pos = 0;
    STAILQ_INIT(&buf->queue);
}

static inline
void reset_wrbuffer(struct wrbuffer *buf)
{
    while (!STAILQ_EMPTY(&buf->queue)) {
        struct queued_string *cur = STAILQ_FIRST(&buf->queue);
        STAILQ_REMOVE_HEAD(&buf->queue, link);
        free(cur->string);
        free(cur);
    }
    buf->pos = 0;
}

static inline
void destroy_wrbuffer(struct wrbuffer *buf)
{
    reset_wrbuffer(buf);
}

static inline
int wrbuffer_add(struct wrbuffer *buf, char *string)
{
    struct queued_string *q = malloc(sizeof(*q));
    if (!q) {
        errnomsg("malloc()");
        return -1;
    }
    q->string = string;
    STAILQ_INSERT_TAIL(&buf->queue, q, link);
    return 0;
}

static inline
int is_wrbuffer_empty(struct wrbuffer *buf)
{
    return STAILQ_EMPTY(&buf->queue);
}

static inline
struct queued_string *wrbuffer_head(struct wrbuffer *buf)
{
    return STAILQ_FIRST(&buf->queue);
}

static inline
char *wrbuffer_start(struct wrbuffer *buf)
{
    struct queued_string *head = wrbuffer_head(buf);
    assert(head);
    return head->string + buf->pos;
}

static inline
void wrbuffer_shift(struct wrbuffer *buf, ssize_t len)
{
    struct queued_string *head = wrbuffer_head(buf);
    assert(head);
    ssize_t nremains = strlen(head->string) - buf->pos;
    assert(nremains >= len);
    if (nremains > len)
        buf->pos += len;
    else {
        buf->pos = 0;
        STAILQ_REMOVE_HEAD(&buf->queue, link);
        free(head->string);
        free(head);
    }
}

static inline
ssize_t wrbuffer_remained(struct wrbuffer *buf)
{
    struct queued_string *head = wrbuffer_head(buf);
    if (!head)
        return 0;
    ssize_t slen = strlen(head->string);
    assert(buf->pos < slen);
    return slen - buf->pos;
}

#endif
