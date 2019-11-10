#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <CUnit/Basic.h>

#include "errors.h"
#include "req_queue_cnd.h"

void test_request(void)
{
    struct queued_request *qrequest = malloc(sizeof(*qrequest));
    assert(qrequest);
    char *key = "key";
    char *value = "value";
    struct db_request *req = &qrequest->ireq;
    req->destroy = destroy_request;
    req->free = (void (*)(struct db_request *)) free;
    init_set_request(req, key, value);
    CU_ASSERT_PTR_NOT_NULL(req->key);
    if (req->key)
        CU_ASSERT_STRING_EQUAL(req->key, key);
    CU_ASSERT_PTR_NOT_NULL(req->value);
    if (req->value)
        CU_ASSERT_STRING_EQUAL(req->value, value);
    req->destroy(req);
    req->free(req);
}

#define N_TASKS_QUEUED 100

void test_requests(void)
{
    struct requests_queue queue;
    init_requests_queue(&queue);
    for (int i = 0; i < N_TASKS_QUEUED; i++) {
        char buf[20];
        sprintf(buf, "%d", i);
        struct queued_request *qrequest = malloc(sizeof(*qrequest));
        assert(qrequest);
        struct db_request *req = &qrequest->ireq;
        req->destroy = destroy_request;
        req->free = (void (*)(struct db_request *)) free;
        init_set_request(req, buf, NULL);
        assert(req->key);
        queue_request(&queue, qrequest);
        assert(req->key);
    }
    for (int i = 0; i < N_TASKS_QUEUED; i++) {
        char buf[20];
        sprintf(buf, "%d", i);
        struct queued_request *qrequest = unqueue_request(&queue);
        CU_ASSERT_PTR_NOT_NULL(qrequest);
        if (qrequest) {
            struct db_request *req = &qrequest->ireq;
            CU_ASSERT_PTR_NOT_NULL(req->key);
            if (req->key)
                CU_ASSERT_STRING_EQUAL(req->key, buf);
            req->destroy(req);
            req->free(req);
        }
    }
    CU_ASSERT_EQUAL(no_requests(&queue), 1);
    for (int i = 0; i < 10; i++) {
        struct queued_request *qrequest = malloc(sizeof(*qrequest));
        assert(qrequest);
        queue_request(&queue, qrequest);
    }
    CU_ASSERT_EQUAL(no_requests(&queue), 0);
    destroy_requests_queue(&queue);
    CU_ASSERT_EQUAL(no_requests(&queue), 1);
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

#define N_TASKS_PRODUCERS 200
#define N_TASKS_PRODUCED 100

struct producer_data {
    struct vqueue *queue;
    int produced;
};

struct consumer_data {
    struct vqueue *queue;
    int consumed;
};

void *producer_thread(void *arg)
{
    struct producer_data *data = arg;
    data->produced = 0;
    for (int i = 0; i < N_TASKS_PRODUCED; i++) {
        char key[20];
        sprintf(key, "%d", i);
        struct db_request *request = data->queue->new_request();
        init_get_request(request, key, check_async_result, strdup(key));
        data->queue->push(data->queue, request);
        data->produced++;
    }
    pthread_exit(NULL);
    return NULL; // non-reaches
}

void *consumer_thread(void *arg)
{
    struct consumer_data *data = arg;
    data->consumed = 0;
    while (1) {
        struct db_request *request;
        request = data->queue->pop(data->queue);
        if (!request)
            break;
        request->result_func(request->key, NULL, request->arg);
        data->consumed++;
        request->destroy(request);
        request->free(request);
    }
    pthread_exit(NULL);
    return NULL; // non-reaches
}

void test_queue(void)
{
    int produced = 0;
    struct cnd_db_queue db_queue;
    struct vqueue *queue = init_cnd_db_queue(&db_queue);
    pthread_t producers[N_TASKS_PRODUCERS], consumer;
    struct producer_data producer_data[N_TASKS_PRODUCERS];
    struct consumer_data consumer_data = {.queue = queue};
    for (int i = 0; i < N_TASKS_PRODUCERS; i++) {
        producer_data[i].queue = queue;
        fatal(pthread_create(&producers[i], NULL,
                             producer_thread, producer_data + i),
              "pthread_create()");
    }
    fatal(pthread_create(&consumer, NULL,
                         consumer_thread, &consumer_data),
          "pthread_create()");
    for (int i = 0; i < N_TASKS_PRODUCERS; i++) {
        fatal(pthread_join(producers[i], NULL), "pthread_join()");
        produced += producer_data[i].produced;
    }
    queue->stop(queue);
    fatal(pthread_join(consumer, NULL), "pthread_join()");
    queue->destroy(queue);
    CU_ASSERT_EQUAL(produced, N_TASKS_PRODUCERS *N_TASKS_PRODUCED);
    CU_ASSERT_EQUAL(consumer_data.consumed, N_TASKS_PRODUCERS *N_TASKS_PRODUCED);
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("DB request queue", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "request tests", test_request))
        goto cleanup;
    if (!CU_add_test(suite, "request queue tests", test_requests))
        goto cleanup;
    if (!CU_add_test(suite, "producers -> consumer", test_queue))
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
