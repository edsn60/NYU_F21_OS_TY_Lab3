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

#include "task_manager.h"


int submit_task(char **argv, task_queue **tail){
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
            (*tail)->next = (task_queue*) malloc(sizeof(task_queue));
            if (!(*tail)->next){
                fprintf(stderr, "Error: malloc failed in task_manager.c:39\n");
                exit(-1);
            }
            (*tail) = (*tail)->next;
            (*tail)->next = NULL;
            (*tail)->task_id = task_id;
            task_id++;
            (*tail)->task_string = addr;
            offset += page_size;
        }
        if (offset < file_size){
            size_t len = file_size-offset;
            addr = mmap(0, len, PROT_READ, MAP_SHARED, fd, offset);
            if (addr == MAP_FAILED){
                fprintf(stderr, "Error: mmap failed in 2\n");
                exit(-1);
            }
            (*tail)->next = (task_queue*) malloc(sizeof(task_queue));
            if (!(*tail)->next){
                fprintf(stderr, "Error: malloc failed in task_manager.c:58\n");
                exit(-1);
            }
            (*tail) = (*tail)->next;
            (*tail)->next = NULL;
            (*tail)->task_id = task_id;
            task_id++;
            (*tail)->task_string = addr;
        }
        close(fd);
    }
    return task_id;
}