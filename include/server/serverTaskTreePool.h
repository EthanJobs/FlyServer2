#ifndef _SERVERTASKTREEPOOL_H
#define _SERVERTASKTREEPOOL_H

#include <coreTreePool.h>
#include <coreHTTPHeader.h>

typedef TreePool taskTreePool;
typedef struct taskTreePoolValue {
    int n_clientFd;
    int n_fileFd;
    int n_fileIndex;
    HTTPHeader *n_hh;
} taskTreePoolValue;

taskTreePoolValue *taskTreePoolValue_init(int clientFd, int fileFd, int fileIndex, HTTPHeader *hh);
void taskTreePoolValue_free(void *ttpv);

taskTreePool *taskTreePool_init();
int taskTreePool_insert(taskTreePool *ttp, int clientFd, int fileFd, int fileIndex, HTTPHeader *hh);
taskTreePoolValue *taskTreePool_get(taskTreePool *ttp, int clientFd);
void taskTreePool_free(taskTreePool *ttp);

// void taskTreePool_display(taskTreePool *ttp, int tmp);

#endif
