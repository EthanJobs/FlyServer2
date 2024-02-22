#ifndef _COREMESCNTL_H
#define _COREMESCNTL_H

#define MES_READ_MAX_SIZE 1024

int MesCntl_getHostFileToFd(int filefd, int writefd, int *index, int max_size);
int MesCntl_getFdToHostFile(int readfd, int filefd, int *index, int max_size);

#endif

/*
 *  Mes_getHostFileToFd(const char *path, char *date, int sendfd);
 *      将本地文件发送至目标fd
 *
 *  Mes_getFdToHostFile(const char *path, char *mes, char *name);
 *      将目标fd保存到本地文件
 *
 */