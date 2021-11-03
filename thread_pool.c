//
// Created by Ysw on 2021/10/31.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>

#include "thread_pool.h"
#include "task_manager.h"
#include "execution.h"


extern thread_pool* threadPool;


int init_thread_pool(long thread_count, char **argv){

    threadPool = (thread_pool*) malloc(sizeof(thread_pool));
    if (!threadPool){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:23\n");
        exit(-1);
    }
    threadPool->task_head = (task_queue*) malloc(sizeof(task_queue));
    if (!threadPool->task_head){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:28\n");
        exit(-1);
    }
    threadPool->task_head->task_string = NULL;
    threadPool->task_head->next = NULL;
    threadPool->task_head->task_id = -1;
    threadPool->task_tail = threadPool->task_head;
    threadPool->task_submission_finished = 0;
    threadPool->task_count = 0;
    threadPool->task_count_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(threadPool->task_count_lock, NULL);

    threadPool->task_queue_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(threadPool->task_queue_lock, NULL);

//    threadPool->remain_task = 0;
    threadPool->remain_task_cond = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
    pthread_cond_init(threadPool->remain_task_cond, NULL);
    threadPool->remain_task_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(threadPool->remain_task_lock, NULL);
    memset(threadPool->result_status, 0, sizeof(int));
    for (int i = 0; i < 100; i++){
        threadPool->read_lock[i] = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
         pthread_mutex_init(threadPool->read_lock[i], NULL);
        threadPool->read_cond[i] = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
        pthread_cond_init(threadPool->read_cond[i], NULL);
    }
    threadPool->task_finished = 0;
    threadPool->task_finished_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    threadPool->all_task_finished = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));

    threadPool->thread_count_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(threadPool->thread_count_lock, NULL);

    threadPool->thread_count = thread_count;
    threadPool->worker_threads = (pthread_t*) malloc(sizeof(pthread_t) * (thread_count + 1));
    threadPool->worker_threads[thread_count] = 0;
    if (!threadPool->worker_threads){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:71\n");
        exit(-1);
    }

    for (int i = 0; i < thread_count; i++){
        pthread_create(&(threadPool->worker_threads[i]), NULL, thread_runner, NULL);
    }

    pthread_create(&(threadPool->result_handling_thread), NULL, collect_result, NULL);

    int task_count = submit_task(argv);

    return task_count;
}