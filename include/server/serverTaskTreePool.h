#ifndef _SERVERTASKTREEPOOL_H
#define _SERVERTASKTREEPOOL_H

#include <coreTreePool.h>
#include <coreJson.h>
#include <coreTree.h>
#include <coreHTTPHeader.h>

#define TASKTREEVALUEEMPTY -1
#define TASKTREEVALUEGETFILE 0
#define TASKTREEVALUEJSON 1
#define TASKTREEVALUEFORMDATA 2

#define CONTENTTYPEJSON "application/json;"
#define CONTENTTYPEFORMDATA "multipart/form-data;"

#define FORMDATACACHEDATASIZE 1024

typedef TreePool taskTreePool;

typedef struct taskTreePoolNode {
    int n_clientFd;
    int n_type;
    void *n_value;
    HTTPHeader *n_hh;
} taskTreePoolNode;

typedef struct taskTreePoolValueGetFile {
    int n_fileFd;
    int n_fileIndex;
} taskTreePoolValueGetFile;

typedef struct taskTreePoolValueJson {
    Json *n_json;
} taskTreePoolValueJson;

typedef struct taskTreePoolValueFormData {
    Tree *n_formData;
    char *n_cacheData;
} taskTreePoolValueFormData;

taskTreePoolValueGetFile *taskTreePoolValueGetFile_init(int fileFd, int fileIndex);
void taskTreePoolValueGetFile_free(void *a);

taskTreePoolValueJson *taskTreePoolValueJson_init(Json *json);
void taskTreePoolValueJson_free(void *a);

taskTreePoolValueFormData *taskTreePoolValueFormData_init();
void taskTreePoolValueFormData_free(void *a);

taskTreePoolNode *taskTreePoolNode_init(int clientFd, int type, void *value, HTTPHeader *hh);
void taskTreePoolNode_freeValue(taskTreePoolNode *ttpn);
void taskTreePoolNode_free(void *a);

taskTreePool *taskTreePool_init();
int taskTreePool_insert(taskTreePool *ttp, int clientFd, int type, void *value, HTTPHeader *hh);
void *taskTreePool_get(taskTreePool *ttp, int clientFd);
void taskTreePool_free(taskTreePool *ttp);
void taskTreePool_display(taskTreePool *ttp);

#endif
