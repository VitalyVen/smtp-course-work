#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "db_simple.h"
#include "errors.h"

static int db_cmp(struct db_node *e1, struct db_node *e2)
{
    return strcmp(e1->key, e2->key);
}

// Gerenarate insert/delete/add functions.
RB_GENERATE(db_head, db_node, entry, db_cmp);

static void free_node(struct db_node *node)
{
    free(node->key);
    free(node->value);
    free(node);
}

#define DB(vdb) containerof(vdb, struct simple_db, idb)

static void destroy(struct vdb *vdb)
{
    struct simple_db *db = DB(vdb);
    struct db_node *node, *next;
    for (node = RB_MIN(db_head, &db->head); node != NULL;) {
        next = RB_NEXT(db_head, &db->head, node);
        RB_REMOVE(db_head, &db->head, node);
        free_node(node);
        node = next;
    }
}

static void free_db(struct vdb *vdb)
{
    free(DB(vdb));
}

static char *get(struct vdb *vdb, char *key)
{
    struct simple_db *db = DB(vdb);
    struct db_node search = {.key = key};
    struct db_node *found = RB_FIND(db_head, &db->head, &search);
    if (found) {
        char *result = strdup(found->value);
        if (!result)
            errnomsg("strdup()");
        return result;
    }
    return NULL;
}

static int geta(struct vdb *vdb, char *key, void *result_arg)
{
    char *value = get(vdb, key);
    assert(vdb->async_result);
    // TODO: Is strdup() kinda bad here? No.
    char *key_copy = strdup(key);
    if (!key_copy) {
        errnomsg("strdup()");
        free(value);
        return -1;
    }
    vdb->async_result(key_copy, value, result_arg);
    // Note: key_copy and value are freed by async result.
    return 0;
}

static void drop_node(struct simple_db *db, char *key)
{
    struct db_node search = {.key = key};
    struct db_node *found = RB_FIND(db_head, &db->head, &search);
    if (found) {
        RB_REMOVE(db_head, &db->head, found);
        free_node(found);
    }
}

static int set(struct vdb *vdb, char *key, char *value)
{
    struct simple_db *db = DB(vdb);
    if (!value) { // Remove the key from the database.
        drop_node(db, key);
        return 0;
    }
    struct db_node *node = calloc(1, sizeof(*node));
    if (!node) {
        errnomsg("calloc()");
        goto error;
    }
    node->key = strdup(key);
    if (!node->key) {
        errnomsg("strdup()");
        goto free_node;
    }
    struct db_node *collision = RB_INSERT(db_head, &db->head, node);
    if (collision) { // Collision detected. Update existing key.
        free_node(node);
        free(collision->value); // Free old value.
        if (!(collision->value = strdup(value))) {
            errnomsg("strdup()");
            goto error;
        }
    } else // New node was inserted.
        if (!(node->value = strdup(value))) {
            errnomsg("strdup()");
            goto free_node;
        }
    return 0;
free_node:
    free_node(node);
error:
    return -1;
}

static int set_inst(struct vdb *vdb, char *key, char *value)
{
    struct simple_db *db = DB(vdb);
    if (!value) { // Remove the key from the database.
        drop_node(db, key);
        free(key);
        return 0;
    }
    struct db_node *node = calloc(1, sizeof(*node));
    if (!node) {
        errnomsg("calloc()");
        return -1;
    }
    node->key = key;
    struct db_node *collision  = RB_INSERT(db_head, &db->head, node);
    if (collision) { // Collision detected. Update existing key.
        free_node(node);
        free(collision->value); // Free old value.
        collision->value = value;
    } else // New node was inserted.
        node->value = value;
    return 0;
}

struct vdb *init_simple_db(struct simple_db *db, db_async_result_t async)
{
    RB_INIT(&db->head);
    db->idb.destroy = destroy;
    db->idb.free = free_db;
    db->idb.get = get;
    db->idb.set = set;
    db->idb.set_inst = set_inst;
    if (async)
        db->idb.geta = geta;
    else
        db->idb.geta = NULL;
    db->idb.async_result = async;
    return &db->idb;
}
