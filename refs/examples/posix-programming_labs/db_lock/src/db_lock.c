#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"
#include "db_lock.h"
#include "string_hash.h"
#include "misc_macros.h"

#define DB(vdb) containerof(vdb, struct lock_db, idb)

static
void destroy(struct vdb *vdb)
{
    struct lock_db *db = DB(vdb);
    for (int i = 0; i < N_SUBS; i++) {
        struct vdb *sub = &db->subs[i].idb;
        sub->destroy(sub);
        int res;
        if ((res = pthread_rwlock_destroy(db->locks + i)))
            errmsg(res, "pthread_rwlock_destroy()");
    }
}

static
void free_db(struct vdb *vdb)
{
    free(DB(vdb));
}

static
char *get(struct vdb *vdb, char *key)
{
    struct lock_db *db = DB(vdb);
    int t = string_hash(key);
    assert(t >= 0 && t < N_SUBS);
    fatal(pthread_rwlock_rdlock(db->locks + t),
          "pthread_rwlock_rdlock()");
    struct vdb *sub = &db->subs[t].idb;
    char *found = sub->get(sub, key);
    fatal(pthread_rwlock_unlock(db->locks + t),
          "pthread_rwlock_unlock()");
    return found;
}

static
int set(struct vdb *vdb, char *key, char *value)
{
    struct lock_db *db = DB(vdb);
    int t = string_hash(key);
    assert(t >= 0 && t < N_SUBS);
    fatal(pthread_rwlock_wrlock(db->locks + t),
          "pthread_rwlock_lock()");
    struct vdb *sub = &db->subs[t].idb;
    sub->set(sub, key, value);
    fatal(pthread_rwlock_unlock(db->locks + t),
          "pthread_rwlock_unlock()");
    return 0;
}

static
int geta(struct vdb *vdb, char *key, void *arg)
{
    char *value = get(vdb, key);
    assert(vdb->async_result);
    char *key_copy = strdup(key);
    if (!key_copy) {
        errnomsg("strdup()");
        free(value);
        return -1;
    }
    vdb->async_result(key_copy, value, arg);
    return 0;
}

struct vdb *init_lock_db(struct lock_db *db, db_async_result_t async)
{
    int nsubs = 0;
    int nlocks = 0;
    for (nsubs = 0; nsubs < N_SUBS; nsubs++)
        if (!init_simple_db(db->subs + nsubs, NULL))
            goto destroy_dbs;
    for (nlocks = 0; nlocks < N_SUBS; nlocks++) {
        int res = pthread_rwlock_init(db->locks + nlocks, NULL);
        if (res) {
            errmsg(res, "pthread_rwlock_init()");
            goto destroy_locks;
        }
    }
    db->idb.destroy = destroy;
    db->idb.free = free_db;
    db->idb.get = get;
    db->idb.set = set;
    if (async)
        db->idb.geta = geta;
    else
        db->idb.geta = NULL;
    db->idb.async_result = async;
    return &db->idb;
destroy_locks:
    for (int i = 0; i < nlocks; i++) {
        int res;
        if ((res = pthread_rwlock_destroy(db->locks + i)))
            errmsg(res, "pthread_rwlock_destroy()");
    }
destroy_dbs:
    for (int i = 0; i < nsubs; i++) {
        struct vdb *sub = &db->subs[i].idb;
        sub->destroy(sub);
    }
    return NULL;
}
