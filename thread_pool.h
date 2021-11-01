//
// Created by Ysw on 2021/10/31.
//

#include "task_manager.h"

#ifndef NYU_F21_OS_TY_LAB3_THREAD_POOL_H
#define NYU_F21_OS_TY_LAB3_THREAD_POOL_H

typedef struct ThreadPool{
    pthread_mutex_t *task_queue_lock;
    pthread_t *threads;
    long thread_count;

    task_queue *task_head;
    task_queue *task_tail;
    int remain_task;

    char **result;
    sem_t **result_lock;
}thread_pool;

#endif //NYU_F21_OS_TY_LAB3_THREAD_POOL_H


int init_thread_pool(long thread_count, char **argv);
