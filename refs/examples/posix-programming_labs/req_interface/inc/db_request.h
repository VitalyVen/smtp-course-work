#ifndef __DB_REQUEST_H__
#define __DB_REQUEST_H__

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "db_virtual.h"
#include "errors.h"

struct db_request {
    void (*destroy)(struct db_request *request);
    void (*free)(struct db_request *req);
    // both set & get:
    char *key;
    // set only:
    char *value;
    // get only:
    db_async_result_t result_func;
    void *arg;
};

static inline
int init_set_request(struct db_request *request, char *key, char *value)
{
    request->key = strdup(key);
    if (!request->key) {
        errnomsg("strdup()");
        return -1;
    }
    if (value) {
        request->value = strdup(value);
    } else
        request->value = NULL;
    request->result_func = NULL;
    request->arg = NULL;
    return 0;
}

static inline
void init_get_request(struct db_request *request, char *key, db_async_result_t result_func, void *arg)
{
    assert(result_func != NULL);
    request->key = strdup(key);
    request->value = NULL;
    request->result_func = result_func;
    request->arg = arg;
}

static inline
void destroy_request(struct db_request *request)
{
    free(request->value);
    free(request->key);
}

#endif
