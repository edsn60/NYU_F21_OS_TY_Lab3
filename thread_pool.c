//
// Created by Ysw on 2021/10/31.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>

#include "thread_pool.h"
#include "task_manager.h"
#include "execution.h"
#include "nyuenc.h"


extern thread_pool* threadPool;


void init_thread_pool(long thread_count, char **argv){

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
    threadPool->task_submission_finished_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!threadPool->task_submission_finished_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:37\n");
        exit(-1);
    }
    pthread_mutex_init(threadPool->task_submission_finished_lock, NULL);
    threadPool->task_submission_finished = 0;
    threadPool->task_count = 0;
    threadPool->task_count_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!threadPool->task_count_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:46\n");
        exit(-1);
    }
    pthread_mutex_init(threadPool->task_count_lock, NULL);

    threadPool->task_queue_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!threadPool->task_queue_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:53\n");
        exit(-1);
    }
    pthread_mutex_init(threadPool->task_queue_lock, NULL);

    for (int i = 0; i < RESULT_BUFFER_SIZE; i++){
        threadPool->read_result[i] = (sem_t*) malloc(sizeof(sem_t));
        sem_init(threadPool->read_result[i], 0, 0);
        threadPool->write_result[i] = (sem_t*) malloc(sizeof(sem_t));
        sem_init(threadPool->write_result[i], 0, 1);
//        char c1 = (char)i;
//        char c2 = (char)(i+RESULT_BUFFER_SIZE);
//        threadPool->read_result[i] = sem_open(&c1, O_CREAT, 0644, 0);
//        if (threadPool->read_result[i] == SEM_FAILED){
//            fprintf(stderr, "Error: threadPool->read_result[%d] failed\n", i);
//        }
//        threadPool->write_result[i] = sem_open(&c2, O_CREAT, 0644, 1);
//        if (threadPool->write_result[i] == SEM_FAILED){
//            fprintf(stderr, "Error: threadPool->write_result[%d] failed\n", i);
//        }
    }
    threadPool->all_task_finished = (sem_t*) malloc(sizeof(sem_t));
    sem_init(threadPool->all_task_finished, 0, 0);

    threadPool->remain_task = (sem_t*) malloc(sizeof(sem_t));
    sem_init(threadPool->remain_task, 0, 0);
//
//    threadPool->all_task_finished = sem_open("/all_task_finished", O_CREAT, 0644, 0);
//    if (threadPool->all_task_finished == SEM_FAILED){
//        fprintf(stderr, "Error: threadPool->all_task_finished failed\n");
//    }
//    threadPool->remain_task = sem_open("/remain_task", O_CREAT, 0644, 0);
//    if (threadPool->remain_task == SEM_FAILED){
//        fprintf(stderr, "Error: threadPool->remain_task failed\n");
//    }

    threadPool->thread_count_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!threadPool->task_count_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:79\n");
        exit(-1);
    }
    pthread_mutex_init(threadPool->thread_count_lock, NULL);

    threadPool->thread_count = thread_count;
    threadPool->worker_threads = (pthread_t*) malloc(sizeof(pthread_t) * (thread_count + 1));
    if (!threadPool->worker_threads){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:87\n");
        exit(-1);
    }

    for (int i = 0; i < thread_count; i++){
        pthread_create(&(threadPool->worker_threads[i]), NULL, thread_runner, NULL);
    }

    pthread_create(&(threadPool->result_handling_thread), NULL, collect_result, NULL);

    generate_task(argv);
}