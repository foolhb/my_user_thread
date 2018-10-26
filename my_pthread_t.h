// File:	my_pthread.h
// Author:	Bo Han, Bo Zhang
// Date:	10/01/2019

// name:  Bo Han, Bo Zhang
// username of iLab:bh398
// iLab Server:ilab3

#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <malloc.h>
#include <time.h>

typedef enum thread_states {
    READY = 1,
    RUNNING = 2,
    BLOCKED = 3,
    WAITING = 4,
    TERMINATED = 5,
};

typedef unsigned long int my_pthread_t;

typedef struct _page_node {
    int page_no;
    struct _page_node *next;
} page_node;

typedef struct _memory_block {
    int page[20];
    int next_free;
    int current_page;
} memory_control_block;

typedef struct _thread_control_block {
    my_pthread_t thread_id;
    ucontext_t thread_context;
    int is_main;
    enum thread_states states;
    int priority;
    int temporary_priority;
    void *retval;
    struct _thread_control_block *joined_by;
    struct _thread_control_block *next;
    memory_control_block *memo_block;
} thread_control_block;

typedef struct _thread_queue_node {
    struct thread_queue_node *next;
    thread_control_block *thread;
} thread_queue_node;

typedef struct _thread_queue {
    thread_control_block *head;
    thread_control_block *rear;
    int priority;
    struct itimerval quantum;
    int size;
} thread_queue;

/* mutex struct definition */
typedef struct _my_pthread_mutex_t {
    /* add something here */
    int lock;
    thread_control_block *lock_owner;
    thread_control_block *waiting_queue;
} my_pthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures
typedef struct _multi_queue {
    thread_queue *ready_queue_0;
    thread_queue *ready_queue_1;
    thread_queue *ready_queue_FCFS;
    thread_queue *finished_queue;
} multi_queue;

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

thread_control_block *get_current_running_thread();


#define USE_MY_PTHREAD 1 (comment it if you want to use pthread)

#ifdef USE_MY_PTHREAD
#define pthread_t my_pthread_t
#define pthread_mutex_t my_pthread_mutex_t
#define pthread_create my_pthread_create
#define pthread_exit my_pthread_exit
#define pthread_join my_pthread_join
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy
#endif
#endif
