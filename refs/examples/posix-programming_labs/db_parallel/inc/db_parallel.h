#ifndef __DB_PARALLEL_H__
#define __DB_PARALLEL_H__

#include <pthread.h>

#include "db_simple.h"
#include "db_virtual.h"
#include "req_queue_virtual.h"

struct thread_data {
    struct vdb *db;
    struct vqueue *requests;
};

#define N_SUBS 0x0ff

// #pragma pack(0)
struct parallel_db {
    struct vdb idb;
    pthread_t threads[N_SUBS];
    struct thread_data thread_data[N_SUBS];
    struct simple_db subs[N_SUBS];
};

// Initinalizes a new database.
struct vdb *init_parallel_db(struct parallel_db *db,
                              create_queue_t create_queue,
                              db_async_result_t async_result);

#endif
