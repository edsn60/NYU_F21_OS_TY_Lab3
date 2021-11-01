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
    char *task = (char*) malloc(sizeof(char) * 4097);
    if (!task){
        fprintf(stderr, "Error: malloc failed in execution.c:61\n");
        exit(-1);
    }
    int task_id;
    int times = 1;
    while(1){
        sleep(1);
        pthread_mutex_lock(threadPool->task_queue_lock);

        if (threadPool->remain_task == 0){

            threadPool->thread_count--;
            pthread_mutex_unlock(threadPool->task_queue_lock);
            pthread_exit(NULL);
        }
        else{
            task_id = threadPool->task_head->next->task_id;
            (threadPool->remain_task)--;
            strncpy(task, threadPool->task_head->next->task_string, 4096);
            task[4096] = '\0';
            task_queue *tmp = threadPool->task_head->next;
            threadPool->task_head->next = tmp->next;
            free(tmp);
            pthread_mutex_unlock(threadPool->task_queue_lock);
        }
        times++;

        threadPool->result[task_id] = encoding(task);
        sem_post(threadPool->result_lock[task_id]);
        fprintf(stderr, "thread up %d\n", task_id);
    }
}