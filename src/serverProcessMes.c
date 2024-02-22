#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <coreLog.h>
#include <coreSQL.h>
#include <coreDate.h>
#include <coreJson.h>
#include <coreMIME.h>
#include <coreIterator.h>
#include <serverAPI.h>
#include <serverMesCntl.h>
#include <serverFormData.h>
#include <serverProcessMes.h>
#include <serverTaskTreePool.h>
#include <serverTaskQueuePool.h>

extern int g_epfd;
extern taskQueuePool *g_tqp;
extern taskTreePool *g_ttp;
extern API *g_API;

// GET获取文件
int getFile(taskTreePoolNode *ttpn);

// 判断参数
int judgeJsonType(APINode *an, Json *json);

// sql处理
int processSQL(taskTreePoolNode *ttpn, APINode *an, Json *json, MYSQL *sqlConnection);

// FormData数据处理
int processFormData(taskTreePoolNode *ttpn, APINode *an, MYSQL *sqlConnection);

// json数据处理
int processJson(taskTreePoolNode *ttpn, APINode *an, MYSQL *sqlConnection);

// 预设数据处理
int processAPINode(taskTreePoolNode *ttpn, APINode *an, MYSQL *sqlConnection);

// 结束处理
void processMes_free(int sign, taskTreePoolNode *ttpn, taskQueuePoolValue *tqpv);

int getFile(taskTreePoolNode *ttpn) {
    // 目前只支持HTTP1.0
    if (strncmp(ttpn->n_hh->n_HTTPHeaderLine->n_version, "HTTP/1", 6)) return 0;

    // 判断method
    if (strcmp("GET", ttpn->n_hh->n_HTTPHeaderLine->n_method)) return 0;

    taskTreePoolValueGetFile *ttpvGF;
    if (ttpn->n_value != NULL) ttpvGF = (taskTreePoolValueGetFile *)ttpn->n_value;

    // 首次获取初始化
    if (ttpn->n_value == NULL) {
        ttpn->n_type = TASKTREEVALUEGETFILE;
        ttpvGF = taskTreePoolValueGetFile_init(TASKTREEVALUEEMPTY, 0);
        ttpn->n_value = (void *)ttpvGF;

        // 如果是/则返回主页
        if (!strcmp(ttpn->n_hh->n_HTTPHeaderLine->n_url, "/")) {
            ttpvGF->n_fileFd = open(WEB_INDEX, O_RDONLY);
            if (ttpvGF->n_fileFd > 0) dprintf(ttpn->n_clientFd, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", MIME_getFileMIME(WEB_INDEX));
        }

        // 打开目标文件
        if (ttpvGF->n_fileFd <= 0) {
            int pathLen = strlen(WEB_PATH);
            char *path = (char *)malloc(sizeof(char) * (pathLen + strlen(ttpn->n_hh->n_HTTPHeaderLine->n_url) + 1));
            strcpy(path, WEB_PATH);
            strcpy(path + pathLen, ttpn->n_hh->n_HTTPHeaderLine->n_url);
            ttpvGF->n_fileFd = open(path, O_RDONLY);
            if (ttpvGF->n_fileFd > 0) dprintf(ttpn->n_clientFd, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", MIME_getFileMIME(path));
            free(path);
        }

        // 如果找不到文件就返回WEB_INDEX文件
        if (ttpvGF->n_fileFd <= 0) {
            ttpvGF->n_fileFd = open(WEB_INDEX, O_RDONLY);
            if (ttpvGF->n_fileFd > 0) dprintf(ttpn->n_clientFd, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", MIME_getFileMIME(WEB_INDEX));

            if (ttpvGF->n_fileFd <= 0) {
                dprintf(ttpn->n_clientFd, "HTTP/1.1 404 Not Found\r\n\r\n");
                return 0;
            }
        }
    }

    if (MesCntl_getHostFileToFd(ttpvGF->n_fileFd, ttpn->n_clientFd, &ttpvGF->n_fileIndex, BUFFER_SIZE) == -1) {
        if (errno == EAGAIN)
            return 1;
    }

    return 0;
}

int judgeJsonType(APINode *an, Json *json) {
    Iterator *i = Tree_getInorderIterator(an->n_query);

    while (i->fun_hasNext(i)) {
        queryNode *qn = (queryNode *)i->fun_next(i);
        jsonValue *jv = Json_getValueInObj(json, qn->n_name);
        if (jv == NULL || jv->n_dataType != qn->n_dataType) return 1;
    }

    Iterator_free(i);

    return 0;
}

int processSQL(taskTreePoolNode *ttpn, APINode *an, Json *json, MYSQL *sqlConnection) {
    char *sql;
    int index = 0, tmp = 0, sqlLen = 0, i = 0;

    for (; i < strlen(an->n_sql); i++) {
        if (an->n_sql[i] == '$' && an->n_sql[i + 1] == '{') {
            tmp = sqlLen;
            sqlLen += i - index;

            if (!index) sql = (char *)malloc(sizeof(char) * sqlLen + 1);
            else sql = (char *)realloc(sql, sizeof(char) * sqlLen + 1);

            strncpy(sql + tmp, an->n_sql + index, i - index);
            sql[sqlLen] = '\0';
            index = i + 2;

            for (; i < strlen(an->n_sql); i++) {
                if (an->n_sql[i] == '}') {
                    char *jsonSearch = (char *)malloc(sizeof(char) * i - index + 1);
                    strncpy(jsonSearch, an->n_sql + index, i - index);
                    jsonSearch[i - index] = '\0';

                    jsonValue *jv = Json_getValueInObj(json, jsonSearch);
                    free(jsonSearch);

                    if (jv == NULL || jv->n_dataType == JSONOBJ || jv->n_dataType == JSONNUMS) {
                        free(sql);
                        return 0;
                    }

                    tmp = sqlLen;
                    if (jv->n_dataType == JSONSTR) sqlLen += strlen((char *)jv->n_data.p_data) + 2;
                    else sqlLen += strlen((char *)jv->n_data.p_data);

                    sql = (char *)realloc(sql, sizeof(char) * sqlLen);

                    if (jv->n_dataType == JSONSTR)
                        sprintf(sql + tmp, "\'%s\'", (char *)jv->n_data.p_data);
                    else if (jv->n_dataType == JSONINT)
                        sprintf(sql + tmp, "%d", jv->n_data.i_data);
                    else if (jv->n_dataType == JSONDOUBLE)
                        sprintf(sql + tmp, "%lf", jv->n_data.d_data);
                    else
                        sprintf(sql + tmp, "%s", (char *)jv->n_data.p_data);

                    index = i + 1;
                    break;
                }
            }
        }
    }

    sqlLen += i - index;
    if (!index) sql = (char *)malloc(sizeof(char) * sqlLen);
    else sql = (char *)realloc(sql, sizeof(char) * sqlLen);
    strncat(sql, an->n_sql + index, i - index);

    dprintf(ttpn->n_clientFd, "HTTP/1.1 200 OK\r\n\r\n");
    sendSqlRes(sqlConnection, sql, ttpn->n_clientFd);
    LOG_DBG("[THREAD : %ld]: \"%s\" is executed\n", pthread_self(), sql);

    free(sql);
    return 0;
}

// FormData数据处理
int processFormData(taskTreePoolNode *ttpn, APINode *an, MYSQL *sqlConnection) {
    printf("我读到了FormData\n");
    FormData *formdata = FormData_init();

    LOG_DBG("[SERVER]: Get %d FormData: \n", ttpn->n_clientFd);

    return FormData_fd_init(formdata, ttpn->n_clientFd);
}

// json数据处理
int processJson(taskTreePoolNode *ttpn, APINode *an, MYSQL *sqlConnection) {
    int sign;
    Json *json = Json_initByFd(ttpn->n_clientFd);

    LOG_DBG("[SERVER]: Get %d JSON: \n", ttpn->n_clientFd);
    if (DBGSIGN) Json_displayValue(json);

    if (!judgeJsonType(an, json)) sign = processSQL(ttpn, an, json, sqlConnection);

    Json_free(json);
    return sign;
}

int processAPINode(taskTreePoolNode *ttpn, APINode *an, MYSQL *sqlConnection) {
    // 跨域判断
    if (!strcmp("OPTIONS", ttpn->n_hh->n_HTTPHeaderLine->n_method)) {
        dprintf(ttpn->n_clientFd, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: POST\r\n\r\n");
        return 0;
    }

    // 自定义数据处理
    if (an->fun_handle) return an->fun_handle(ttpn, an, sqlConnection);

    // 目前只支持HTTP1.0
    if (strncmp(ttpn->n_hh->n_HTTPHeaderLine->n_version, "HTTP/1", 6)) return 0;

    // 判断method
    if (strcmp(an->n_method, ttpn->n_hh->n_HTTPHeaderLine->n_method)) return 0;

    // 判断返回类型
    char *contentType = HTTPHeader_getHeaderValue(ttpn->n_hh, "Content-Type");

    // 分类数据处理
    if (!strncmp(contentType, CONTENTTYPEJSON, strlen(CONTENTTYPEJSON))) return processJson(ttpn, an, sqlConnection);
    else if (!strncmp(contentType, CONTENTTYPEFORMDATA, strlen(CONTENTTYPEFORMDATA))) return processFormData(ttpn, an, sqlConnection);
    else return 0;
}

void processMes_free(int sign, taskTreePoolNode *ttpn, taskQueuePoolValue *tqpv) {
    if (sign == 0) {
        // 结束
        HTTPHeader_free(ttpn->n_hh);
        ttpn->n_hh = NULL;
        taskTreePoolNode_freeValue(ttpn);
        LOG_DBG("[THREAD : %ld]: %d get close\n", pthread_self(), ttpn->n_clientFd);
        close(ttpn->n_clientFd);
    } else if (sign == 1) {
        // 未读完文件
        struct epoll_event ev;
        ev.events = EPOLLOUT | EPOLLONESHOT | EPOLLET;
        ev.data.fd = ttpn->n_clientFd;
        epoll_ctl(g_epfd, EPOLL_CTL_MOD, ttpn->n_clientFd, &ev);
    } else if (sign == 2) {
        // 未写完文件
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
        ev.data.fd = ttpn->n_clientFd;
        epoll_ctl(g_epfd, EPOLL_CTL_MOD, ttpn->n_clientFd, &ev);
    }

    taskQueuePoolValue_free(tqpv);
}

void *processMes(void *arg) {
    MYSQL *sqlConnection = MYSQL_init(HOST, USERNAME, PASSWORD, DATABASE, 0);
    int sign;

    while (1) {
        taskQueuePoolValue *tqpv = taskQueuePool_pop(g_tqp);
        taskTreePoolNode *ttpn = taskTreePool_get(g_ttp, tqpv->n_clientFd);
        LOG_DBG("[THREAD : %ld]: Get %d fd\n", pthread_self(), tqpv->n_clientFd);

        if (ttpn == NULL) {
            //!优化
            taskTreePool_insert(g_ttp, tqpv->n_clientFd, -1, NULL, NULL);
            ttpn = taskTreePool_get(g_ttp, tqpv->n_clientFd);
        }

        if (ttpn->n_hh == NULL) {
            ttpn->n_hh = HTTPHeader_fd_init(ttpn->n_clientFd);
            if (ttpn->n_hh == NULL) {
                LOG_DBG("[THREAD : %ld]: HTTPHEADER read error\n", pthread_self());
                close(ttpn->n_clientFd);
                continue;
            }
        }

        LOG_DBG("[THREAD : %ld]: Get %d HTTPHeader:\n", pthread_self(), ttpn->n_clientFd);
        if (DBGSIGN) HTTPHeader_fd_print(ttpn->n_hh, 1);

        APINode *an = API_getAN(g_API, ttpn->n_hh->n_HTTPHeaderLine->n_url);
        if (an) sign = processAPINode(ttpn, an, sqlConnection);
        else sign = getFile(ttpn);

        processMes_free(sign, ttpn, tqpv);
    }
}
