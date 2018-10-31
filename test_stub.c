//
// Created by bh398 on 10/29/18.
//

#include <malloc.h>

typedef long int my_pthread_t;
typedef struct _memory_block {
    int page[20];
    int next_free[20];
    int current_page;
} memory_control_block;


typedef struct _thread_control_block {
    my_pthread_t thread_id;
    int is_main;
    int priority;
    int temporary_priority;
    void *retval;
    struct _thread_control_block *joined_by;
    struct _thread_control_block *next;
    memory_control_block *memo_block;
} thread_control_block;


thread_control_block *get_current_running_thread() {
    thread_control_block *tcb = (thread_control_block *) malloc(sizeof(thread_control_block));
    tcb->thread_id = 1;
    return tcb;
}
