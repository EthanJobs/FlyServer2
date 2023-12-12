#ifndef _COREQUEUEPOOL_H
#define _COREQUEUEPOOL_H

#include <pthread.h>
#include <coreQueue.h>

// 任务队列池
typedef struct QueuePool {
    Queue *n_queue;
    pthread_mutex_t n_lock;
    pthread_cond_t n_haveValue;
} QueuePool;

QueuePool *QueuePool_init(void (*fun_free)(void *));
int QueuePool_push(QueuePool *qp, void *qpv);
void *QueuePool_pop(QueuePool *qp);
void QueuePool_display(QueuePool *qp, void (*fun_print)(void *));
void QueuePool_free(QueuePool *qp);

#endif