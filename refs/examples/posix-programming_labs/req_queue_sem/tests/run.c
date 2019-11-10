#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <pthread.h>

#include <CUnit/Basic.h>

#include "test_out.h"
#include "req_queue_sem.h"

int init(void)
{
    return 0;
}

int clean(void)
{
    return 0;
}

#define N_TASKS_QUEUED 100

void test_requests(void)
{
    struct sem_db_queue sem_queue;
    struct vqueue *queue =
        init_sem_db_queue(&sem_queue, N_TASKS_QUEUED);
    struct db_request *request;
    for (int i = 0; i < N_TASKS_QUEUED; i++) {
        char key[20], value[20];
        sprintf(key, "%d", i);
        sprintf(value, "%d-%d", i, i);
        request = queue->new_request();
        init_set_request(request, key, value);
        queue->push(queue, request);
    }
    for (int i = 0; i < N_TASKS_QUEUED; i++) {
        char key[20], value[20];
        sprintf(key, "%d", i);
        sprintf(value, "%d-%d", i, i);
        request = queue->pop(queue);
        CU_ASSERT_PTR_NOT_NULL(request);
        if (request) {
            CU_ASSERT_PTR_NOT_NULL(request->key);
            if (request->key)
                CU_ASSERT_STRING_EQUAL(request->key, key);
            CU_ASSERT_PTR_NOT_NULL(request->value);
            if (request->value)
                CU_ASSERT_STRING_EQUAL(request->value, value);
            request->destroy(request);
            request->free(request);
        }
    }
    queue->stop(queue);
    request = queue->pop(queue);
    CU_ASSERT_PTR_NULL(request);
    queue->destroy(queue);
}

void check_async_result(char *key, char *value, void *arg)
{
    char *expected = arg;
    if (!expected)
        CU_ASSERT_PTR_NULL(key)
        else
            CU_ASSERT_STRING_EQUAL(key, expected);
    free(expected);
    CU_ASSERT_PTR_NULL(value);
}

#define N_TASKS_PRODUCED 100

int consumed;

void *consumer_thread(void *arg)
{
    struct vqueue *queue = arg;
    consumed = 0;
    while (1) {
        struct db_request *request = queue->pop(queue);
        if (!request)
            break;
        request->result_func(request->key, NULL, request->arg);
        consumed++;
        request->destroy(request);
        request->free(request);
    }
    pthread_exit(NULL);
    return NULL; // non-reaches
}

void test_queue(void)
{
    int produced = 0;
    struct sem_db_queue sem_queue;
    struct vqueue *queue =
        init_sem_db_queue(&sem_queue, N_TASKS_PRODUCED / 7);
    pthread_t consumer;
    fatal(pthread_create(&consumer, NULL, consumer_thread, queue),
          "pthread_create()");
    for (int i = 0; i < N_TASKS_PRODUCED; i++) {
        struct db_request *request = queue->new_request();
        char key[20];
        sprintf(key, "%d", i);
        init_get_request(request, key, check_async_result, strdup(key));
        queue->push(queue, request);
        produced++;
    }
    queue->stop(queue);
    fatal(pthread_join(consumer, NULL), "pthread_join()");
    queue->destroy(queue);
    CU_ASSERT_EQUAL(produced, N_TASKS_PRODUCED);
    CU_ASSERT_EQUAL(consumed, N_TASKS_PRODUCED);
}

/*
// FIXME: it works on bsd/64 ((
void test_fail(void)
{
    REDIRECT_OUTPUTS;
    char s[1024];
    struct sem_db_queue sem_queue;
    CU_ASSERT_EQUAL(init_sem_db_queue(&sem_queue, INT_MAX), NULL);
    READ_ERR(s);
    CU_ASSERT(strstr(s, "alloc") != NULL);
    RESTORE_OUTPUTS;
}
*/

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("DB queue queue", init, clean);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "simple tests", test_requests))
        goto cleanup;
    if (!CU_add_test(suite, "producer -> consumer", test_queue))
        goto cleanup;
    //if (!CU_add_test(suite, "out of memory tests", test_fail))
    //    goto cleanup;
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

