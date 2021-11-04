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
#include "nyuenc.h"

extern thread_pool* threadPool;


void single_thread(char **argv){
    char *filename;
    struct stat st;
    int fd;
    char *addr;
    unsigned char *result;
    unsigned char reserved[3] = {'\0'};
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
        res_len = strlen((char*)result) - 2;

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
    char *task;
    int task_id;
    while(1){
        sem_wait(threadPool->remain_task);


        pthread_mutex_lock(threadPool->task_queue_lock);
        pthread_mutex_lock(threadPool->task_submission_finished_lock);
        if (threadPool->task_submission_finished == 1){
            pthread_mutex_unlock(threadPool->task_submission_finished_lock);
            if (!(threadPool->task_head->next)){
                threadPool->task_tail = threadPool->task_head;
                sem_post(threadPool->remain_task);
                pthread_mutex_unlock(threadPool->task_queue_lock);
                pthread_exit(NULL);
            }
        }
        pthread_mutex_unlock(threadPool->task_submission_finished_lock);

        unsigned char *result;
        task_id = threadPool->task_head->next->task_id;
        task = threadPool->task_head->next->task_string;
        task_queue *tmp = threadPool->task_head->next;
        threadPool->task_head->next = tmp->next;

        if (!threadPool->task_head->next){
            threadPool->task_tail = threadPool->task_head;
        }

        pthread_mutex_unlock(threadPool->task_queue_lock);
        result = encoding(task);
        tmp->next = NULL;
        free(tmp);
        sem_wait(threadPool->write_result[task_id % RESULT_BUFFER_SIZE]);
        threadPool->result[task_id % 100] = result;
        sem_post(threadPool->read_result[task_id % RESULT_BUFFER_SIZE]);
    }
}


void *collect_result(){
    unsigned char reserved[3] = {'\0'};
    unsigned char *result;
    size_t res_len;
    for (long i = 0; ; i++){
        pthread_mutex_lock(threadPool->task_submission_finished_lock);
        if (threadPool->task_submission_finished == 1) {
            pthread_mutex_unlock(threadPool->task_submission_finished_lock);
            pthread_mutex_lock(threadPool->task_count_lock);
            if (i == threadPool->task_count) {
                pthread_mutex_unlock(threadPool->task_count_lock);
                sem_post(threadPool->all_task_finished);
                sem_post(threadPool->remain_task);
                printf("%s", reserved);
                pthread_exit(NULL);
            }
            pthread_mutex_unlock(threadPool->task_count_lock);
        }
        else{
            pthread_mutex_unlock(threadPool->task_submission_finished_lock);
        }
        sem_wait(threadPool->read_result[i % RESULT_BUFFER_SIZE]);
        result = threadPool->result[i % RESULT_BUFFER_SIZE];
        sem_post(threadPool->write_result[i % RESULT_BUFFER_SIZE]);
        res_len = strlen((char*)result) - 2;
        if (reserved[0] == result[0]){
            result[1] += reserved[1];
        }
        else{
            printf("%s", reserved);
        }
        strncpy(reserved, result + res_len, 2);
        result[res_len] = '\0';
        printf("%s", result);
        free(result);
    }
}