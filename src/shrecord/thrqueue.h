#ifndef __THRQUEUE_H
#define __THRQUEUE_H

#include <sys/queue.h>

STAILQ_HEAD(QueueHead,QueueEntry);

struct Queue {
	pthread_mutex_t mutex;
	pthread_cond_t cv;
	pthread_cond_t enq_wait_cv;
	int enq_waiters;
	int length;
	int limit;
	int pool_length;
	int pool_limit;
	struct QueueHead queue;
	struct QueueHead pool;
};

struct QueueEntry {
	void *item;
	STAILQ_ENTRY(QueueEntry) entries;
};

struct Queue* queue_init();
int queue_destroy(struct Queue *q);
int queue_empty(struct Queue *q);
int queue_full(struct Queue *q);
int queue_enq(struct Queue *q, void *item);
int queue_length(struct Queue *q);
int queue_pool_length(struct Queue *q);
void queue_limit(struct Queue *q, int limit);
void queue_pool_limit(struct Queue *q, int limit);
void *queue_deq(struct Queue *q);

#endif
