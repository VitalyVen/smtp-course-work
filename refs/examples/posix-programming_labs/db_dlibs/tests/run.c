#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>

#include <CUnit/Basic.h>

#include "dblib.h"
#include "errors.h"

struct vdb *db;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void async_result(char *key, char *value, void *arg)
{
    fatal(pthread_mutex_lock(&mutex),
          "pthread_mutex_lock()");
    CU_ASSERT_STRING_EQUAL(value, arg);
    fatal(pthread_mutex_unlock(&mutex),
           "pthread_mutex_unlock()");
}

void test_db(void)
{
    assert(db->geta);
    assert(db->set);
    db->set(db, "1", "first");
    db->geta(db, "1", "first");
    db->set(db, "2", "second");
    db->geta(db, "1", "first");
    db->geta(db, "2", "second");
    db->set(db, "1", "prime");
    db->set(db, "3", "third");
    db->geta(db, "1", "prime");
    db->geta(db, "3", "third");
    db->destroy(db);
}

void usage(char *argv[])
{
    printf("Usage: %s some-db-library.so \n", argv[0]);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        usage(argv);
    assert(!load_db_lib(argv[1]));
    printf("DB library: %s\n", db_name());
    db = create_db(async_result);
    assert(db);
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("DB", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "key-value DB", test_db))
        goto cleanup;
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    if (CU_get_failure_list())
        r = EXIT_FAILURE;
cleanup:
    CU_cleanup_registry();
exit:
    close_db_lib();
    if (CU_get_error())
        return CU_get_error();
    else
        return r;
}
