#ifndef _COREHTTPHEADER_H
#define _COREHTTPHEADER_H

#include <coreTree.h>
#define BUFSIZE 1024

typedef struct HTTPHeaderLine {
    char *n_method;
    char *n_url;
    char *n_version;
} HTTPHeaderLine;

typedef struct HTTPHeaderValue {
    char *n_header;
    char *n_value;
} HTTPHeaderValue;

typedef struct HTTPHeader {
    HTTPHeaderLine *n_HTTPHeaderLine;
    Tree *n_HTTPHeaderValues;
} HTTPHeader;

HTTPHeader *HTTPHeader_fd_init(int fd);
void HTTPHeaderValue_fd_print(HTTPHeaderValue *hhv, int fd);
void HTTPHeader_fd_print(HTTPHeader *hh, int fd);
int HTTPHeader_insertHeaderValue(HTTPHeader *hh, char *header, char *value);
char *HTTPHeader_getHeaderValue(HTTPHeader *hh, char *header);
void HTTPHeader_free(HTTPHeader *hh);

/* // requestHeaderValue
requestHeaderValue *requestHeaderValue_init(char *header, char *value);
void requestHeaderValue_print(requestHeaderValue *rhv);
void requestHeaderValue_fd_print(requestHeaderValue *rhv, int fd);
void requestHeaderValue_free(requestHeaderValue *rhv);

// HTTPHeader
HTTPHeader *HTTPHeader_init();
HTTPHeader *HTTPHeader_fd_init(int fd);
int HTTPHeader_insertRequestHeader(HTTPHeader *hh, requestHeaderValue *rhv);
char *HTTPHeader_getRequestHeader(HTTPHeader *hh, char *header);
void HTTPHeader_print(HTTPHeader *hh);
void HTTPHeader_fd_print(HTTPHeader *hh, int fd);
void HTTPHeader_free(HTTPHeader *hh); */

#endif

/*
 *  requestHeaderValue
 *      requestHeaderValue *requestHeaderValue_init(char *header, char *value);
 *          创建相应的一条消息头,失败返回NULL
 *
 *      void requestHeaderValue_print(requestHeaderValue *rhv);
 *          输出消息头内容
 *
 *      void requestHeaderValue_free(requestHeaderValue *rhv);
 *          释放消息头结构
 *
 *  HTTPHeader
 *      HTTPHeader *HTTPHeader_init();
 *          创建一个空的http协议结构,失败返回NULL
 *
 *      HTTPHeader *HTTPHeader_fd_init(int fd);
 *          根据一个文件描述符创建相应的http结构,失败返回NULL
 *
 *      int HTTPHeader_insertRequestHeader(HTTPHeader *hh, requestHeaderValue *rhv);
 *          插入新的消息头结构,成功返回0.失败返回-1
 *
 *      void HTTPHeader_print(HTTPHeader *hh);
 *          输出一个http协议结构
 *
 *      void HTTPHeader_free(HTTPHeader *hh);
 *          释放一个http协议结构
 *
 */
