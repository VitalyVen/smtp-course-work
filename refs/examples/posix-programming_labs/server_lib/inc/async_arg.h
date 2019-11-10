#ifndef __ASYNC_ARG_H__
#define __ASYNC_ARG_H__

struct async_arg;

typedef void (*async_notify_t)(struct async_arg *self);

struct async_arg {
    async_notify_t before_async_get;
};

#endif
