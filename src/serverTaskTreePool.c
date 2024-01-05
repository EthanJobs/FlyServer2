#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <serverFormData.h>
#include <serverTaskTreePool.h>

int taskTreePoolNode_fun_compareValue(void *a, void *b);
int taskTreePoolNode_fun_searchValue(void *a, void *b);

int taskTreePoolValueFormDataValue_fun_compareValue(void *a, void *b);
int taskTreePoolValueFormDataValue_fun_searchValue(void *a, void *b);

int taskTreePoolNode_fun_compareValue(void *a, void *b) {
    return ((taskTreePoolNode *)a)->n_clientFd - ((taskTreePoolNode *)b)->n_clientFd;
}

int taskTreePoolNode_fun_searchValue(void *a, void *b) {
    return *(int *)a - ((taskTreePoolNode *)b)->n_clientFd;
}

int taskTreePoolValueFormDataValue_fun_compareValue(void *a, void *b) {
    return ((FormDataValue *)a)->n_name - ((FormDataValue *)b)->n_name;
}

int taskTreePoolValueFormDataValue_fun_searchValue(void *a, void *b) {
    return (char *)a - ((FormDataValue *)b)->n_name;
}

taskTreePoolValueGetFile *taskTreePoolValueGetFile_init(int fileFd, int fileIndex) {
    taskTreePoolValueGetFile *ttpvGF = (taskTreePoolValueGetFile *)malloc(sizeof(taskTreePoolValueGetFile));

    ttpvGF->n_fileFd = fileFd;
    ttpvGF->n_fileIndex = fileIndex;

    return ttpvGF;
}

void taskTreePoolValueGetFile_free(void *a) {
    if (a == NULL) return;

    taskTreePoolValueGetFile *ttpvGF = (taskTreePoolValueGetFile *)a;
    if (ttpvGF->n_fileFd > 0) close(ttpvGF->n_fileFd);
    free(ttpvGF);
}

taskTreePoolValueJson *taskTreePoolValueJson_init(Json *json) {
    taskTreePoolValueJson *ttpvJ = (taskTreePoolValueJson *)malloc(sizeof(taskTreePoolValueJson));

    ttpvJ->n_json = json;

    return ttpvJ;
}

void taskTreePoolValueJson_free(void *a) {
    if (a == NULL) return;

    taskTreePoolValueJson *ttpvJ = (taskTreePoolValueJson *)a;
    if (ttpvJ->n_json) Json_free(ttpvJ->n_json);

    return;
}

taskTreePoolValueFormData *taskTreePoolValueFormData_init() {
    taskTreePoolValueFormData *ttpvFD = (taskTreePoolValueFormData *)malloc(sizeof(taskTreePoolValueFormData));

    ttpvFD->n_formData = Tree_init(taskTreePoolValueFormDataValue_fun_compareValue, FormDataValue_free);
    ttpvFD->n_cacheData = (char *)malloc(sizeof(char) * FORMDATACACHEDATASIZE);

    return ttpvFD;
}

void taskTreePoolValueFormData_free(void *a) {
    if (a == NULL) return;

    taskTreePoolValueFormData *ttpvFD = (taskTreePoolValueFormData *)a;
    if (ttpvFD->n_formData) Tree_free(ttpvFD->n_formData);
    if (ttpvFD->n_cacheData) free(ttpvFD->n_cacheData);

    return;
}

taskTreePoolNode *taskTreePoolNode_init(int clientFd, int type, void *value, HTTPHeader *hh) {
    taskTreePoolNode *ttpn = (taskTreePoolNode *)malloc(sizeof(taskTreePoolNode));

    ttpn->n_clientFd = clientFd;
    ttpn->n_type = type;
    ttpn->n_value = value;
    ttpn->n_hh = hh;

    return ttpn;
}

void taskTreePoolNode_freeValue(taskTreePoolNode *ttpn) {
    if (ttpn == NULL || ttpn->n_value == NULL) return;

    if (ttpn->n_type == TASKTREEVALUEGETFILE) taskTreePoolValueGetFile_free(ttpn->n_value);
    else if (ttpn->n_type == TASKTREEVALUEJSON) taskTreePoolValueJson_free(ttpn->n_value);
    else if (ttpn->n_type == TASKTREEVALUEFORMDATA) taskTreePoolValueFormData_free(ttpn->n_value);

    ttpn->n_type = TASKTREEVALUEEMPTY;
    ttpn->n_value = NULL;

    return;
}

void taskTreePoolNode_free(void *a) {
    if (a == NULL) return;

    taskTreePoolNode *ttpn = (taskTreePoolNode *)a;
    if (ttpn->n_hh) HTTPHeader_free(ttpn->n_hh);
    if (ttpn->n_type == TASKTREEVALUEGETFILE) taskTreePoolValueGetFile_free(ttpn->n_value);
    else if (ttpn->n_type == TASKTREEVALUEJSON) taskTreePoolValueJson_free(ttpn->n_value);
    else if (ttpn->n_type == TASKTREEVALUEFORMDATA) taskTreePoolValueFormData_free(ttpn->n_value);

    free(ttpn);

    return;
}

taskTreePool *taskTreePool_init() {
    return TreePool_init(taskTreePoolNode_fun_compareValue, taskTreePoolNode_free);
}

int taskTreePool_insert(taskTreePool *ttp, int clientFd, int type, void *value, HTTPHeader *hh) {
    return TreePool_insert(ttp, (void *)taskTreePoolNode_init(clientFd, type, value, hh));
}

void *taskTreePool_get(taskTreePool *ttp, int clientFd) {
    return TreePool_get(ttp, clientFd, taskTreePoolNode_fun_searchValue);
}

void taskTreePool_free(taskTreePool *ttp) {
    TreePool_free(ttp);
}
