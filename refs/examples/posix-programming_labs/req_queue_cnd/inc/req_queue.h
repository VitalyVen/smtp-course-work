#ifndef __REQ_QUEUE_H__
#define __REQ_QUEUE_H__

#include "req_cnd.h"

void init_requests_queue(struct requests_queue *queue);

void destroy_requests_queue(struct requests_queue *queue);

void queue_request(struct requests_queue *queue, struct queued_request *request);

struct queued_request *unqueue_request(struct requests_queue *queue);

int no_requests(struct requests_queue *queue);

#endif
