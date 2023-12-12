#include <serverTaskQueuePool.h>
#include <stdlib.h>

taskQueuePoolValue *taskQueuePoolValue_init(int clientFd) {
    taskQueuePoolValue *tqpv = (taskQueuePoolValue *)malloc(sizeof(taskQueuePoolValue));

    tqpv->n_clientFd = clientFd;

    return tqpv;
}

void taskQueuePoolValue_free(void *a) {
    if (!a) return;

    taskQueuePoolValue *tqpv = (taskQueuePoolValue *)a;

    free(tqpv);

    return;
}

taskQueuePool *taskQueuePool_init() {
    return QueuePool_init(taskQueuePoolValue_free);
}

int taskQueuePool_push(taskQueuePool *tqp, int fd) {
    return QueuePool_push(tqp, (void *)taskQueuePoolValue_init(fd));
}

taskQueuePoolValue *taskQueuePool_pop(taskQueuePool *tqp) {
    return (taskQueuePoolValue *)QueuePool_pop(tqp);
}

void taskQueuePool_free(taskQueuePool *tqp) {
    QueuePool_free(tqp);
}