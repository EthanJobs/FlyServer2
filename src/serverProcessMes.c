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
#include <coreMesCntl.h>
#include <coreIterator.h>
#include <serverProcessMes.h>
#include <serverTaskQueuePool.h>
#include <serverTaskTreePool.h>
#include <serverAPI.h>

extern int g_epfd;
extern taskQueuePool *g_tqp;
extern taskTreePool *g_ttp;
extern API *g_API;

// TODO：GET获取文件
int getFile(taskTreePoolValue *ttpv) {
    // 目前只支持HTTP1.0
    if (strncmp(ttpv->n_hh->n_HTTPHeaderLine->n_version, "HTTP/1", 6)) return 0;

    // 判断method
    if (strcmp("GET", ttpv->n_hh->n_HTTPHeaderLine->n_method)) return 0;

    // 首次获取初始化
    if (ttpv->n_fileFd == -1) {
        // 如果是/则返回主页
        if (!strcmp(ttpv->n_hh->n_HTTPHeaderLine->n_url, "/")) {
            ttpv->n_fileFd = open(WEB_INDEX, O_RDONLY);
            if (ttpv->n_fileFd > 0) dprintf(ttpv->n_clientFd, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", MIME_getFileMIME(WEB_INDEX));
        }

        // 打开目标文件
        if (ttpv->n_fileFd <= 0) {
            int pathLen = strlen(WEB_PATH);
            char *path = (char *)malloc(sizeof(char) * (pathLen + strlen(ttpv->n_hh->n_HTTPHeaderLine->n_url) + 1));
            strcpy(path, WEB_PATH);
            strcpy(path + pathLen, ttpv->n_hh->n_HTTPHeaderLine->n_url);
            ttpv->n_fileFd = open(path, O_RDONLY);
            if (ttpv->n_fileFd > 0) dprintf(ttpv->n_clientFd, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", MIME_getFileMIME(path));
            free(path);
        }

        // 如果找不到文件就返回WEB_INDEX文件
        if (ttpv->n_fileFd <= 0) {
            ttpv->n_fileFd = open(WEB_INDEX, O_RDONLY);
            if (ttpv->n_fileFd > 0) dprintf(ttpv->n_clientFd, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", MIME_getFileMIME(WEB_INDEX));

            if (ttpv->n_fileFd <= 0) {
                dprintf(ttpv->n_clientFd, "HTTP/1.1 404 Not Found\r\n\r\n");
                return 0;
            }
        }
    }

    if (MesCntl_getHostFileToFd(ttpv->n_fileFd, ttpv->n_clientFd, &ttpv->n_fileIndex, BUFFER_SIZE) == -1) {
        if (errno == EAGAIN)
            return 1;
    }

    return 0;
}

// TODO: 判断参数
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

// TODO: sql处理
int processSQL(taskTreePoolValue *ttpv, APINode *an, Json *json, MYSQL *sqlConnection) {
    char *sql;
    int index = 0, tmp = 0, sqlLen = 0, i = 0;

    for (; i < strlen(an->n_sql); i++) {
        if (an->n_sql[i] == '$' && an->n_sql[i + 1] == '{') {
            tmp = sqlLen;
            sqlLen += i - index;

            if (!index) sql = (char *)malloc(sizeof(char) * sqlLen);
            else sql = (char *)realloc(sql, sizeof(char) * sqlLen);

            strncpy(sql + tmp, an->n_sql + index, i - index);
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

    dprintf(ttpv->n_clientFd, "HTTP/1.1 200 OK\r\n\r\n");
    sendSqlRes(sqlConnection, sql, ttpv->n_clientFd);
    LOG_DBG("[THREAD : %ld]: \"%s\" is executed\n", pthread_self(), sql);

    free(sql);
    return 0;
}

// TODO: 预设数据处理
int processAPINode(taskTreePoolValue *ttpv, APINode *an, MYSQL *sqlConnection) {
    // 跨域判断
    if (!strcmp("OPTIONS", ttpv->n_hh->n_HTTPHeaderLine->n_method)) {
        dprintf(ttpv->n_clientFd, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: POST\r\n\r\n");
        return 0;
    }

    if (an->fun_handle) return an->fun_handle(ttpv, an, sqlConnection);

    // 目前只支持HTTP1.0
    if (strncmp(ttpv->n_hh->n_HTTPHeaderLine->n_version, "HTTP/1", 6)) return 0;

    // 判断method
    if (strcmp(an->n_method, ttpv->n_hh->n_HTTPHeaderLine->n_method)) return 0;

    int sign;

    Json *json = Json_initByFd(ttpv->n_clientFd);
    LOG_DBG("[SERVER]: Get %d JSON: \n", ttpv->n_clientFd);
    if (DBGSIGN) Json_displayValue(json);

    if (!judgeJsonType(an, json)) sign = processSQL(ttpv, an, json, sqlConnection);

    Json_free(json);
    return sign;
}

// TODO: 结束处理
void processMes_free(int sign, taskTreePoolValue *ttpv, taskQueuePoolValue *tqpv) {
    if (sign == 0) {
        // 结束
        HTTPHeader_free(ttpv->n_hh);
        ttpv->n_hh = NULL;
        if (ttpv->n_fileFd > 0) close(ttpv->n_fileFd);
        ttpv->n_fileFd = -1;
        ttpv->n_fileIndex = 0;
        LOG_DBG("[THREAD : %ld]: %d get close\n", pthread_self(), ttpv->n_clientFd);
        close(ttpv->n_clientFd);
    } else if (sign == 1) {
        // 未读完文件
        struct epoll_event ev;
        ev.events = EPOLLOUT | EPOLLONESHOT | EPOLLET;
        ev.data.fd = ttpv->n_clientFd;
        epoll_ctl(g_epfd, EPOLL_CTL_MOD, ttpv->n_clientFd, &ev);
    } else if (sign == 2) {
        // 未写完文件
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
        ev.data.fd = ttpv->n_clientFd;
        epoll_ctl(g_epfd, EPOLL_CTL_MOD, ttpv->n_clientFd, &ev);
    }

    taskQueuePoolValue_free(tqpv);
}

// TODO: 数据处理
void *processMes(void *arg) {
    MYSQL *sqlConnection = MYSQL_init(HOST, USERNAME, PASSWORD, DATABASE, 0);
    int sign;

    while (1) {
        taskQueuePoolValue *tqpv = taskQueuePool_pop(g_tqp);
        taskTreePoolValue *ttpv = taskTreePool_get(g_ttp, tqpv->n_clientFd);
        LOG_DBG("[THREAD : %ld]: Get %d fd\n", pthread_self(), tqpv->n_clientFd);

        if (ttpv == NULL) {
            taskTreePool_insert(g_ttp, tqpv->n_clientFd, -1, 0, NULL);
            ttpv = taskTreePool_get(g_ttp, tqpv->n_clientFd);
        }

        if (ttpv->n_hh == NULL) {
            ttpv->n_hh = HTTPHeader_fd_init(ttpv->n_clientFd);
            if (ttpv->n_hh == NULL) {
                LOG_DBG("[THREAD : %ld]: HTTPHEADER read error\n", pthread_self());
                close(ttpv->n_clientFd);
                continue;
            }
        }

        LOG_DBG("[THREAD : %ld]: Get %d HTTPHeader:\n", pthread_self(), ttpv->n_clientFd);
        if (DBGSIGN)
            HTTPHeader_fd_print(ttpv->n_hh, 1);

        APINode *an = API_getAN(g_API, ttpv->n_hh->n_HTTPHeaderLine->n_url);
        if (an) sign = processAPINode(ttpv, an, sqlConnection);
        else sign = getFile(ttpv);

        processMes_free(sign, ttpv, tqpv);
    }
}
