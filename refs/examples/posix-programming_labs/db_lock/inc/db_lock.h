#ifndef __DB_LOCK_H__
#define __DB_LOCK_H__

#include <pthread.h>

#include "db_simple.h"
#include "db_virtual.h"

#define N_SUBS 0x0ff

struct lock_db {
    struct vdb idb;
    struct simple_db subs[N_SUBS];
    pthread_rwlock_t locks[N_SUBS]; // Read-write locks.
};

// Initinalizes a new database.
struct vdb *init_lock_db(struct lock_db *db,
                                db_async_result_t async);

#endif
