#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <CUnit/Basic.h>

#include "errors.h"
#include "db_virtual.h"
#include "db_lock.h"
#include "string_hash.h"

void test_hash(void)
{
    CU_ASSERT_EQUAL(string_hash(""), 0);
    CU_ASSERT_EQUAL(string_hash("abcdef"), string_hash("abcdef"));
    CU_ASSERT_EQUAL(string_hash("abcdef1234567890"),
                    string_hash("abcdef1234567890abcdef1234567890"));
}

#define ASSERT_DB(key, value) { \
    char *v = value; \
    char *r = db->get(db, key); \
    if (!value) \
        CU_ASSERT_PTR_NULL(r)  \
    else { \
        CU_ASSERT_PTR_NOT_NULL(r);  \
        CU_ASSERT_STRING_EQUAL(r, v); \
    }; \
    free(r); \
}

void async_get_result(char *key, char *value, void *arg)
{
    CU_ASSERT_STRING_EQUAL(value, arg);
    free(key);
    free(value);
}

void test_db(void)
{
    struct lock_db *lock_db = malloc(sizeof(*lock_db));
    assert(lock_db);
    struct vdb *db = init_lock_db(lock_db, async_get_result);
    db->set(db, "1", "first");
    ASSERT_DB("1", "first");
    db->set(db, "2", "second");
    ASSERT_DB("2", "second");
    ASSERT_DB("1", "first");
    db->geta(db, "1", "first");
    db->set(db, "3", "third");
    ASSERT_DB("3", "third");
    db->set(db, "2", NULL);
    ASSERT_DB("2", NULL);
    db->set(db, "1", NULL);
    ASSERT_DB("1", NULL);
    db->set(db, "3", NULL);
    db->destroy(db);
    db->free(db);
}

#define NTHREADS 50
#define NREPEATS 1000

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

void *thread_func(void *arg)
{
    struct vdb *db = arg;
    int r = 0;
    for (int i = 0; i < NREPEATS; i++) {
        db->set(db, "1", "first");
        db->set(db, "2", "second");
        db->set(db, "3", "third");
        char *r1 = db->get(db, "1");
        char *r2 = db->get(db, "2");
        char *r3 = db->get(db, "3");
        r += r1 != NULL &&
             strcmp(r1, "first") == 0;
        r += r2 != NULL &&
             strcmp(r2, "second") == 0;
        r += r3 != NULL &&
             strcmp(r3, "third") == 0;
        free(r1);
        free(r2);
        free(r3);
    }
    pthread_mutex_lock(&mut);
    CU_ASSERT_EQUAL(r, 3 *NREPEATS);
    pthread_mutex_unlock(&mut);
    return NULL;
}

void test_db_threads(void)
{
    pthread_t t[NTHREADS];
    struct lock_db lock_db;
    void *result;
    struct vdb *db = init_lock_db(&lock_db, NULL);
    for (int i = 0; i < NTHREADS; i++)
        fatal(pthread_create(t + i, NULL, thread_func, db),
              "pthread_create()");
    for (int i = 0; i < NTHREADS; i++)
        fatal(pthread_join(t[i], &result),
              "pthread_join()");
    db->destroy(db);
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("DB", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "hashing function", test_hash))
        goto cleanup;
    if (!CU_add_test(suite, "db", test_db))
        goto cleanup;
    if (!CU_add_test(suite, "multi-threaded db", test_db_threads))
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
