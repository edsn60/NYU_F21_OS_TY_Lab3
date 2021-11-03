//
// Created by Ysw on 2021/10/31.
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <errno.h>

#include "thread_pool.h"
#include "execution.h"
#include "encoder.h"
#include "task_manager.h"


extern thread_pool* threadPool;


void single_thread(char **argv){
    char *filename;
    struct stat st;
    int fd;
    char *addr;
    char *result;
    char reserved[3] = {'\0'};
    size_t res_len;
    for (char **file_ptr = &argv[optind]; *file_ptr; file_ptr++){

        filename = *file_ptr;
        fd = open(filename, O_RDONLY);
        if (fstat(fd, &st) < 0){
            fprintf(stderr, "Error: fstat error\n");
            exit(-1);
        }
        addr = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        result = encoding(addr);
        res_len = strlen(result) - 2;

        if (reserved[0] == result[0]){
            result[1] += reserved[1];
        }
        else{
            printf("%s", reserved);
        }
        strncpy(reserved, result + res_len, 2);
        result[res_len] = '\0';
        printf("%s", result);
    }
    printf("%s", reserved);
}


void *thread_runner(){
    int status;
    char *task = (char*) malloc(sizeof(char) * 4097);
    if (!task){
        fprintf(stderr, "Error: malloc failed in execution.c:61\n");
        exit(-1);
    }
    int task_id;
    while(1){
        char *result;
        pthread_mutex_lock(threadPool->task_queue_lock);
        if (!(threadPool->task_head->next)){
            pthread_cond_wait(threadPool->remain_task_cond, threadPool->task_queue_lock);
            if (threadPool->task_submission_finished == 1){
                pthread_mutex_lock(threadPool->thread_count_lock);
                threadPool->thread_count--;
                pthread_mutex_unlock(threadPool->thread_count_lock);
                pthread_cond_signal(threadPool->remain_task_cond);
                pthread_mutex_unlock(threadPool->task_queue_lock);
                fprintf(stderr, "thread: %u unlocked task queue\n", pthread_self());

                fprintf(stderr, "thread: %u exited\n", pthread_self());
                pthread_exit(NULL);
            }
        }
        else{
            pthread_cond_wait(threadPool->remain_task_cond, threadPool->task_queue_lock);
        }
        fprintf(stderr, "thread: %u locked task queue\n", pthread_self());
        task_id = threadPool->task_head->next->task_id;     // TODO:: bug
        strncpy(task, threadPool->task_head->next->task_string, sizeof(char) * 4096);
        task_queue *tmp = threadPool->task_head->next;
        threadPool->task_head->next = tmp->next;
//        threadPool->remain_task--;
        if (!threadPool->task_head->next){
            threadPool->task_tail = threadPool->task_head;
        }
        pthread_mutex_unlock(threadPool->task_queue_lock);
        fprintf(stderr, "thread: %u unlocked task queue\n", pthread_self());
        task[4096] = '\0';
        free(tmp);
        result = encoding(task);
        pthread_mutex_lock(threadPool->read_lock[task_id % 100]);
        if (threadPool->result_status[task_id % 100] == 1){
            pthread_cond_wait(threadPool->read_cond[task_id % 100], threadPool->read_lock[task_id % 100]);
        }

        threadPool->result[task_id % 100] = result;
        threadPool->result_status[task_id % 100] = 1;
        pthread_cond_signal(threadPool->read_cond[task_id % 100]);
        pthread_mutex_unlock(threadPool->read_lock[task_id % 100]);
        fprintf(stderr, "thread: %u write result: %d\n", pthread_self(), task_id);
    }
}


void *collect_result(){
    char reserved[3];
    char *result;
    size_t res_len;
    for (long i = 0; ; i++){
        pthread_mutex_lock(threadPool->task_count_lock);
        if (i == threadPool->task_count){
            pthread_mutex_unlock(threadPool->task_count_lock);
            pthread_mutex_lock(threadPool->thread_count_lock);
            if (threadPool->thread_count == 0){
                pthread_mutex_unlock(threadPool->thread_count_lock);
//                printf("%s", reserved);
                pthread_mutex_lock(threadPool->task_finished_lock);
                threadPool->task_finished = 1;
                pthread_cond_signal(threadPool->all_task_finished);
                pthread_mutex_unlock(threadPool->task_finished_lock);

                fprintf(stderr, "result thread exited\n");
                pthread_exit(NULL);
            }
            pthread_mutex_unlock(threadPool->thread_count_lock);
        }

        pthread_mutex_unlock(threadPool->task_count_lock);
        pthread_mutex_lock(threadPool->read_lock[i % 100]);
        fprintf(stderr, "result thread locked result\n");
        if (threadPool->result_status[i % 100] == 0){
            fprintf(stderr, "result thread waiting on result, unlocked\n");
            pthread_cond_wait(threadPool->read_cond[i % 100], threadPool->read_lock[i % 100]);
            fprintf(stderr, "result thread locked result again\n");
        }
        fprintf(stderr, "result thread reading result %ld\n", i);
        result = threadPool->result[i % 100];
        threadPool->result_status[i % 100] = 0;
        pthread_cond_signal(threadPool->read_cond[i % 100]);
        pthread_mutex_unlock(threadPool->read_lock[i % 100]);
        fprintf(stderr, "result thread unlocked result\n");
        res_len = strlen(result) - 2;
        if (reserved[0] == result[0]){
            result[1] += reserved[1];
        }
        else{
//            printf("%s", reserved);
        }
        strncpy(reserved, result + res_len, 2);
        result[res_len] = '\0';
//        printf("%s", result);
    }
}