#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>

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
        char reserved[3];
        char *result;
        size_t res_len;
        for(int i = 0; i < task_count; i++){
            sleep(1);
            if (sem_wait(threadPool->result_lock[i]) == -1){
                fprintf(stderr, "Error: sem_wait failed, errno: %d\n", errno);
                exit(-1);
            }
            fprintf(stderr, "main down %d\n", i);
            printf("%d\n", i);
            result = threadPool->result[i];
            res_len = strlen(result) - 2; // TODO:: bug

            if (reserved[0] == result[0]){
                result[1] += reserved[1];
            }
            else{
                printf("%s", reserved);
            }
            strncpy(reserved, result + res_len, 2);
            result[res_len] = '\0';
            printf("%s", result);
        }
        printf("%s", reserved);
        for (pthread_t *thread = threadPool->threads; *thread; thread++){
            pthread_join(*thread, NULL);
        }
    }
}
