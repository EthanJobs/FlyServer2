#include <serverAPI.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <coreLink.h>
#include <coreJson.h>

int getTypeIndex(char *fileName);
int compareQNToQN(void *a, void *b);
int compareStringToQN(void *a, void *b);
int compareANToAN(void *a, void *b);
int compareStringToAN(void *a, void *b);
int API_init_in(char *path, API *api);

int compareQNToQN(void *a, void *b) {
    return strcmp(((queryNode *)a)->n_name, ((queryNode *)b)->n_name);
}

int compareStringToQN(void *a, void *b) {
    return strcmp((char *)a, ((queryNode *)b)->n_name);
}

void freeJSON(void *a) {
    Json_free((Json *)a);
}

queryNode *queryNode_init(char *name, int dataType) {
    queryNode *qn = (queryNode *)malloc(sizeof(queryNode));
    qn->n_name = name;
    qn->n_dataType = dataType;

    return qn;
}

void queryNode_free(void *a) {
    queryNode *qn = (queryNode *)a;
    if (qn == NULL) return;

    free(qn);

    return;
}

int compareANToAN(void *a, void *b) {
    return strcmp(((APINode *)a)->n_name, ((APINode *)b)->n_name);
}

int compareStringToAN(void *a, void *b) {
    return strcmp((char *)a, ((APINode *)b)->n_name);
}

APINode *APINode_init(char *method, char *apiName, char *name, char *sql) {
    APINode *an = (APINode *)malloc(sizeof(APINode));
    an->n_method = method;
    an->n_name = (char *)malloc(sizeof(char) * (strlen(apiName) + strlen(name) + 3));
    sprintf(an->n_name, "/%s/%s", apiName, name);
    an->n_sql = sql;
    an->n_query = Tree_init(compareANToAN, queryNode_free);
    an->fun_handle = NULL;

    return an;
}

void APINode_free(void *a) {
    APINode *an = (APINode *)a;
    if (an == NULL) return;

    if (an->n_name) free(an->n_name);
    if (an->n_query) Tree_free(an->n_query);
    free(an);

    return;
}

int getTypeIndex(char *fileName) {
    for (int i = strlen(fileName) - 1; i >= 0; i--)
        if (fileName[i] == '.') return i + 1;
    return -1;
}

int API_init_in(char *path, API *api) {
    DIR *d;
    struct dirent *file;

    if (!(d = opendir(path))) {
        printf("error opendir %s!!!\n", path);
        return -1;
    }

    while ((file = readdir(d)) != NULL) {
        if (strncmp(file->d_name, ".", 1) == 0) continue;

        if (file->d_type == 4) {
            char *buf = (char *)malloc(sizeof(char) * (strlen(path) + strlen(file->d_name) + 2));
            sprintf(buf, "%s/%s", path, file->d_name);
            API_init_in(buf, api);
            free(buf);
            continue;
        }

        if (strcmp(file->d_name + getTypeIndex(file->d_name), "json") != 0) continue;

        char *buf = (char *)malloc(sizeof(char) * (strlen(path) + strlen(file->d_name) + 2));
        sprintf(buf, "%s/%s", path, file->d_name);

        int fileFd = open(buf, O_RDONLY);
        free(buf);
        if (fileFd == -1) continue;

        Json *json = Json_initByFd(fileFd);
        close(fileFd);

        jsonValue *JV_name = Json_getValueInObj(json, "head");
        if (JV_name == NULL || JV_name->n_dataType != JSONSTR)
            continue;

        Link_headInsertValue(api->n_data, (void *)json);

        jsonValue *JV_api = Json_getValueInObj(json, "api");
        if (JV_api == NULL || JV_api->n_dataType != JSONNUMS)
            continue;

        Iterator *apiLI = Link_getIterator((Link *)JV_api->n_data.p_data);

        while (apiLI->fun_hasNext(apiLI)) {
            jsonValue *JV_api_node = apiLI->fun_next(apiLI);
            if (JV_api_node == NULL || JV_api_node->n_dataType != JSONOBJ)
                continue;

            jsonValue *JV_api_method = Json_getValueInObj((Json *)JV_api_node->n_data.p_data, "method");
            if (JV_api_method == NULL || JV_api_method->n_dataType != JSONSTR)
                continue;
            jsonValue *JV_api_name = Json_getValueInObj((Json *)JV_api_node->n_data.p_data, "name");
            if (JV_api_name == NULL || JV_api_name->n_dataType != JSONSTR)
                continue;
            jsonValue *JV_api_sql = Json_getValueInObj((Json *)JV_api_node->n_data.p_data, "sql");
            if (JV_api_sql != NULL && JV_api_sql->n_dataType != JSONSTR)
                continue;

            APINode *an = APINode_init((char *)JV_api_method->n_data.p_data, (char *)JV_name->n_data.p_data, (char *)JV_api_name->n_data.p_data, JV_api_sql ? (char *)JV_api_sql->n_data.p_data : "");

            jsonValue *JV_api_query = Json_getValueInObj((Json *)JV_api_node->n_data.p_data, "query");
            if (JV_api_query != NULL && JV_api_query->n_dataType == JSONNUMS) {
                Iterator *apiQueryLI = Link_getIterator((Link *)JV_api_query->n_data.p_data);
                while (apiQueryLI->fun_hasNext(apiQueryLI)) {
                    jsonValue *JV_api_node_query = apiLI->fun_next(apiQueryLI);
                    if (JV_api_node_query == NULL || JV_api_node_query->n_dataType != JSONOBJ)
                        continue;

                    jsonValue *JV_api_node_query_name = Json_getValueInObj((Json *)JV_api_node_query->n_data.p_data, "name");
                    if (JV_api_node_query_name == NULL || JV_api_node_query_name->n_dataType != JSONSTR)
                        continue;
                    jsonValue *JV_api_node_query_type = Json_getValueInObj((Json *)JV_api_node_query->n_data.p_data, "type");
                    if (JV_api_node_query_type == NULL || JV_api_node_query_type->n_dataType != JSONSTR)
                        continue;

                    // ! bug                                                                                            |
                    Tree_insertValue(an->n_query, (void *)queryNode_init((char *)JV_api_node_query_name->n_data.p_data, JSONSTR));
                }

                Iterator_free(apiQueryLI);
            }

            Tree_insertValue(api->n_apis, (void *)an);
        }
        Iterator_free(apiLI);
        // Tree_insertValue(t, (void *)api);
    }

    closedir(d);
    return 0;
}

API *API_init(char *path) {
    API *api = (API *)malloc(sizeof(API));
    api->n_apis = Tree_init(compareANToAN, APINode_free);
    api->n_data = Link_init(freeJSON);
    API_init_in(path, api);

    return api;
}

void API_display(API *api) {
    Iterator *j = Tree_getInorderIterator(api->n_apis);

    while (j->fun_hasNext(j)) {
        APINode *an = (APINode *)j->fun_next(j);
        printf("name: %s\n", an->n_name);
        printf("sql: %s\n", an->n_sql);

        Iterator *k = Tree_getInorderIterator(an->n_query);

        while (k->fun_hasNext(k)) {
            queryNode *qn = (queryNode *)k->fun_next(k);
            printf("  name:%s\n", qn->n_name);
            printf("  type:%d\n\n", qn->n_dataType);
        }

        Iterator_free(k);
    }

    Iterator_free(j);
}

APINode *API_getAN(API *api, char *url) {
    return (APINode *)Tree_getValue(api->n_apis, url, compareStringToAN);
}

void API_free(void *a) {
    if (!a) return;
    API *api = (API *)a;

    if (api->n_apis) Tree_free(api->n_apis);
    if (api->n_data) Link_free(api->n_data);
    free(api);

    return;
}