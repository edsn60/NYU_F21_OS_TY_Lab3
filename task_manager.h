//
// Created by Ysw on 2021/10/31.
//

#ifndef NYU_F21_OS_TY_LAB3_TASK_MANAGER_H
#define NYU_F21_OS_TY_LAB3_TASK_MANAGER_H

typedef struct TaskQueue{
    char *task_string;
    int task_id;
    struct TaskQueue *next;
}task_queue;

#endif //NYU_F21_OS_TY_LAB3_TASK_MANAGER_H

int submit_task(char **argv, task_queue **tail);