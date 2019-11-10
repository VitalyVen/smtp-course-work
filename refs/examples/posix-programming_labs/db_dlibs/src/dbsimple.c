#include <assert.h>
#include <stdlib.h>

#include "db_simple.h"
#include "errors.h"

const char *name()
{
    return "Simple DB";
}

struct vdb *create_db(db_async_result_t async_result)
{
    struct simple_db *db = malloc(sizeof(*db));
    if (!db) {
        errnomsg("malloc()");
        return NULL;
    }
    return init_simple_db(db, async_result);
}
