#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
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
        init_thread_pool(threads_count, argc, argv);

        sem_wait(threadPool->all_task_finished);    // wait for all tasks to be finished

        // destroy the thread pool
        destroy_thread_pool(threads_count);
    }
}
