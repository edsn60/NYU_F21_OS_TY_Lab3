#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>

#include "thread_pool.h"
#include "execution.h"


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
        int task_count = init_thread_pool(threads_count, argv);
        pthread_mutex_lock(threadPool->task_finished_lock);
        if (threadPool->task_finished == 0){
            pthread_cond_wait(threadPool->all_task_finished, threadPool->task_finished_lock);
        }
//        sem_wait(threadPool->sem_read_finished);
        pthread_join(threadPool->result_handling_thread, NULL);
        for (pthread_t *thread = threadPool->worker_threads; *thread; thread++){
            pthread_join(*thread, NULL);
        }
    }
}
