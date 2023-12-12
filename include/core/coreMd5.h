#ifndef _COREMD5_H
#define _COREMD5_H

typedef unsigned int bit32;

bit32 rol(bit32 val, int size);
char *Md5_get(char *data, int bitLen);

#endif

/*
 *
 *  bit32 rol(bit32 val, int size);
 *      循环左移
 *
 *  char *md5(char *data, int bitLen);
 *      输出data的md5值
 *
 */
