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


/**To submit the task to task queue mutual exclusively
 *
 * @param task:: string needed to be compressed
 * @param task_id:: id of the task
 * @param task_size:: length of ```task```
 * @param is_finished:: if it is the last task(1 yes, 0 no)
 */
void submit_task(char *task, int task_id, size_t task_size, int is_finished){
    pthread_mutex_lock(threadPool->task_queue_lock);    // grab the task queue lock
    if (!threadPool->task_head->next){
        threadPool->task_tail = threadPool->task_head;
    }

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
    pthread_mutex_lock(threadPool->task_count_lock);    // grab the task count lock
    threadPool->task_count++;
    pthread_mutex_unlock(threadPool->task_count_lock);  // release the task count lock

    if (is_finished){   // if last task
        pthread_mutex_lock(threadPool->task_submission_finished_lock);
        threadPool->task_submission_finished = 1;
        pthread_mutex_unlock(threadPool->task_submission_finished_lock);
    }
    sem_post(threadPool->remain_task);      // notify worker threads new task is ready
    pthread_mutex_unlock(threadPool->task_queue_lock);      //release the task queue lock
}


/**To generate the task from the mapped address one by one
 *
 */
void generate_task(){
    int task_id = 0;
    char *addr;
    off_t page_size = sysconf(_SC_PAGE_SIZE);   // 4096
    off_t file_size;
    if (!threadPool->file_info){
        fprintf(stderr, "No file info\n");
    }
    for (fileinfo **file_info = threadPool->file_info; *file_info; file_info++){
        addr = (*file_info)->addr;
        file_size = (*file_info)->size;
        while (file_size > page_size) {
            submit_task(addr, task_id, page_size, 0);
            addr += page_size;
            file_size -= page_size;
            task_id++;
        }
        if (!*(file_info+1)){
            submit_task(addr, task_id, file_size, 1);
        }
        else{
            submit_task(addr, task_id, file_size, 0);
            task_id++;
        }
    }
}


/**Map all the files one by one and construct a struct array to temporarily store the mapped information
 *
 * @param argc:: number of commandline arguments
 * @param argv:: list of commandline arguments
 * @return:: total size of all the files
 */
off_t map_files(int argc, char **argv){
    off_t total_size = 0;
    struct stat st;
    fileinfo **file_info = (fileinfo**) malloc(sizeof(fileinfo*) * (argc - 2));
    file_info[argc-3] = NULL;
    fileinfo **file = file_info;
    for (char** arg = &argv[optind]; *arg; arg++){
        *file = (fileinfo*) malloc(sizeof(fileinfo));
        int fd = open(*arg, O_RDONLY);
        if (fd == -1){
            fprintf(stderr, "Error: failed to open file '%s', ignored\n", *arg);
            exit(-1);
        }
        if (fstat(fd, &st) < 0) {
            fprintf(stderr, "Error: fstat error\n");
            exit(-1);
        }
        off_t file_size = st.st_size;
        total_size += file_size;
        (*file)->size = file_size;
        (*file)->addr = mmap(0, file_size, PROT_READ, MAP_SHARED, fd, 0);
        file++;
    }
    threadPool->file_info = file_info;
    return total_size;
}