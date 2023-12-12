#include <errno.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <coreLog.h>
#include <coreDate.h>
#include <coreMIME.h>
#include <serverTaskQueuePool.h>
#include <serverTaskTreePool.h>
#include <serverStart.h>
#include <serverInit.h>

extern int g_epfd;
extern struct epoll_event g_ev, g_events[EPOLL_SIZE];
extern int g_server_sockfd;                             //服务端套接字描述符
extern int g_client_sockfd;                             //客户端套接字描述符
struct sockaddr_in g_client_addr;                       //客户端套接字信息
extern taskQueuePool *g_tqp;
extern taskTreePool *g_ttp;

void server_start() {
    log_init();
    threadSige_init();
    serverEpollBind_init();
    taskPool_init();
    GAPI_init();
    MIME_init();

    LOG_COMMON("[SERVER]: Server is waiting\n");
    while (1) {
        int nfds = epoll_wait(g_epfd, g_events, EPOLL_SIZE, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            LOG_ERROR("[SERVER]: Epoll_wait error\n");
            exit(1);
        }

        for (int i = 0; i < nfds; i++) {
            if (g_events[i].data.fd == g_server_sockfd) {
                socklen_t len = sizeof(struct sockaddr_in);
                g_client_sockfd = accept(g_server_sockfd, (struct sockaddr *)&g_client_addr, &len);
                if (g_client_sockfd < 0) {
                    LOG_ERROR("[SERVER]: Accept error\n");
                    exit(1);
                }

                int flag = fcntl(g_client_sockfd, F_GETFL);
                flag |= O_NONBLOCK;
                fcntl(g_client_sockfd, F_SETFL, flag);

                g_ev.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
                g_ev.data.fd = g_client_sockfd;
                epoll_ctl(g_epfd, EPOLL_CTL_ADD, g_client_sockfd, &g_ev);
            } else {
                int clifd = g_events[i].data.fd;
                if (clifd < 3) continue;
                taskQueuePool_push(g_tqp, clifd);
            }
        }
    }

    taskPool_free();
    GAPI_free();
    MIME_free();
}
