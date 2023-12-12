#ifndef _SERVERTASKQUEUEPOOL_H
#define _SERVERTASKQUEUEPOOL_H

#include <coreQueuePool.h>

typedef QueuePool taskQueuePool;
typedef struct taskQueuePoolValue {
    int n_clientFd;
} taskQueuePoolValue;

taskQueuePoolValue *taskQueuePoolValue_init(int clientFd);
void taskQueuePoolValue_free(void *a);

taskQueuePool *taskQueuePool_init();
int taskQueuePool_push(taskQueuePool *tqp, int fd);
taskQueuePoolValue *taskQueuePool_pop(taskQueuePool *tqp);
void taskQueuePool_free(taskQueuePool *tqp);

// void taskQueuePool_display(taskQueuePool *tqp);

#endif