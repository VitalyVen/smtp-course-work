#include <stdlib.h>

#include "errors.h"
#include "misc_macros.h"
#include "req_cnd.h"

static void free_request(struct db_request *vreq)
{
    free(containerof(vreq, struct queued_request, ireq));
}

struct db_request *new_cnd_request(void)
{
    struct queued_request *request = malloc(sizeof(*request));
    if (!request) {
        errnomsg("malloc()");
        return NULL;
    }
    request->ireq.free = free_request;
    request->ireq.destroy = destroy_request;
    return (struct db_request *) request;
}
