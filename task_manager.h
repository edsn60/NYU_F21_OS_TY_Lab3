//
// Created by Ysw on 2021/10/31.
//

#ifndef NYU_F21_OS_TY_LAB3_TASK_MANAGER_H
#define NYU_F21_OS_TY_LAB3_TASK_MANAGER_H

typedef struct TaskQueue{
    char *task_string;
    int task_id;
    size_t task_size;
    struct TaskQueue *next;
}task_queue;

#endif //NYU_F21_OS_TY_LAB3_TASK_MANAGER_H

void generate_task(int argc, char **argv);