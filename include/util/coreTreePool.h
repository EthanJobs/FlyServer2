#ifndef _CORETREEPOOL_H
#define  _CORETREEPOOL_H

#include <coreTree.h>
#include <pthread.h>

// 任务树池
typedef struct TreePool {
    Tree *n_tree;
    pthread_mutex_t n_lock;
} TreePool;

TreePool *TreePool_init(int (*fun_compareValue)(void *, void *), void (*fun_free)(void *));
int TreePool_insert(TreePool *tp, void *tpv);
void *TreePool_get(TreePool *tp, int clientFd, int (*fun_compare)(void *, void *));
void TreePool_display(TreePool *tp, int tmp, void (*fun_print)(void *));
void TreePool_free(TreePool *tp);

#endif