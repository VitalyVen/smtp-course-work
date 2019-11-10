#ifndef __REQ_QUEUE_VIRTUAL_H__
#define __REQ_QUEUE_VIRTUAL_H__

#include "db_request.h"

// Virtual queue interface.
struct vqueue {
    void (*destroy)(struct vqueue *);
    void (*free)(struct vqueue *);
    struct db_request *(*new_request)(void);
    struct db_request *(*pop)(struct vqueue *);
    int (*push)(struct vqueue *, struct db_request *);
    void (*stop)(struct vqueue *);
};

typedef struct vqueue *(*create_queue_t)(void);

#endif
