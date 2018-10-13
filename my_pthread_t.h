// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server: 
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ucontext.h>
#include <signal.h>

typedef enum thread_status {
    READY = 1,
    BLOCKING = 2,
    FINISHED = 3,
};

typedef struct _thread_control_block {
    uint thread_id;
    ucontext_t thread_context;
    enum thread_status status;
    void **retval
} thread_control_block, my_pthread_t;

typedef struct _thread_queue_node {
    struct thread_queue_node *next;
    thread_control_block *thread;
} thread_queue_node;

typedef struct _thread_queue {
    thread_queue_node *head;
    int size;
} thread_queue;

/* mutex struct definition */
typedef struct my_pthread_mutex_t {
    /* add something here */
    int flag;
    thread_control_block *mutex_owner;
    thread_queue_node waiting_queue;
} my_pthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures
typedef struct thread_scheduler {

};

/* Function Declarations: */

/* create a new thread */
int my_pthread_create(my_pthread_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#endif
