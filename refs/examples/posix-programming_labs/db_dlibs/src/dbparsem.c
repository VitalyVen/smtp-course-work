#include <assert.h>
#include <stdlib.h>

#include "db_parallel.h"
#include "errors.h"
#include "req_queue_sem.h"

const char *name()
{
    return "Parallel DB (queue: semaphores)";
}

struct vdb *create_db(db_async_result_t async_result)
{
    struct parallel_db *db = malloc(sizeof(*db));
    if (!db) {
        errnomsg("malloc()");
        return NULL;
    }
    return init_parallel_db(db, create_sem_queue, async_result);
}
