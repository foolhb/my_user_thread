// File:	my_pthread.c
// Author:	Bo Han, Bo Zhang
// Date:	10/01/2019

// name:  Bo Han, Bo Zhang
// username of iLab:bh398
// iLab Server:ilab3

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>

#include "my_pthread_t.h"
//#include "my_memory_t.h"

#define STACKSIZE (4*1024 - 1)
#define TIMEUNIT 25
#define MAX_THREAD_NUMBER 100
#define NUMBER_OF_QUEUE_LEVELS 7
#define LOWEST_PRIORITY (NUMBER_OF_QUEUE_LEVELS - 1)

static int initialized = 0;
static int thread_counter = 0;
static int schedule_invoking_times = 0;

thread_control_block *all_threads[MAX_THREAD_NUMBER];
thread_queue *ready_queues[NUMBER_OF_QUEUE_LEVELS];
thread_queue *finished_queue;

ucontext_t return_context;
thread_control_block *current_running_thread = NULL;

struct itimerval time_quantum[NUMBER_OF_QUEUE_LEVELS];
sigset_t signal_mask;

/**
 * Part I:Helper functions for Multi Queue Scheduler,
 * including pushing and popping thread from queue, finding nonempty queue with highest priority,
 * levelling up threads with low priority, awaking blocked threads that are joined to the terminated thread, etc.
 */
void insert_thread_to_rear(thread_queue *queue, thread_control_block *thread) {
    if (queue->size == 0) {
        queue->size = 1;
        queue->head = thread;
        queue->rear = thread;
    } else {
        queue->size++;
        queue->rear->next = thread;
        queue->rear = thread;
    }
}

thread_control_block *pop_thread_from_head(thread_queue *queue) {
    if (queue->size == 0) {
        return NULL;
    }
    queue->size--;
    thread_control_block *thread = queue->head;
    queue->head = queue->head->next;
    thread->next = NULL;
    return thread;
}

thread_queue *nonempty_queue_with_highest_priority() {
    int i = 0;
    for (i; i < NUMBER_OF_QUEUE_LEVELS; i++) {
        if (ready_queues[i]->size != 0) {
            return ready_queues[i];
        }
    }
    return NULL;
}

void awake_joining_threads(thread_control_block *terminated_thread) {
    thread_control_block *thread_to_awake = terminated_thread->joined_by;
    while (thread_to_awake != NULL) {
        thread_to_awake->states = READY;
        thread_control_block *temp = thread_to_awake;
        thread_to_awake = thread_to_awake->next;
        temp->next = NULL;
        insert_thread_to_rear(ready_queues[temp->priority], temp);
    }
}

void level_up() {
    int i = NUMBER_OF_QUEUE_LEVELS - 1;
    for (i; i > 0; i--) {
        if (ready_queues[i]->size == 0) break;
        thread_control_block *tbc = ready_queues[i]->head;
        while (tbc != NULL) {
            tbc->priority--;
            tbc = tbc->next;
        }
        ready_queues[i - 1]->rear->next = ready_queues[i]->head;
        ready_queues[i - 1]->rear = ready_queues[i]->rear;
        ready_queues[i]->head = NULL;
        ready_queues[i]->head = NULL;
    }
}

/**
 * Part II: Scheduler
 * We use a multilevel feedback queue, number of queues is defined in NUMBER_OF_QUEUE_LEVELS.
 * Currently we use 3 queues, with 2 Round Robin queue and 1 FCFS queue
 * After scheduler is invoked specific times, threads in low level queues will be levelled up in case of starvation
 */

void schedule(int signum) {
    schedule_invoking_times++;
    sigprocmask(SIG_SETMASK, &signal_mask, NULL);
    /** handle the last running thread first, put it into the corresponding queue according to its thread states */
    thread_control_block *last_running_thread = current_running_thread;
    int new_priority;
    switch (last_running_thread->states) {
        case TERMINATED:
            insert_thread_to_rear(finished_queue, last_running_thread);
            awake_joining_threads(last_running_thread);
            break;
        case READY:
            if (last_running_thread->temporary_priority != -1) new_priority = last_running_thread->temporary_priority;
            else {
                new_priority = last_running_thread->priority + 1;
                last_running_thread->priority++;
            }
            insert_thread_to_rear(ready_queues[new_priority], last_running_thread);
            break;
        case WAITING:
            break;
        case BLOCKED:
            break;
    }
    /** check if we need to level up the threads with low priority, in case of long time starvation */
    if (schedule_invoking_times >= current_running_thread) {
        schedule_invoking_times = 0;
        level_up();
    }
    /** get the next thread to execute(to swap in) from the nonempty queue with highest priority*/
    thread_queue *current_queue = nonempty_queue_with_highest_priority();
    if (current_queue == NULL) return;
    else current_running_thread = pop_thread_from_head(current_queue);
//    if (current_running_thread->priority != NUMBER_OF_QUEUE_LEVELS - 1) {
    setitimer(ITIMER_VIRTUAL, &time_quantum[current_queue->priority], NULL);
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    swapcontext(&(last_running_thread->thread_context), &(current_running_thread->thread_context));
}


/**
 * Part III: Helper function for p_thread APIs,
 * including task wrapper, environment initializing, etc.
 */

int get_thread_id() {
    thread_counter++;
    return thread_counter;
}

/**
 * parameter function is the real thread task routine, which is wrapped by this function
 */
void *thread_task_wrapper(void *(*function)(void *), void *arg) {
    signal(SIGVTALRM, schedule);
    current_running_thread->retval = function(arg);
    current_running_thread->states = TERMINATED;
}

/**
 * Necessary post work after a thread finishes, such as free the resources
 */
void function_after_task_finished() {
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    //printf("Thread %d is finished \n", current_running_thread->thread_id);
    free((current_running_thread->thread_context.uc_stack.ss_sp));
    current_running_thread->thread_context.uc_stack.ss_sp = NULL;
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    raise(SIGVTALRM);
}

/**
 * Executed the first time entering this program
 * Initialize all the queues, main context, time quantum table for different queues, etc.
 */
int environment_initialize() {
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGVTALRM);

    int i = 0;
    for (i = 0; i < NUMBER_OF_QUEUE_LEVELS; i++) {
        ready_queues[i] = (thread_queue *) malloc(sizeof(thread_queue));
        ready_queues[i]->head = NULL;
        ready_queues[i]->rear = NULL;
        ready_queues[i]->size = 0;
        ready_queues[i]->priority = i;
    }
    finished_queue = (thread_queue *) malloc(sizeof(thread_queue));

    getcontext(&return_context);
    return_context.uc_stack.ss_sp = (char *) malloc(STACKSIZE);
    return_context.uc_stack.ss_size = STACKSIZE;
    return_context.uc_stack.ss_flags = 0;
    return_context.uc_link = 0;
    makecontext(&return_context, function_after_task_finished, 0);

    thread_control_block *main_function_thread = NULL;
    main_function_thread = (thread_control_block *) malloc(sizeof(thread_control_block));
    main_function_thread->thread_id = 0;
    getcontext(&(main_function_thread->thread_context));
    main_function_thread->thread_context.uc_stack.ss_size = 10 * STACKSIZE;
    main_function_thread->thread_context.uc_stack.ss_sp = (char *) malloc(10 * STACKSIZE);
    main_function_thread->thread_context.uc_link = &return_context;
    main_function_thread->is_main = 1;
    main_function_thread->states = RUNNING;
    main_function_thread->priority = 0;
    main_function_thread->temporary_priority = -1;
    main_function_thread->joined_by = NULL;
    main_function_thread->next = NULL;
    all_threads[main_function_thread->thread_id] = main_function_thread;
    current_running_thread = main_function_thread;

    for (i = 0; i < NUMBER_OF_QUEUE_LEVELS - 1; i++) {
        time_quantum[i].it_value.tv_sec = 0;
        time_quantum[i].it_interval.tv_sec = 0;
        time_quantum[i].it_value.tv_usec = TIMEUNIT * (i + 1);
        time_quantum[i].it_interval.tv_usec = TIMEUNIT * (i + 1);
    }
    time_quantum[i].it_value.tv_sec = 0;
    time_quantum[i].it_interval.tv_sec = 0;
    time_quantum[i].it_value.tv_usec = 0;
    time_quantum[i].it_interval.tv_usec = 0;

    signal(SIGVTALRM, schedule);

    return 0;
}


/**
 * Part IV: My_pthread APIs
 */


/**
 * create a new thread
 * @param attr: This parameter is ignored according to the assignment!
 */
int my_pthread_create(my_pthread_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg) {
    if (initialized == 0) {
        environment_initialize();
        initialized = 1;
        //printf("Environment initialized \n");
    }
    thread_control_block *tcb = NULL;
    tcb = (thread_control_block *) malloc(sizeof(thread_control_block));
    //sigprocmask(SIG_SETMASK, &signal_mask, NULL);
    tcb->joined_by = NULL;
    tcb->next = NULL;
    tcb->thread_id = get_thread_id();
    *thread = tcb->thread_id;
    getcontext(&(tcb->thread_context));
    tcb->thread_context.uc_stack.ss_sp = (char *) malloc(STACKSIZE);
    tcb->thread_context.uc_stack.ss_size = STACKSIZE;
    tcb->thread_context.uc_stack.ss_flags = 0;
    tcb->thread_context.uc_link = &return_context;
    makecontext(&(tcb->thread_context), thread_task_wrapper, 2, function, arg);
    sigemptyset(&tcb->thread_context.uc_sigmask);
    tcb->is_main = 0;
    tcb->states = READY;
    tcb->priority = 0;
    tcb->temporary_priority = -1;
    tcb->joined_by = NULL;
    tcb->next = NULL;
    insert_thread_to_rear(ready_queues[tcb->priority], tcb);
    all_threads[*thread] = tcb;
    //sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    return 0;
}

/* give CPU procession to other user level threads voluntarily */
int my_pthread_yield() {
    //printf("thread %d yields \n", current_running_thread->thread_id);
    raise(SIGVTALRM);
    return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
    current_running_thread->retval = value_ptr;
    current_running_thread->states = TERMINATED;
    raise(SIGVTALRM);
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
    sigprocmask(SIG_SETMASK, &signal_mask, NULL);
    thread_control_block *joinedThread = all_threads[thread];
    //printf("Thread %d is joined to thread %d \n", current_running_thread->thread_id, joinedThread->thread_id);
    if (joinedThread->states != TERMINATED) {
        current_running_thread->states = BLOCKED;
        current_running_thread->next = joinedThread->joined_by;
        joinedThread->joined_by = current_running_thread;
        sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
        my_pthread_yield();
    }
    return 1;
}

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
    if (mutex == NULL) {
        mutex = (my_pthread_mutex_t *) malloc(sizeof(my_pthread_mutex_t));
    }
    mutex->lock = 0;
    mutex->lock_owner = NULL;
    mutex->waiting_queue = NULL;
    return 0;
};

void detect_priority_inversion(my_pthread_mutex_t *mutex) {
    thread_control_block *owner = mutex->lock_owner;
    int current_priority = owner->priority;
    thread_control_block *waiting_thread = mutex->waiting_queue;
    while (waiting_thread != NULL) {
        if (waiting_thread->priority < current_priority) current_priority = waiting_thread->priority;
        waiting_thread = waiting_thread->next;
    }
    if (current_priority < owner->priority) owner->temporary_priority = current_priority;
    return;
}


/* aquire the mutex lock */
/**
 * Each mutex has its own waiting queue. If a thread fails to get te lock this time, it is put into the waiting queue.
 * Each time a new thread trying to get the mutex, detect if there is priority inversion
 */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
    while (1) {
        sigprocmask(SIG_SETMASK, &signal_mask, NULL);
        if ((__sync_lock_test_and_set(&mutex->lock, 1)) == 0) {
            mutex->lock_owner = current_running_thread;
            sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
            detect_priority_inversion(mutex);
            return 0;
        } else {
            current_running_thread->states = WAITING;
            current_running_thread->next = mutex->waiting_queue;
            mutex->waiting_queue = current_running_thread;
            detect_priority_inversion(mutex);
            sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
            raise(SIGVTALRM);
        }
    }
};

/* release the mutex lock */
/**
 * Unlock the mutex, and awake the first waiting thread in the waiting queue to get the lock, if there is any.
 * If current lock owner has a temporary priority to handle the priority inversion, reset it to normal mode.
 */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
    sigprocmask(SIG_SETMASK, &signal_mask, NULL);
    mutex->lock = 0;
    mutex->lock_owner->temporary_priority = -1;
    mutex->lock_owner = NULL;
    if (mutex->waiting_queue != NULL) {
        thread_control_block *next_lock_owner = mutex->waiting_queue;
        next_lock_owner->states = READY;
        mutex->waiting_queue = next_lock_owner->next;
        next_lock_owner->next = NULL;
        insert_thread_to_rear(ready_queues[next_lock_owner->priority], next_lock_owner);
    }
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
    if (mutex->lock == 1) {
        printf("Error! Trying to destroy a locked mutex!\n");
        return -1;
    }
    return 0;
};

thread_control_block *get_current_running_thread() {
    return current_running_thread;
}