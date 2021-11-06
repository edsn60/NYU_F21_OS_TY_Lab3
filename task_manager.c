//
// Created by Ysw on 2021/10/31.
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>

#include "task_manager.h"
#include "thread_pool.h"


extern thread_pool* threadPool;


void submit_task(char *task, int task_id, size_t task_size){
//    pthread_mutex_lock(threadPool->task_queue_lock);
//    if (!threadPool->task_head->next){
//        threadPool->task_tail = threadPool->task_head;
//    }

    threadPool->task_tail->next = (task_queue*) malloc(sizeof(task_queue));
    if (!(threadPool->task_tail->next)){
        fprintf(stderr, "Error: malloc failed in task_manager.c:28\n");
        exit(-1);
    }

    threadPool->task_tail = threadPool->task_tail->next;
    threadPool->task_tail->next = NULL;
    threadPool->task_tail->task_id = task_id;
    threadPool->task_tail->task_string = task;
    threadPool->task_tail->task_size = task_size;
//    pthread_mutex_lock(threadPool->task_count_lock);
    threadPool->task_count++;
//    pthread_mutex_unlock(threadPool->task_count_lock);

//    if (isfinished){
//        pthread_mutex_lock(threadPool->task_submission_finished_lock);
//        threadPool->task_submission_finished = 1;
//        pthread_mutex_unlock(threadPool->task_submission_finished_lock);
//    }
//
//    sem_post(threadPool->remain_task);
//    pthread_mutex_unlock(threadPool->task_queue_lock);
}


void generate_task(int argc, char **argv){
    int task_id = 0;
    struct stat st;
    char *addr;
    off_t page_size = sysconf(_SC_PAGE_SIZE);

    for (int i = 3; i < argc; i ++) {
        char *filename = argv[i];
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Error: failed to open file '%s', ignored\n", filename);
            exit(-1);
        }
        if (fstat(fd, &st) < 0) {
            fprintf(stderr, "Error: fstat error\n");
            exit(-1);
        }
        off_t file_size = st.st_size;
        addr = mmap(0, file_size, PROT_READ, MAP_SHARED, fd, 0);
        while (file_size >= page_size) {
            submit_task(addr, task_id, page_size);
            addr += page_size;
            file_size -= page_size;
            task_id++;
        }
        if (file_size < page_size){
            submit_task(addr, task_id, file_size);
            task_id++;
        }
    }
}