//
// Created by Ysw on 2021/10/31.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>

#include "thread_pool.h"
#include "task_manager.h"
#include "execution.h"


extern thread_pool* threadPool;


/**Initialize the thread pool.
 *
 * @param thread_count:: the number of worker threads.
 * @param argc:: number of command line arguments
 * @param argv:: list of command line arguments
 */
void init_thread_pool(long thread_count, int argc, char **argv){
    threadPool = (thread_pool*) malloc(sizeof(thread_pool));
    if (!threadPool){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:23\n");
        exit(-1);
    }

    // initialize the task queue with an empty head node
    threadPool->task_head = (task_queue*) malloc(sizeof(task_queue));
    if (!threadPool->task_head){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:28\n");
        exit(-1);
    }

    threadPool->task_head->task_string = NULL;
    threadPool->task_head->next = NULL;
    threadPool->task_head->task_id = -1;
    threadPool->task_tail = threadPool->task_head;

    threadPool->task_queue_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!threadPool->task_queue_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:53\n");
        exit(-1);
    }
    pthread_mutex_init(threadPool->task_queue_lock, NULL);

    // ```task_submission_finished``` represents if there will be more tasks
    threadPool->task_submission_finished_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!threadPool->task_submission_finished_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:37\n");
        exit(-1);
    }

    pthread_mutex_init(threadPool->task_submission_finished_lock, NULL);
    threadPool->task_submission_finished = 0;

    // ```task_count``` represents the number of tasks submitted, used for read thread to judge if it has read all results
    threadPool->task_count = 0;
    threadPool->task_count_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!threadPool->task_count_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:46\n");
        exit(-1);
    }
    pthread_mutex_init(threadPool->task_count_lock, NULL);

    // map all files to know the maximum number of tasks
    off_t total_size = map_files(argc, argv);
    total_size = total_size / 4096 + argc - 2;

    // allocate the result buffer and semaphore
    threadPool->result = (unsigned char**) malloc(sizeof(unsigned char*) * total_size);
    threadPool->result_sem = (sem_t*) malloc(sizeof(sem_t) * total_size);
    for (off_t i = 0; i < total_size; i++){
        if (sem_init(&threadPool->result_sem[i], 0, 0) == -1){
            fprintf(stderr, "Error: sem_init failed for result_sem[%ld], errno: %d, des: %s", i, errno, strerror(errno));
        }
    }

    // initialize the semaphores
    threadPool->all_task_finished = (sem_t*) malloc(sizeof(sem_t));
    sem_init(threadPool->all_task_finished, 0, 0);

    threadPool->remain_task = (sem_t*) malloc(sizeof(sem_t));
    sem_init(threadPool->remain_task, 0, 0);

    threadPool->thread_count_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!threadPool->task_count_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:79\n");
        exit(-1);
    }
    pthread_mutex_init(threadPool->thread_count_lock, NULL);

    threadPool->thread_count = thread_count;

    threadPool->worker_threads = (pthread_t*) malloc(sizeof(pthread_t) * (thread_count));
    if (!threadPool->worker_threads){
        fprintf(stderr, "Error: malloc failed in thread_pool.c:87\n");
        exit(-1);
    }

    // create the worker threads
    for (int i = 0; i < thread_count; i++){
        pthread_create((threadPool->worker_threads) + i, NULL, thread_controller, NULL);
    }

    // create read thread
    pthread_create(&(threadPool->result_handling_thread), NULL, collect_result, NULL);

    // begin to generate and submit tasks
    generate_task();
}


/**Destroy the thread pool
 *
 * @param threads_count:: number of worker threads
 */
void destroy_thread_pool(long threads_count){
    pthread_join(threadPool->result_handling_thread, NULL);
    for (long i = 0; i < threads_count; i++){
        pthread_join(*((threadPool->worker_threads)+i), NULL);
    }
    free(threadPool->worker_threads);

    free(threadPool->task_head);

    free(threadPool->result_sem);

    pthread_mutex_destroy(threadPool->task_count_lock);
    free(threadPool->task_count_lock);
    pthread_mutex_destroy(threadPool->thread_count_lock);
    free(threadPool->thread_count_lock);
    pthread_mutex_destroy(threadPool->task_queue_lock);
    free(threadPool->task_queue_lock);
}