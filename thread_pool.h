//
// Created by Ysw on 2021/10/31.
//

#include "task_manager.h"

#ifndef NYU_F21_OS_TY_LAB3_THREAD_POOL_H
#define NYU_F21_OS_TY_LAB3_THREAD_POOL_H

typedef struct ThreadPool{


    long task_count;
    pthread_mutex_t *task_count_lock;

    int task_submission_finished;

    pthread_mutex_t *thread_count_lock;
    pthread_t *worker_threads;
    long thread_count;

    pthread_t result_handling_thread;

    pthread_mutex_t *task_queue_lock;
    task_queue *task_head;
    task_queue *task_tail;

    pthread_mutex_t *remain_task_lock;
//    long remain_task;
    pthread_cond_t *remain_task_cond;

    char *result[100];
    int result_status[100];     // 0: ok to write, 1: ok to read
    pthread_mutex_t *read_lock[100];
    pthread_cond_t *read_cond[100];

    int task_finished;
    pthread_cond_t *all_task_finished;
    pthread_mutex_t *task_finished_lock;

}thread_pool;

#endif //NYU_F21_OS_TY_LAB3_THREAD_POOL_H


int init_thread_pool(long thread_count, char **argv);
