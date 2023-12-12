#ifndef _SERVERAPI_H
#define _SERVERAPI_H

#include <coreTree.h>
#include <coreLink.h>
#include <coreSQL.h>
#include <serverTaskTreePool.h>

typedef struct queryNode {
    int n_dataType;
    char *n_name;
} queryNode;

typedef struct APINode {
    char *n_method;
    char *n_name;
    char *n_sql;
    Tree *n_query;
    int (*fun_handle)(taskTreePoolValue *, struct APINode *, MYSQL *);
} APINode;

typedef struct API {
    Tree *n_apis;   // APINode
    Link *n_data;   // JSON
} API;

queryNode *queryNode_init(char *name, int dataType);
void queryNode_free(void *a);

APINode *APINode_init(char *method, char *apiName, char *name, char *sql);
void APINode_free(void *a);

API *API_init(char *path);
void API_display(API *api);
void API_free(void *a);
APINode *API_getAN(API *api, char *url);

#endif
