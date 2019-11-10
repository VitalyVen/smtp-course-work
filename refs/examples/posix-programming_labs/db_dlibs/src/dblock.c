#include <assert.h>
#include <stdlib.h>

#include "db_lock.h"
#include "errors.h"

const char *name()
{
    return "Locking DB";
}

struct vdb *create_db(db_async_result_t async_result)
{
    struct lock_db *db = malloc(sizeof(*db));
    if (!db) {
        errnomsg("malloc()");
        return NULL;
    }
    return init_lock_db(db, async_result);
}
