#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <CUnit/Basic.h>

#include "errors.h"
#include "test_out.h"
#include "queue_sem.h"

int init(void)
{
    return 0;
}

int clean(void)
{
    return 0;
}

#define N_ENTRIES 1

void *producer_thread(void *arg)
{
    struct queue_sem *q = arg;
    intptr_t i;
    for (i = 0; i < N_ENTRIES; i++)
        queue_sem_push(q, (void *) i);
    pthread_exit((void *) i);
    return NULL; // non-reaches
}

void *consumer_thread(void *arg)
{
    intptr_t ok = 0;
    struct queue_sem *q = arg;
    for (intptr_t i = 0; i < N_ENTRIES; i++) {
        intptr_t n = (intptr_t) queue_sem_pop(q);
        if (i == n)
            ok++;
    }
    pthread_exit((void *) ok);
    return NULL; // non-reaches
}

void run_queue_sem(int queue_size)
{
    struct queue_sem queue;
    init_queue_sem(&queue, queue_size);
    pthread_t consumer;
    pthread_t producer;
    assert(!pthread_create(&consumer, NULL, consumer_thread, &queue));
    assert(!pthread_create(&producer, NULL, producer_thread, &queue));
    void *result;
    assert(!pthread_join(consumer, &result));
    int consumed = (intptr_t) result;
    assert(!pthread_join(producer, &result));
    int produced = (intptr_t) result;
    CU_ASSERT_EQUAL(produced, consumed);
    CU_ASSERT_EQUAL(consumed, N_ENTRIES);
    CU_ASSERT_EQUAL(queue.head, queue.tail);
    CU_ASSERT_EQUAL(queue.head, N_ENTRIES % queue_size);
    destroy_queue_sem(&queue);
}

void test_queue_sem(void)
{
    run_queue_sem(1);
    run_queue_sem(2);
    run_queue_sem(5);
    run_queue_sem(20);
    run_queue_sem(100);
    run_queue_sem(1000);
    run_queue_sem(10000);
}

void *producer_eintr(void *arg)
{
    struct queue_sem *q = arg;
    queue_sem_push(q, producer_eintr);
    pthread_exit(NULL);
    return NULL; // non-reaches
}

void *consumer_eintr(void *arg)
{
    struct queue_sem *q = arg;
    pthread_exit(queue_sem_pop(q));
    return NULL; // non-reaches
}

void sig_handler(int signal)
{
}

void test_eintr(void)
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sig_handler;
    assert(!sigaction(SIGINT, &sa, NULL));
    struct queue_sem queue;
    init_queue_sem(&queue, 10);
    pthread_t consumer;
    assert(!pthread_create(&consumer, NULL, consumer_eintr, &queue));
    // FIXME: without sleeping, signal could be delivered before sem_wait.
    sleep(1);
    assert(!pthread_kill(consumer, SIGINT));
    pthread_t producer;
    assert(!pthread_create(&producer, NULL, producer_eintr, &queue));
    assert(!pthread_join(producer, NULL));
    void *result;
    assert(!pthread_join(consumer, &result));
    CU_ASSERT_PTR_EQUAL(result, producer_eintr)
    destroy_queue_sem(&queue);
}

void test_queue_fail()
{
    char s[1024];
    REDIRECT_OUTPUTS;
    struct queue_sem q;
    // TODO: It is better to use dmalloc.
    CU_ASSERT_EQUAL(init_queue_sem(&q, SSIZE_MAX), -1);
    READ_ERR(s);
    CU_ASSERT(strstr(s, "alloc()") != NULL || strstr(s, "sem_init()") != NULL);
    RESTORE_OUTPUTS;
}

int main()
{
    int r = EXIT_SUCCESS;
    if (CU_initialize_registry() != CUE_SUCCESS)
        goto exit;
    CU_pSuite suite = CU_add_suite("queue with semaphores", init, clean);
    if (!suite)
        goto cleanup;
    if (!CU_add_test(suite, "queue tests", test_queue_sem))
        goto cleanup;
    if (!CU_add_test(suite, "eintr test", test_eintr))
        goto cleanup;
    if (!CU_add_test(suite, "error test", test_queue_fail))
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
