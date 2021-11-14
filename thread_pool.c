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


extern thread_pool_t* thread_pool;


/**Initialize the thread pool.
 *
 * @param thread_count:: the number of worker threads.
 * @param argc:: number of command line arguments
 * @param argv:: list of command line arguments
 */
void init_thread_pool(long thread_count, int argc, char **argv){
    thread_pool = (thread_pool_t*) malloc(sizeof(thread_pool_t));
    if (!thread_pool){
        fprintf(stderr, "Error: malloc failed in thread_pool_t.c:23\n");
        exit(-1);
    }

    // initialize the task queue with an empty head node
    thread_pool->task_head = (task_queue*) malloc(sizeof(task_queue));
    if (!thread_pool->task_head){
        fprintf(stderr, "Error: malloc failed in thread_pool_t.c:28\n");
        exit(-1);
    }

    thread_pool->task_head->task_string = NULL;
    thread_pool->task_head->next = NULL;
    thread_pool->task_head->task_id = -1;
    thread_pool->task_tail = thread_pool->task_head;

    thread_pool->task_queue_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!thread_pool->task_queue_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool_t.c:53\n");
        exit(-1);
    }
    pthread_mutex_init(thread_pool->task_queue_lock, NULL);

    // ```task_submission_finished``` represents if there will be more tasks
    thread_pool->task_submission_finished_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!thread_pool->task_submission_finished_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool_t.c:37\n");
        exit(-1);
    }

    pthread_mutex_init(thread_pool->task_submission_finished_lock, NULL);
    thread_pool->task_submission_finished = 0;

    // ```task_count``` represents the number of tasks submitted, used for read thread to judge if it has read all results
    thread_pool->task_count = 0;
    thread_pool->task_count_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!thread_pool->task_count_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool_t.c:46\n");
        exit(-1);
    }
    pthread_mutex_init(thread_pool->task_count_lock, NULL);

    // map all files to know the maximum number of tasks
    off_t total_size = map_files(argc, argv);
    total_size = total_size / 4096 + argc - 2;

    // allocate the result buffer and semaphore
    thread_pool->result = (unsigned char**) malloc(sizeof(unsigned char*) * total_size);
    thread_pool->result_sem = (sem_t*) malloc(sizeof(sem_t) * total_size);
    for (off_t i = 0; i < total_size; i++){
        if (sem_init(&thread_pool->result_sem[i], 0, 0) == -1){
            fprintf(stderr, "Error: sem_init failed for result_sem[%ld], errno: %d, des: %s", i, errno, strerror(errno));
        }
    }

    // initialize the semaphores
    thread_pool->all_task_finished = (sem_t*) malloc(sizeof(sem_t));
    sem_init(thread_pool->all_task_finished, 0, 0);

    thread_pool->remain_task = (sem_t*) malloc(sizeof(sem_t));
    sem_init(thread_pool->remain_task, 0, 0);

    thread_pool->thread_count_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!thread_pool->task_count_lock){
        fprintf(stderr, "Error: malloc failed in thread_pool_t.c:79\n");
        exit(-1);
    }
    pthread_mutex_init(thread_pool->thread_count_lock, NULL);

    thread_pool->thread_count = thread_count;

    thread_pool->worker_threads = (pthread_t*) malloc(sizeof(pthread_t) * (thread_count));
    if (!thread_pool->worker_threads){
        fprintf(stderr, "Error: malloc failed in thread_pool_t.c:87\n");
        exit(-1);
    }

    // create the worker threads
    for (int i = 0; i < thread_count; i++){
        pthread_create((thread_pool->worker_threads) + i, NULL, thread_controller, NULL);
    }

    // create read thread
    pthread_create(&(thread_pool->result_handling_thread), NULL, collect_result, NULL);

    // begin to generate and submit tasks
    generate_task();
}


/**Destroy the thread pool
 *
 * @param threads_count:: number of worker threads
 */
void destroy_thread_pool(long threads_count){
    pthread_join(thread_pool->result_handling_thread, NULL);
    for (long i = 0; i < threads_count; i++){
        pthread_join(*((thread_pool->worker_threads) + i), NULL);
    }
    free(thread_pool->worker_threads);

    free(thread_pool->task_head);

    free(thread_pool->result_sem);

    pthread_mutex_destroy(thread_pool->task_count_lock);
    free(thread_pool->task_count_lock);
    pthread_mutex_destroy(thread_pool->thread_count_lock);
    free(thread_pool->thread_count_lock);
    pthread_mutex_destroy(thread_pool->task_queue_lock);
    free(thread_pool->task_queue_lock);
}