// File:	my_pthread.c
// Author:	Bo Han, Bo Zhang
// Date:	10/01/2019

// name:  Bo Han, Bo Zhang
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"

#define STACKSIZE 5*1024
#define TIMEQUANTUM 25
#define MAX_THREAD_NUMBER 100
#define NUMBER_OF_QUEUE_LEVELS 3

static int initialized = 0;
static int thread_counter = 0;

thread_control_block *all_threads[MAX_THREAD_NUMBER];
thread_queue *queues[NUMBER_OF_QUEUE_LEVELS];
thread_queue *finished_queue;
//multi_queue *queues;
ucontext_t return_context;
thread_control_block *current_running_thread = NULL;

struct itimerval quantum[NUMBER_OF_QUEUE_LEVELS];
//struct itimerval quantum_1;
sigset_t signal_mask;

/**
 * Part I: Operation for queue
 */
void insert_node(thread_queue *queue, thread_control_block *thread) {
    if (queue->size == 0) {
        queue->size = 1;
        queue->head = thread;
        queue->rear = thread;
    } else {
        queue->size++;
        queue->rear->next = thread;
        queue->rear = thread;
    }
    //printf("thread %d put into queue\n", thread->thread_id);
}

thread_control_block *pop_node(thread_queue *queue) {
    if (queue->size == 0) {
        printf("Error! No thread in ready queue! \n");
        return NULL;
    }
    queue->size--;
    thread_control_block *thread = queue->head;
    queue->head = queue->head->next;
    int id = thread->thread_id;
//    printf("thread %d is popped out\n", thread->thread_id);
    return thread;

}

/**
 * Part II: Scheduler
 */
thread_queue *get_highest_nonempty_queue() {
    thread_control_block *next_thread_to_swap_in = NULL;
    if (queues[0]->size != 0) {
        return queues[0];
    } else if (queues[1]->size != 0) {
        return queues[1];
    } else if (queues[2]->size != 0) {
        return queues[2];
    }
    return NULL;
}

void schedule(int signum) {
//    printf("Invoking scheduler now \n");
    sigprocmask(SIG_SETMASK, &signal_mask, NULL);
    thread_control_block *last_running_thread = current_running_thread;
    thread_control_block *next_thread_to_swap_in = NULL;
    thread_queue *current_queue = get_highest_nonempty_queue();
    if (current_queue == NULL) {
//        printf("This is the last thread hasn't finished yet \n");
        return;
    } else {
        current_running_thread = pop_node(current_queue);
    }

    int new_priority = last_running_thread->priority + 1;
    if (last_running_thread->status == TERMINATED) {
        insert_node(finished_queue, current_running_thread);
    } else {
        insert_node(queues[new_priority], last_running_thread);
    }
//    printf("thread %d is executing \n", current_running_thread->thread_id);
    if (current_running_thread->priority != 2) {
        setitimer(ITIMER_VIRTUAL, &quantum[new_priority], NULL);
    }
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    swapcontext(&(last_running_thread->thread_context), &(current_running_thread->thread_context));
}


/**
 * Part III: helper function for task wrapping, environment initializing, etc.
 */

int get_thread_id() {
    thread_counter++;
    return thread_counter;
}

void *thread_task_wrapper(void *(*function)(void *), void *arg) {
    signal(SIGVTALRM, schedule);
    current_running_thread->retval = function(arg);
    printf("return value(int) is: %d\n", current_running_thread->retval);
    current_running_thread->status = TERMINATED;
}

void function_after_task_finished() {
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    printf("Thread %d is finished \n", current_running_thread->thread_id);
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    raise(SIGVTALRM);
}

int environment_initialize() {
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGVTALRM);

//    queues = (multi_queue *) malloc(sizeof(multi_queue));
//    queues->ready_queue_0 = (thread_queue *) malloc(sizeof(thread_queue));
//    queues->ready_queue_1 = (thread_queue *) malloc(sizeof(thread_queue));
//    queues->ready_queue_FCFS = (thread_queue *) malloc(sizeof(thread_queue));
//    queues->finished_queue = (thread_queue *) malloc(sizeof(thread_queue));

    int i = 0;
    for (i = 0; i < NUMBER_OF_QUEUE_LEVELS; i++) {
        queues[i] = (thread_queue *) malloc(sizeof(thread_queue));
        queues[i]->head = NULL;
        queues[i]->rear = NULL;
        queues[i]->size = 0;
        queues[i]->priority = i;
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
    getcontext(&(main_function_thread->thread_context));
    main_function_thread->thread_context.uc_stack.ss_size = 10 * STACKSIZE;
    main_function_thread->thread_context.uc_stack.ss_sp = (char *) malloc(10 * STACKSIZE);
    main_function_thread->thread_context.uc_link = &return_context;
    main_function_thread->status = RUNNING;
    main_function_thread->is_main = 1;
    main_function_thread->thread_id = 0;
    all_threads[main_function_thread->thread_id] = main_function_thread;
    current_running_thread = main_function_thread;

//    quantum_0.it_value.tv_sec = 0;
//    quantum_0.it_interval.tv_sec = 0;
//    quantum_0.it_value.tv_usec = TIMEQUANTUM;
//    quantum_0.it_interval.tv_usec = TIMEQUANTUM;
//
//    quantum_0.it_value.tv_sec = 0;
//    quantum_0.it_interval.tv_sec = 0;
//    quantum_0.it_value.tv_usec = 2 * TIMEQUANTUM;
//    quantum_0.it_interval.tv_usec = 2 * TIMEQUANTUM;

    for (i = 0; i < NUMBER_OF_QUEUE_LEVELS; i++) {
        quantum[i].it_value.tv_sec = 0;
        quantum[i].it_interval.tv_sec = 0;
        quantum[i].it_value.tv_usec = TIMEQUANTUM * (i + 1);
        quantum[i].it_interval.tv_usec = TIMEQUANTUM * (i + 1);
    }

    signal(SIGVTALRM, schedule);
    //setitimer(ITIMER_VIRTUAL, &quantum[0], NULL);
    return 0;
}

/**
 * Part IV: My_pthread APIs
 */

/* create a new thread */
int my_pthread_create(my_pthread_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg) {
    if (initialized == 0) {
        environment_initialize();
        initialized = 1;
        printf("Environment initialized \n");
    }
    thread_control_block *tcb = NULL;
    tcb = (thread_control_block *) malloc(sizeof(thread_control_block));

    sigprocmask(SIG_SETMASK, &signal_mask, NULL);
    tcb->next = NULL;
    tcb->thread_id = get_thread_id();
    *thread = tcb->thread_id;
    tcb->is_main = 0;
    tcb->priority = 0;
    tcb->status = READY;
    getcontext(&(tcb->thread_context));
    tcb->thread_context.uc_stack.ss_sp = (char *) malloc(STACKSIZE);
    tcb->thread_context.uc_stack.ss_size = STACKSIZE;
    tcb->thread_context.uc_stack.ss_flags = 0;
    tcb->thread_context.uc_link = &return_context;
    makecontext(&(tcb->thread_context), thread_task_wrapper, 2, function, arg);
    sigemptyset(&tcb->thread_context.uc_sigmask);
    insert_node(queues[tcb->priority], tcb);
    all_threads[*thread] = tcb;
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    return 0;
}

/* give CPU procession to other user level threads voluntarily */
int my_pthread_yield() {
    //调用schedule
    printf("thread %d yields \n", current_running_thread->thread_id);
    raise(SIGVTALRM);
    return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
    current_running_thread->retval = value_ptr;
    current_running_thread->status = TERMINATED;
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
    thread_control_block *tempPtr = all_threads[thread];
    current_running_thread->status = BLOCKED;
    printf("Thread %d is joined to thread %d \n", current_running_thread->thread_id, tempPtr->thread_id);
    while (1) {
        if (tempPtr->status == TERMINATED) {
            printf("Joined thread is finished! \n");
            current_running_thread->status = RUNNING;
            value_ptr = tempPtr->retval;
            return 1;
        }
        my_pthread_yield();
    }
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

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
    while (1) {
        if ((__sync_lock_test_and_set(&mutex->lock, 1)) == 0) break;
        my_pthread_yield();
    }
    mutex->lock_owner = current_running_thread;
    return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
    mutex->lock = 0;
    mutex->lock_owner = NULL;
    return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
    if (mutex->lock == 1) {
        printf("Error! Trying to destroyed an locked mutex!\n");
        return -1;
    }
    return 0;
};






