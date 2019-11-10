#ifndef __DB_SIMPLE_H__
#define __DB_SIMPLE_H__

// We will use red-black tree for our key-value "database".
#include "tree.h"

#include "db_virtual.h"
#include "misc_macros.h"

// Structure to store key-value datas.
// It uses red-black tree for better search performance.
struct db_node {
    RB_ENTRY(db_node) entry;
    char *key;
    char *value;
};

// Define 'struct db_head';
RB_HEAD(db_head, db_node);

// Key-value DB.
struct simple_db {
    struct vdb idb;
    struct db_head head;
};

// Initinalizes a new database.
struct vdb *init_simple_db(struct simple_db *db,
                           db_async_result_t async);

#endif
