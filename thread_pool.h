//
// Created by Ysw on 2021/10/31.
//

#include "task_manager.h"
#include "nyuenc.h"

#ifndef NYU_F21_OS_TY_LAB3_THREAD_POOL_H
#define NYU_F21_OS_TY_LAB3_THREAD_POOL_H

typedef struct ThreadPool{


    long task_count;
    pthread_mutex_t *task_count_lock;

    sem_t *all_task_finished;
    int task_submission_finished;
    pthread_mutex_t *task_submission_finished_lock;

    pthread_mutex_t *thread_count_lock;
    pthread_t *worker_threads;
    long thread_count;

    pthread_t result_handling_thread;

    pthread_mutex_t *task_queue_lock;
    task_queue *task_head;
    task_queue *task_tail;

    sem_t *remain_task;


    unsigned char *result[RESULT_BUFFER_SIZE];
    sem_t *read_result[RESULT_BUFFER_SIZE];
    sem_t *write_result[RESULT_BUFFER_SIZE];


}thread_pool;

#endif //NYU_F21_OS_TY_LAB3_THREAD_POOL_H


void init_thread_pool(long thread_count, int argc, char **argv);
