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
#include <errno.h>
#include <string.h>

#include "task_manager.h"
#include "thread_pool.h"


extern thread_pool* threadPool;


void submit_task(char *task, int task_id, int isfinished){
    pthread_mutex_lock(threadPool->task_queue_lock);
    if (!threadPool->task_head->next){
        threadPool->task_tail = threadPool->task_head;
    }
//    fprintf(stderr, "submitting task %d\n", task_id);
    threadPool->task_tail->next = (task_queue*) malloc(sizeof(task_queue));
    if (!(threadPool->task_tail->next)){
        fprintf(stderr, "Error: malloc failed in task_manager.c:28\n");
        exit(-1);
    }
    threadPool->task_tail = threadPool->task_tail->next;
    threadPool->task_tail->next = NULL;
    threadPool->task_tail->task_id = task_id;
    threadPool->task_tail->task_string = task;
    pthread_mutex_lock(threadPool->task_count_lock);
    threadPool->task_count++;
    pthread_mutex_unlock(threadPool->task_count_lock);
    if (isfinished){
        pthread_mutex_lock(threadPool->task_submission_finished_lock);
        threadPool->task_submission_finished = 1;
        pthread_mutex_unlock(threadPool->task_submission_finished_lock);
//        fprintf(stderr, "all task submitted %d\n", task_id);
    }
    sem_post(threadPool->remain_task);
//    fprintf(stderr, "submitting task %d\n", task_id);
    pthread_mutex_unlock(threadPool->task_queue_lock);
}


void generate_task(char **argv){
    int task_id = 0;
    struct stat st;
    char *addr;
    off_t page_size = sysconf(_SC_PAGE_SIZE);
    off_t remain_size = page_size;
    char *task = (char *) malloc(sizeof(char) * (page_size + 1));
    for (char **c = &argv[optind]; *c; c++) {
//        fprintf(stderr, "file name: %s\n", *c);

        int fd = open(*c, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Error: failed to open file '%s', ignored\n", *c);
            exit(-1);
        }
        if (fstat(fd, &st) < 0) {
            fprintf(stderr, "Error: fstat error\n");
            exit(-1);
        }
        off_t file_size = st.st_size;
        addr = mmap(0, file_size, PROT_READ, MAP_SHARED, fd, 0);
        while (file_size >= page_size) {
            if (!remain_size) {
                task = (char *) malloc(sizeof(char) * (page_size + 1));
                if (!task){
                    fprintf(stderr, "Error: malloc failed in task_manager.c:74\n");
                    exit(-1);
                }
                strncat(task, addr, page_size);
                file_size -= page_size;
                addr += page_size;
            } else {
                strncat(task, addr, remain_size);
                file_size -= remain_size;
                addr += remain_size;
                remain_size = 0;
            }
            submit_task(task, task_id, 0);
            task_id++;
        }
        if (file_size < page_size) {
            if (!remain_size) {
                task = (char *) malloc(sizeof(char) * (page_size + 1));
                if (!task){
                    fprintf(stderr, "Error: malloc failed in task_manager.c:93\n");
                    exit(-1);
                }
                strncat(task, addr, file_size);
                remain_size = page_size - file_size;
            } else {
                if (remain_size > file_size) {
                    strncat(task, addr, file_size);
                    remain_size -= file_size;
                } else {   // remain_size <= file_size
                    strncat(task, addr, remain_size);
                    submit_task(task, task_id, 0);
                    task_id++;
                    file_size -= remain_size;
                    addr += remain_size;
                    task = (char *) malloc(sizeof(char) * (page_size + 1));
                    if (!task){
                        fprintf(stderr, "Error: malloc failed in task_manager.c:110\n");
                        exit(-1);
                    }
                    strncat(task, addr, file_size);
                    remain_size = page_size - file_size;
                }
            }
        }
    }
    submit_task(task, task_id, 1);
}