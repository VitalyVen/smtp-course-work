#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <CUnit/Basic.h>

#include "db_parallel.h"
#include "db_virtual.h"
#include "errors.h"
#include "req_queue_cnd.h"
#include "string_hash.h"

void test_hash(void)
{
    CU_ASSERT_EQUAL(string_hash(""), 0);
    CU_ASSERT_EQUAL(string_hash("abcdef"), string_hash("abcdef"));
    CU_ASSERT_EQUAL(string_hash("abcdef1234567890"),
                    string_hash("abcdef1234567890abcdef1234567890"));
}

int n_get;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void check_result(char *key, char *value, void *arg)
{
    char *expected = arg;
    assert(!(pthread_mutex_lock(&mutex)));
    n_get++;
    if (!expected) {
        CU_ASSERT_PTR_NULL(value)
    }
    else {
        CU_ASSERT_PTR_NOT_NULL(value);
        CU_ASSERT_STRING_EQUAL(value, expected);
    }
    assert(!(pthread_mutex_unlock(&mutex)));
    free(key);
    free(value);
}

void test_single_thread(void)
{
    struct parallel_db *par_db = malloc(sizeof(*par_db));
    assert(par_db);
    struct vdb *db = init_parallel_db(par_db, create_cnd_queue, check_result);
    n_get = 0;
    db->set(db, "1", "first");
    db->set(db, "2", "second");
    db->set(db, "3", "third");
    db->geta(db, "1", "first");
    db->geta(db, "2", "second");
    db->geta(db, "3", "third");
    db->set(db, "1", NULL);
    db->geta(db, "1", NULL);
    db->geta(db, "no_key", NULL);
    db->destroy(db);
    db->free(db);
    // TODO: Q. Is memory synchonized? (n_get)
    // A. Yes, after pthread_mutex_unlock (http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html)
    CU_ASSERT_EQUAL(n_get, 5);
}


#define N_THREADS 50
#define N_REPEATS 200

void *sample_thread(void *data)
{
    struct vdb *db = data;
    assert(db->geta);
    assert(db->async_result);
    for (int i = 0; i < N_REPEATS; i++) {
        db->set(db, "1", "first");
        db->set(db, "2", "second");
        db->geta(db, "2", "second");
        db->geta(db, "1", "first");
        db->set(db, "3", "third");
        db->geta(db, "3", "third");
    }
    pthread_exit(NULL);
    return NULL; // non-reaches
}

void test_multi_threads(void)
{
    struct parallel_db parallel_db;
    struct vdb *db = init_parallel_db(&parallel_db,
                                      create_cnd_queue, check_result);
    n_get = 0;
    pthread_t threads[N_THREADS];
    for (int i = 0; i < N_THREADS; i++)
        fatal(pthread_create(threads + i, NULL, sample_thread, db),
              "pthread_create()");
    for (int i = 0; i < N_THREADS; i++)
        fatal(pthread_join(threads[i], NULL), "pthread_join()");
    db->set(db, "2", NULL);
    db->set(db, "1", NULL);
    db->set(db, "3", NULL);
    db->geta(db, "1", NULL);
    db->geta(db, "2", NULL);
    db->geta(db, "3", NULL);
    db->destroy(db);
    CU_ASSERT_EQUAL(n_get, N_THREADS *N_REPEATS *3 + 3);
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
    if (!CU_add_test(suite, "db single-threaded test", test_single_thread))
        goto cleanup;
    if (!CU_add_test(suite, "db multi-threaded test", test_multi_threads))
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
