#include <serverTaskTreePool.h>
#include <stdlib.h>
#include <stdio.h>

int taskTreePoolValue_fun_compareValue(void *a, void *b);
int taskTreePoolValue_fun_searchCompareValue(void *a, void *b);

taskTreePoolValue *taskTreePoolValue_init(int clientFd, int fileFd, int fileIndex, HTTPHeader *hh) {
    taskTreePoolValue *ttpv = (taskTreePoolValue *)malloc(sizeof(taskTreePoolValue));

    ttpv->n_clientFd = clientFd;
    ttpv->n_fileFd = fileFd;
    ttpv->n_fileIndex = fileIndex;
    ttpv->n_freeData = NULL;
    ttpv->n_hh = hh;

    return ttpv;
}

void taskTreePoolValue_free(void *a) {
    if (a == NULL) return;

    taskTreePoolValue *ttpv = (taskTreePoolValue *)a;
    HTTPHeader_free(ttpv->n_hh);
    free(ttpv);

    return;
}

int taskTreePoolValue_fun_compareValue(void *a, void *b) {
    return ((taskTreePoolValue *)a)->n_clientFd - ((taskTreePoolValue *)b)->n_clientFd;
}

int taskTreePoolValue_fun_searchCompareValue(void *a, void *b) {
    return *(int *)a - ((taskTreePoolValue *)b)->n_clientFd;
}

taskTreePool *taskTreePool_init() {
    return TreePool_init(taskTreePoolValue_fun_compareValue, taskTreePoolValue_free);
}

int taskTreePool_insert(taskTreePool *ttp, int clientFd, int fileFd, int fileIndex, HTTPHeader *hh) {
    return TreePool_insert(ttp, (void *)taskTreePoolValue_init(clientFd, fileFd, fileIndex, hh));
}

taskTreePoolValue *taskTreePool_get(taskTreePool *ttp, int clientFd) {
    return TreePool_get(ttp, clientFd, taskTreePoolValue_fun_searchCompareValue);
}

void taskTreePool_free(taskTreePool *ttp) {
    TreePool_free(ttp);
}
