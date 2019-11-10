#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <CUnit/Basic.h>

#include "db_virtual.h"
#include "misc_macros.h"

#define DB(vdb) containerof(vdb, struct dumb_db, idb)

struct dumb_db {
    struct vdb idb;
    char *value;
};

void destroy_db(struct vdb *vdb)
{
    free(DB(vdb)->value);
}

void free_db(struct vdb *vdb)
{
    free(DB(vdb));
}

char *db_get(struct vdb *vdb, char *key)
{
    struct dumb_db *db = DB(vdb);
    if (db->value) {
        char *copy = strdup(db->value);
        assert(copy);
        return copy;
    }
    return NULL;
}

int db_set(struct vdb *vdb, char *key, char *value)
{
    struct dumb_db *db = DB(vdb);
    free(db->value);
    if (value) {
        db->value = strdup(value);
        assert(db->value);
    } else
        db->value = NULL;
    return 0;
}

int db_set_inst(struct vdb *vdb, char *key, char *value)
{
    struct dumb_db *db = DB(vdb);
    free(db->value);
    if (value)
        db->value = value;
    else
        db->value = NULL;
    return 0;
}

int db_geta(struct vdb *vdb, char *key, void *arg)
{
    struct dumb_db *db = DB(vdb);
    char *value = NULL;
    if (db->value) {
        value = strdup(db->value);
        assert(value);
    }
    vdb->async_result(key, value, arg);
    return 0;
}

void async_result(char *key, char *value, void *arg)
{
    char *expected = arg;
    if (!expected) {
        CU_ASSERT_PTR_NULL(value)
    } else
        CU_ASSERT_STRING_EQUAL(value, expected);
    free(value);
}

void test_db(void)
{
    struct dumb_db dumb_db =  {
        .idb = {
            .destroy = destroy_db,
            .free = free_db,
            .get = db_get,
            .geta = db_geta,
            .set = db_set,
            .set_inst = db_set_inst,
            .async_result = async_result,
        },
        .value = NULL,
    };
    struct vdb *db = &dumb_db.idb;
    char *r;
    CU_ASSERT_PTR_EQUAL(db->get(db, NULL), NULL);
    db->set(db, NULL, "first");
    r = db->get(db, NULL);
    CU_ASSERT_STRING_EQUAL(r, "first");
    free(r);
    db->set_inst(db, NULL, strdup("second"));
    r = db->get(db, NULL);
    CU_ASSERT_STRING_EQUAL(r, "second");
    free(r);
    db->set(db, NULL, NULL);
    r = db->get(db, NULL);
    CU_ASSERT_PTR_EQUAL(r, NULL);
    free(r);
    db->set(db, NULL, "first");
    db->geta(db, NULL, "first");
    db->set(db, NULL, "second");
    db->geta(db, NULL, "second");
    db->set(db, NULL, NULL);
    db->geta(db, NULL, NULL);
    db->destroy(db);
}

void test_free(void)
{
    struct dumb_db *dumb_db = calloc(1, sizeof(*dumb_db));
    assert(dumb_db);
    dumb_db->idb.free = free_db;
    struct vdb *db = &dumb_db->idb;
    db->free(db);
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("Interfaces", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "virtual database", test_db))
        goto cleanup;
    if (!CU_add_test(suite, "free", test_free))
        goto cleanup;
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    if (CU_get_failure_list())
        r = EXIT_FAILURE;
cleanup:
    CU_cleanup_registry();
exit:
    if (CU_get_error())
        return CU_get_error();
    else
        return r;
}
