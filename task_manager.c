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

#include "task_manager.h"
#include "thread_pool.h"


extern thread_pool* threadPool;


int submit_task(char **argv){
    int task_id = 0;
    struct stat st;
    char *addr;
    off_t page_size = sysconf(_SC_PAGE_SIZE);
    for (char **c = &argv[optind]; *c; c++){
        off_t offset = 0;

        int fd = open(*c, O_RDONLY);
        if (fd == -1){
            fprintf(stderr, "Error: failed to open file '%s', ignored\n", *c);
            exit(-1);
        }
        if (fstat(fd, &st) < 0){
            fprintf(stderr, "Error: fstat error\n");
            exit(-1);
        }
        off_t file_size = st.st_size;
        while (file_size - offset >= page_size){
            addr = mmap(0, page_size, PROT_READ, MAP_SHARED, fd, offset);
            if (addr == MAP_FAILED){
                fprintf(stderr, "Error: mmap failed in 1\n");
                exit(-1);
            }
            pthread_mutex_lock(threadPool->task_queue_lock);
            if (!threadPool->task_head->next){
                threadPool->task_tail = threadPool->task_head;
            }
            fprintf(stderr, "submitting task %d\n", task_id);
            threadPool->task_tail->next = (task_queue*) malloc(sizeof(task_queue));
            if (!(threadPool->task_tail->next)){
                fprintf(stderr, "Error: malloc failed in task_manager.c:39\n");
                exit(-1);
            }
            threadPool->task_tail = threadPool->task_tail->next;
            threadPool->task_tail->next = NULL;
            threadPool->task_tail->task_id = task_id;
            threadPool->task_tail->task_string = addr;
            pthread_mutex_lock(threadPool->task_count_lock);
            threadPool->task_count++;
            pthread_mutex_unlock(threadPool->task_count_lock);
//            threadPool->remain_task++;
            pthread_cond_signal(threadPool->remain_task_cond);
            pthread_mutex_unlock(threadPool->task_queue_lock);

            fprintf(stderr, "task %d submitted\n", task_id);
            offset += page_size;
            task_id++;
        }
        if (offset < file_size){
            size_t len = file_size-offset;
            addr = mmap(0, len, PROT_READ, MAP_SHARED, fd, offset);
            if (addr == MAP_FAILED){
                fprintf(stderr, "Error: mmap failed in task_manager.c: 62\n");
                exit(-1);
            }
            pthread_mutex_lock(threadPool->task_queue_lock);
            fprintf(stderr, "submitting task %d\n", task_id);
            threadPool->task_tail->next = (task_queue*) malloc(sizeof(task_queue));
            if (!(threadPool->task_tail->next)){
                fprintf(stderr, "Error: malloc failed in task_manager.c:58\n");
                exit(-1);
            }
            threadPool->task_tail = threadPool->task_tail->next;
            threadPool->task_tail->next = NULL;
            threadPool->task_tail->task_id = task_id;
            threadPool->task_tail->task_string = addr;
            pthread_mutex_lock(threadPool->task_count_lock);
            threadPool->task_count++;
            pthread_mutex_unlock(threadPool->task_count_lock);
//            threadPool->remain_task++;

            threadPool->task_submission_finished = 1;
            pthread_cond_signal(threadPool->remain_task_cond);
            pthread_mutex_unlock(threadPool->task_queue_lock);
            fprintf(stderr, "task %d submitted\n", task_id);
            task_id++;
        }
        close(fd);
    }
    fprintf(stderr, "All tasks submitted, total: %d\n", task_id);
    return task_id;
}