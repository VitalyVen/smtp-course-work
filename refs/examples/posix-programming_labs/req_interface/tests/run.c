#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <CUnit/Basic.h>

#include "db_virtual.h"
#include "db_request.h"
#include "misc_macros.h"
#include "req_queue_virtual.h"

void async_result(char *key, char *value, void *arg)
{
    char *expected = arg;
    if (!expected) {
        CU_ASSERT_PTR_NULL(value)
    } else
        CU_ASSERT_STRING_EQUAL(value, expected);
}

void test_requests(void)
{
    char *key = "key";
    char *value = "value";
    void *arg = test_requests;
    struct db_request t;
    init_set_request(&t, key, value);
    CU_ASSERT_STRING_EQUAL(t.key, key);
    CU_ASSERT_STRING_EQUAL(t.value, value);
    CU_ASSERT_PTR_NOT_EQUAL(t.value, value);
    CU_ASSERT_PTR_NOT_EQUAL(t.key, key);
    CU_ASSERT_PTR_NULL(t.result_func);
    destroy_request(&t);
    init_set_request(&t, key, NULL);
    CU_ASSERT_PTR_NULL(t.value);
    CU_ASSERT_PTR_NULL(t.result_func);
    destroy_request(&t);
    init_get_request(&t, key, async_result, arg);
    CU_ASSERT_STRING_EQUAL(t.key, key);
    CU_ASSERT_PTR_NOT_EQUAL(t.key, key);
    CU_ASSERT_PTR_NULL(t.value);
    CU_ASSERT_PTR_EQUAL(t.arg, arg);
    CU_ASSERT_PTR_EQUAL(t.result_func, async_result);
    destroy_request(&t);
}

struct dumb_queue {
    struct vqueue iqueue;
    struct db_request *request;
};


#define QUEUE(vqueue) containerof(vqueue, struct dumb_queue, iqueue)

void destroy_queue(struct vqueue *vqueue)
{
}

void queue_stop(struct vqueue *vqueue)
{
    QUEUE(vqueue)->request = NULL;
}

struct db_request *queue_pop(struct vqueue *vqueue) {
    struct dumb_queue *queue = QUEUE(vqueue);
    struct db_request *t = queue->request;
    queue->request = NULL;
    return t;
}

int queue_push(struct vqueue *vqueue, struct db_request *request)
{
    QUEUE(vqueue)->request = request;
    return 0;
}

void test_queue(void)
{
    struct dumb_queue dumb_queue = {
        .iqueue = {
            .destroy = destroy_queue,
            .stop = queue_stop,
            .push = queue_push,
            .pop = queue_pop
        },
        .request = NULL
    };
    struct vqueue *queue = &dumb_queue.iqueue;
    // TODO: where is a test?
    queue->destroy(queue);
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("Interfaces", NULL, NULL);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "DB requests", test_requests))
        goto cleanup;
    if (!CU_add_test(suite, "DB requests queue", test_queue))
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
