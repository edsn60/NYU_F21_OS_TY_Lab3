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


/**Function used for single thread to encode
 *
 * @param argv: list of commandline arguments
 */
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
        result = encoding(addr, st.st_size);
        res_len = strlen((char*)result) - 2;

        if (reserved[0] == result[0]){
            result[1] += reserved[1];
        }
        else{
            printf("%s", reserved);
        }
        reserved[0] = *(result + res_len);
        reserved[1] = *(result + res_len + 1);
        *(result+res_len) = '\0';
        printf("%s", result);
    }
    printf("%s", reserved);
}


/**Worker threads controller
 *
 */
void *thread_controller(){

    while(1){
        sem_wait(threadPool->remain_task);      // if there are tasks to do, otherwise, block and wait for tasks

        pthread_mutex_lock(threadPool->task_queue_lock);
        pthread_mutex_lock(threadPool->task_submission_finished_lock);
        if (threadPool->task_submission_finished == 1){    // if no more tasks
            pthread_mutex_unlock(threadPool->task_submission_finished_lock);
            if (!(threadPool->task_head->next)){    // if empty task queue
                threadPool->task_tail = threadPool->task_head;
                sem_post(threadPool->remain_task);
                pthread_mutex_unlock(threadPool->task_queue_lock);
                pthread_exit(NULL);     // exit
            }
        } else{
            pthread_mutex_unlock(threadPool->task_submission_finished_lock);
        }

        unsigned char *result;
        task_queue *tmp = threadPool->task_head->next;
        threadPool->task_head->next = tmp->next;

        if (!threadPool->task_head->next){
            threadPool->task_tail = threadPool->task_head;
        }
        pthread_mutex_unlock(threadPool->task_queue_lock);
        result = encoding(tmp->task_string, tmp->task_size);

        *(threadPool->result + tmp->task_id) = result;
        sem_post(threadPool->result_sem + tmp->task_id);
        tmp->next = NULL;
        free(tmp);
    }
}


/**Collect the result
 *
 */
void *collect_result(){
    unsigned char reserved[3] = {'\0'};
    unsigned char *result;
    size_t res_len;
    for (long i = 0; ; i++){
        pthread_mutex_lock(threadPool->task_submission_finished_lock);
        if (threadPool->task_submission_finished == 1) {    // if no more tasks
            pthread_mutex_unlock(threadPool->task_submission_finished_lock);
            pthread_mutex_lock(threadPool->task_count_lock);
            if (i == threadPool->task_count) {      // if read all results
                pthread_mutex_unlock(threadPool->task_count_lock);
                sem_post(threadPool->all_task_finished);
                sem_post(threadPool->remain_task);
                printf("%s", reserved);
                pthread_exit(NULL);     // exit
            }
            pthread_mutex_unlock(threadPool->task_count_lock);
        }
        else{
            pthread_mutex_unlock(threadPool->task_submission_finished_lock);
        }
        sem_wait(threadPool->result_sem + i);
        result = *(threadPool->result + i);
        res_len = strlen((char*)result) - 2;
        if (reserved[0] == result[0]){
            result[1] += reserved[1];
        }
        else{
            printf("%s", reserved);
        }
        reserved[0] = *(result + res_len);
        reserved[1] = *(result + res_len + 1);
        *(result+res_len) = '\0';
        printf("%s", result);
        free(result);
    }
}