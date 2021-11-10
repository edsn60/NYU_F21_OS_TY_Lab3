//
// Created by Ysw on 2021/10/31.
//

#include "task_manager.h"
#include "nyuenc.h"

#ifndef NYU_F21_OS_TY_LAB3_THREAD_POOL_H
#define NYU_F21_OS_TY_LAB3_THREAD_POOL_H

typedef struct ThreadPool{


    long task_count;    // represents the number of tasks submitted, used for read thread to judge if it has read all results
    pthread_mutex_t *task_count_lock;

    sem_t *all_task_finished;   // the semaphore used to wake the main thread after all tasks are done
    int task_submission_finished;   // represents if there will be more tasks
    pthread_mutex_t *task_submission_finished_lock;

    pthread_mutex_t *thread_count_lock;
    pthread_t *worker_threads;  // array of worker threads
    long thread_count;

    pthread_t result_handling_thread;   // thread specifically used to collect results

    pthread_mutex_t *task_queue_lock;
    task_queue *task_head;     // task queue
    task_queue *task_tail;

    sem_t *remain_task;    // semaphore used to notify worker threads if there are tasks to do

    unsigned char **result;    // array of results
    sem_t *result_sem;     // semaphore to notify read thread if the results are ready

    fileinfo **file_info;     // temporarily store the info of mapped files

}thread_pool;

#endif //NYU_F21_OS_TY_LAB3_THREAD_POOL_H


void init_thread_pool(long thread_count, int argc, char **argv);
void destroy_thread_pool(long threads_count);