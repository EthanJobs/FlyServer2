#ifndef _SERVERINIT_H
#define _SERVERINIT_H

#define EPOLL_SIZE 1024
#define PORT 2002
#define PTHREAD_MAX 1
#define API_PATH "./api"

void log_init();
void threadSige_init();
void serverEpollBind_init();
void taskPool_init();
void GAPI_init();

void taskPool_free();
void GAPI_free();

#endif