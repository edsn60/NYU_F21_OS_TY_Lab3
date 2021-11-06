#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>

#include "thread_pool.h"
#include "execution.h"
#include "nyuenc.h"


thread_pool *threadPool = NULL;


int main(int argc, char **argv) {
    int option = getopt(argc, argv, ":j:");

    if (option == 63){     // 63 == '?'
        fprintf(stderr, "Error: Unknown option '-%c' \n", optopt);
        exit(-1);
    }

    else if(option == 58 || option == -1){     // 58 == ':', means that no "-j" option, single thread
        single_thread(argv);
        fflush(stdout);
    }
    else{   // multi thread
        char *arg = optarg;
        long threads_count = strtol(arg, NULL, 10);
        init_thread_pool(threads_count, argc, argv);
        unsigned char reserved[3] = {'\0'};
        unsigned char *result;
        size_t res_len;
        for (size_t i = 0; i < threadPool->task_count; i++){
            sem_wait(threadPool->result_sem[i]);
            result = threadPool->result[i];
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
            free(threadPool->result_sem[i]);
        }
        printf("%s", reserved);

//        sem_wait(threadPool->all_task_finished);
//        pthread_join(threadPool->result_handling_thread, NULL);
        for (long i = 0; i < threads_count; i++){
            pthread_join(threadPool->worker_threads[i], NULL);
        }
        free(threadPool->worker_threads);

        free(threadPool->task_head);
//        for(int i = 0; i < RESULT_BUFFER_SIZE; i++){
//            free(threadPool->read_result[i]);
//            free(threadPool->write_result[i]);
//        }
//        pthread_mutex_destroy(threadPool->task_count_lock);
//        free(threadPool->task_count_lock);
//        pthread_mutex_destroy(threadPool->thread_count_lock);
//        free(threadPool->thread_count_lock);
        pthread_mutex_destroy(threadPool->task_queue_lock);
        free(threadPool->task_queue_lock);
        free(threadPool);
    }
}
