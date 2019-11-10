#ifndef __DB_VIRTUAL_H__
#define __DB_VIRTUAL_H__

typedef void (*db_async_result_t)(char *key, char *value, void *arg);

// Virtual database interface.
struct vdb {
    // Function pointers.
    // destroy() frees database resources.
    void (*destroy)(struct vdb *db);
    // db->free(db) should be used instead of free(db);
    void (*free)(struct vdb *db);
    // set() stores a copy of value and a copy of a key to the database.
    // It adds the key-value pair if a key was not found.
    // It remove the key from database if a value is NULL.
    int (*set)(struct vdb *db, char *key, char *value);
    // set_inst() stores a value itself to a database (not a copy of value).
    // It uses a key itself too (not a copy of a key).
    int (*set_inst)(struct vdb *db, char *key, char *value);
    // get() searches a database for a specified key.
    // It returns a copy of a found value or NULL
    // if the key does not exist in the database.
    char *(*get)(struct vdb *db, char *key);
    // TODO: comment
    int (*geta)(struct vdb *db, char *key, void *result_arg);
    // TODO: comment
    db_async_result_t async_result;
};

#endif
