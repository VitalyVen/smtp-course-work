#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"
#include "db_simple.h"
#include "db_parallel.h"
#include "misc_macros.h"
#include "string_hash.h"

static void *db_thread(void *arg)
{
    struct thread_data *data = arg;
    struct vqueue *requests = data->requests;
    struct vdb *db = data->db;
    while (1) {
        struct db_request *request = requests->pop(requests);
        if (!request)
            break;
        if (request->result_func) { // get
            //TODO: Q. value is a copy. Is it a pity?
            // (A: not in poll server case)
            char *value = db->get(db, request->key);
            request->result_func(request->key, value, request->arg);
            // request->key and value are freed by result_func!
        } else  // set:
            // request->free and value are stored in db!
            db->set_inst(db, request->key, request->value);
        request->free(request);
    }
    pthread_exit(NULL);
    return NULL; // non-reaches
}

#define DB(vdb) containerof(vdb, struct parallel_db, idb)

static void destroy(struct vdb *vdb)
{
    struct parallel_db *db = DB(vdb);
    for (int i = 0; i < N_SUBS; i++) {
        struct vqueue *requests = db->thread_data[i].requests;
        requests->stop(requests);
        int res;
        if ((res = pthread_join(db->threads[i], NULL)))
            errmsg(res, "pthread_join()");
    }
    for (int i = 0; i < N_SUBS; i++) {
        struct vdb *sdb = db->thread_data[i].db;
        sdb->destroy(sdb);
        struct vqueue *requests = db->thread_data[i].requests;
        requests->destroy(requests);
        requests->free(requests);
    }
}

static void free_db(struct vdb *vdb)
{
    free(DB(vdb));
}

static inline 
struct vqueue *get_queue(struct parallel_db *db, char *key)
{
    int t = string_hash(key);
    assert(t >= 0 && t < N_SUBS);
    struct vqueue *q = db->thread_data[t].requests;
    return q;
}

static int geta(struct vdb *vdb, char *key, void *arg)
{
    struct parallel_db *db = DB(vdb);
    assert(vdb->async_result);
    struct vqueue *requests = get_queue(db, key);
    struct db_request *request = requests->new_request();
    if (!request)
        return -1;
    init_get_request(request, key, vdb->async_result, arg);
    requests->push(requests, request);
    return 0;
}

static int set(struct vdb *vdb, char *key, char *value)
{
    struct parallel_db *db = DB(vdb);
    struct vqueue *requests = get_queue(db, key);
    assert(requests);
    struct db_request *request = requests->new_request();
    if (!request)
        return -1;
    init_set_request(request, key, value);
    requests->push(requests, request);
    return 0;
}

struct vdb *init_parallel_db(struct parallel_db *db,
                 create_queue_t create_queue,
                 db_async_result_t async)
{
    int res;
    int ndbs = 0;
    int nreqs = 0;
    int ntrds = 0;
    for (ndbs = 0; ndbs < N_SUBS; ndbs++)
        if (!(db->thread_data[ndbs].db =
              init_simple_db(db->subs + ndbs, NULL)))
            goto destroy_dbs;
    for (nreqs = 0; nreqs < N_SUBS; nreqs++)
        if (!(db->thread_data[nreqs].requests = create_queue()))
            goto destroy_reqs;
    for (ntrds = 0; ntrds < N_SUBS; ntrds++)
        if ((res = pthread_create(db->threads + ntrds, NULL,
                                  db_thread, db->thread_data + ntrds))) {
              errmsg(res, "pthread_create()");
              goto destroy_trds;
        }
    db->idb.destroy = destroy;
    db->idb.free = free_db;
    db->idb.geta = geta;
    db->idb.get = NULL;
    db->idb.set = set;
    //TODO: Q. Why?
    db->idb.set_inst = NULL;
    db->idb.async_result = async;
    return &db->idb;
destroy_trds:
    for (int i = 0; i < ntrds; i++) {
        struct vqueue *requests = db->thread_data[i].requests;
        requests->stop(requests);
        int res;
        if ((res = pthread_join(db->threads[i], NULL)))
            errmsg(res, "pthread_join()");
    }
destroy_reqs:
    for (int i = 0; i < nreqs; i++) {
        struct vqueue *requests = db->thread_data[i].requests;
        requests->destroy(requests);
        requests->free(requests);
    }
destroy_dbs:
    for (int i = 0; i < ndbs; i++) {
        struct vdb *sdb = db->thread_data[i].db;
        sdb->destroy(sdb);
    }
    return NULL;
}
