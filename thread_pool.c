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

    int task_count = submit_task(argv, &(threadPool->task_tail));

    threadPool->task_queue_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!threadPool->task_queue_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:40\n");
        exit(-1);
    }
    pthread_mutex_init(threadPool->task_queue_lock, NULL);

    threadPool->remain_task = task_count;

    threadPool->result = (char**) malloc(sizeof(char*) * (task_count + 1));
    if (!threadPool->result){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:50\n");
        exit(-1);
    }
    threadPool->result[task_count] = NULL;
    threadPool->result_lock = (sem_t**) malloc(sizeof(sem_t*) * (task_count + 1));
    if (!threadPool->result_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:55\n");
        exit(-1);
    }
    char c = '0';
    for (int i = 0; i < task_count; i++){
        threadPool->result_lock[i] = sem_open(&c, O_CREAT, 0644, 0);
        if (threadPool->result_lock[i] == SEM_FAILED){
            fprintf(stderr, "Error: sem_open failed, errno: %d\n", errno);
            exit(-1);
        }
    }
    threadPool->result_lock[task_count] = NULL;

    threadPool->thread_count = thread_count;
    threadPool->threads = (pthread_t*) malloc(sizeof(pthread_t) * thread_count);
    if (!threadPool->threads){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:71\n");
        exit(-1);
    }
    for (int i = 0; i < thread_count; i++){
        pthread_create(&(threadPool->threads[i]), NULL, thread_runner, NULL);
    }
    return task_count;
}