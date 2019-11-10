#ifndef __REQ_CND_H__
#define __REQ_CND_H__

#include "queue.h"

#include "db_request.h"

struct queued_request {
    struct db_request ireq;
    STAILQ_ENTRY(queued_request) link;
};

STAILQ_HEAD(requests_queue, queued_request);

struct db_request *new_cnd_request(void);

#endif
