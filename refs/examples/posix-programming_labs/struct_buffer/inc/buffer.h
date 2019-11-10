#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"

struct buffer {
    char *buf;
    size_t size;
    size_t pos;
};

static inline
int init_buffer(struct buffer *buf, size_t size)
{
    buf->pos = 0;
    buf->buf = malloc(size);
    if (!buf->buf) {
        errnomsg("malloc()");
        buf->size = 0;
        return -1;
    }
    buf->size = size;
    return 0;
}

static inline
void destroy_buffer(struct buffer *buf)
{
    free(buf->buf);
    buf->size = 0;
}

static inline
void reset_buffer(struct buffer *buf)
{
    buf->pos = 0;
}

static inline
char *buffer_start(struct buffer *buf)
{
    assert(buf->buf);
    return buf->buf + buf->pos;
}

static inline
size_t buffer_space(struct buffer *buf)
{
    assert(buf->size >= buf->pos);
    return buf->size - buf->pos;
}

static inline
ssize_t buffer_shift(struct buffer *buf, size_t shift)
{
    if (buf->size < buf->pos + shift)
        return -1;
    return buf->pos += shift;
}

static inline
char *buffer_cut(struct buffer *buf, size_t len)
{
    if (buf->pos < len)
        return NULL;
    char *cut = malloc(len + 1);
    if (!cut) {
        errnomsg("malloc()");
        return NULL;
    }
    memcpy(cut, buf->buf, len);
    cut[len] = '\0';
    memmove(buf->buf, buf->buf + len, buf->pos - len);
    buf->pos -= len;
    return cut;
}

static inline
void buffer_clean(struct buffer *buf, size_t len)
{
    assert(buf->pos >= len);
    if (buf->pos < len) // FIXME: MIN?
        len = buf->pos;
    memmove(buf->buf, buf->buf + len, buf->pos - len);
    buf->pos -= len;
}

static inline
int is_buffer_empty(struct buffer *buf)
{
    return buf->pos == 0;
}

static inline
ssize_t buffer_find(struct buffer *buf, char *string)
{
    size_t l = strlen(string);
    if (l > buf->pos)
        return -1;
    // Buffer is not null-terminated so we cannot use strstr().
    for (size_t i = 0; i < buf->pos - l + 1; i++) {
        size_t j = 0;
        while (j < l && buf->buf[i + j] == string[j])
            j++;
        if (j == l)
            return i;
    }
    return -1;
}

#endif
