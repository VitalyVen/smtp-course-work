#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <CUnit/Basic.h>

#include "db_simple.h"

#define ASSERT_DB(key, value) do  { \
    char *v = value; \
    char *r = db->get(db, key); \
    if (!value) \
        CU_ASSERT_PTR_NULL(r)  \
    else { \
        CU_ASSERT_PTR_NOT_NULL(r);  \
        CU_ASSERT_STRING_EQUAL(r, v); \
    }; \
    free(r); \
} while (0)

int async_result;

void async_get_result(char *key, char *value, void *arg)
{
    async_result++;
    CU_ASSERT_STRING_EQUAL(value, arg);
    free(key);
    free(value);
}

void test_db(void)
{
    async_result = 0;
    struct simple_db *simple_db = malloc(sizeof(*simple_db));
    assert(simple_db);
    struct vdb *db = init_simple_db(simple_db, async_get_result);
    db->set(db, "1", "first");
    ASSERT_DB("1", "first");
    db->set(db, "2", "second");
    ASSERT_DB("1", "first");
    db->geta(db, "1", "first");
    ASSERT_DB("2", "second");
    db->geta(db, "2", "second");
    db->set_inst(db, strdup("1"), strdup("prime"));
    ASSERT_DB("1", "prime");
    db->set(db, "3", "third");
    db->set(db, "1", "primus one");
    ASSERT_DB("3", "third");
    ASSERT_DB("1", "primus one");
    db->set(db, "3", "3");
    db->set(db, "1", "1");
    db->set(db, "2", "2");
    ASSERT_DB("1", "1");
    ASSERT_DB("2", "2");
    ASSERT_DB("3", "3");
    db->set(db, "2", NULL);
    db->set(db, "1", NULL);
    db->set(db, "3", NULL);
    ASSERT_DB("1", NULL);
    ASSERT_DB("2", NULL);
    ASSERT_DB("3", NULL);
    CU_ASSERT_PTR_NULL(simple_db->head.rbh_root);
    db->set(db, "1", "A");
    db->set(db, "2", "BB");
    db->set(db, "3", "CCC");
    ASSERT_DB("1", "A");
    ASSERT_DB("2", "BB");
    ASSERT_DB("3", "CCC");
    db->set(db, "1", "AAAA");
    db->set(db, "2", "BBBBBB");
    db->set(db, "3", "CCCCCCCC");
    db->destroy(db);
    CU_ASSERT_EQUAL(async_result, 2);
    db->free(db);
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("DB", NULL, NULL);
    if (!suite)
        goto cleanup;
    // Add tests.
    if (!CU_add_test(suite, "key-value DB", test_db))
        goto cleanup;
    // Run all tests using the CUnit Basic interface.
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
