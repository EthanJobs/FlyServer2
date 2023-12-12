#include <signal.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <coreDate.h>
#include <coreLog.h>
#include <serverInit.h>
#include <serverTaskQueuePool.h>
#include <serverTaskTreePool.h>
#include <serverProcessMes.h>
#include <serverAPI.h>
#include <serverCustomAPI.h>

int g_epfd;
struct epoll_event g_ev, g_events[EPOLL_SIZE];
int g_server_sockfd;                            //服务端套接字描述符
int g_client_sockfd;                            //客户端套接字描述符
char g_str[INET_ADDRSTRLEN];
struct sockaddr_in g_server_addr;               //服务端套接字信息
struct sockaddr_in g_client_addr;               //客户端套接字信息
taskQueuePool *g_tqp;
taskTreePool *g_ttp;
API *g_API;

// 日志输出测试
void log_init() {
    LOG_COMMON("[SERVER]: LOG TEST\n");
    LOG_ERROR("[SERVER]: LOGERROR TEST\n");
    LOG_DBG("[SERVER]: DBG TEST\n");

    return;
}

// 线程信号屏蔽
void threadSige_init() {
    signal(SIGPIPE, SIG_IGN);

    sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGPIPE);
    int rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0) {
        LOG_ERROR("[SERVER]: Block sigpipe error\n");
        exit(1);
    }
}

// 套接字绑定与EPOLL初始化
void serverEpollBind_init() {
    /*获取服务端套接字描述符*/
    g_server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_server_sockfd == -1) {
        LOG_ERROR("[SERVER]: Socket error\n");
        exit(1);
    }

    // epoll
    g_epfd = epoll_create(EPOLL_SIZE);
    g_ev.events = EPOLLIN;// | EPOLLET;
    g_ev.data.fd = g_server_sockfd;
    epoll_ctl(g_epfd, EPOLL_CTL_ADD, g_server_sockfd, &g_ev);

    /*初始化服务端套接字信息*/
    memset(&g_server_addr, 0, sizeof(struct sockaddr_in));
    g_server_addr.sin_family = AF_INET; //使用IPv4
    g_server_addr.sin_port = htons(PORT); //对端口进行字节转化
    g_server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //对IP地址进行格式化转化

    /*因为最后需要服务器主动关闭连接，所以要设置服务器套接字为可重用*/
    int option = 1;
    setsockopt(g_server_sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /*绑定服务器套接字信息*/
    if (bind(g_server_sockfd, (struct sockaddr *)(&g_server_addr), sizeof(struct sockaddr_in)) < 0) {
        LOG_ERROR("[SERVER]: Bind error\n");
        exit(1);
    }

    /*将服务器套接字转化为被动监听*/
    if (listen(g_server_sockfd, 3) < 0) {
        LOG_ERROR("[SERVER]: Listen error\n");
        exit(1);
    }
}

// 多线程初始化
void taskPool_init() {
    g_tqp = taskQueuePool_init();
    g_ttp = taskTreePool_init();

    // 多线程
    pthread_t tid;
    for (int i = 0; i < PTHREAD_MAX; i++) {
        pthread_create((&tid), NULL, processMes, NULL);
        pthread_detach(tid);
        LOG_COMMON("[SERVER]: New thread is %#lx\n", tid);
    }
}

// APIs初始化
void GAPI_init() {
    g_API = API_init(API_PATH);
    API_customInit();
    LOG_DBG("[SERVER]: API INIT: \n");
    if (DBGSIGN)
        API_display(g_API);
}

void taskPool_free() {
    taskQueuePool_free(g_tqp);
    taskTreePool_free(g_ttp);
}

void GAPI_free() {
    API_free(g_API);
}