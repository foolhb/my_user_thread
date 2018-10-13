//
// Created by 韩博 on 2018/10/12.
//

#include "my_pthread_t.h"

void print();

void main() {
    my_pthread_t *thread = (my_pthread_t *) malloc(sizeof(my_pthread_t));
    my_pthread_create(thread, NULL, print, NULL);
    my_pthread_create(thread, NULL, print, NULL);
    return;

//    int STACKSIZE = 10 * 1024;
//    thread_control_block *main_function_thread = NULL;
//    if (!(main_function_thread = (thread_control_block *) malloc(sizeof(thread_control_block)))) {
//        printf("Error when creating main function TCB \n");
//    }
//    if (!(main_function_thread->thread_context.uc_stack.ss_sp = malloc(1024))) {
//        printf("Error when initializing main function thread_context \n");
//    }
//    main_function_thread->thread_context.uc_stack.ss_size = STACKSIZE;
//    main_function_thread->thread_context.uc_stack.ss_flags = 0;
//    main_function_thread->thread_context.uc_link = NULL;
//    getcontext(&(main_function_thread->thread_context));
//    main_function_thread->thread_id = 1;
//    thread_queue_node *node;
//    node = (thread_queue_node *) malloc(sizeof(thread_queue_node));
}

void print() {
    int i = 0;
    for (i = 0; i < 100; i++) {
        printf("%d \n", i);
    }

}