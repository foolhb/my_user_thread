// File:	my_pthread.c
// Author:	Bo Han, Bo Zhang
// Date:	10/01/2019

// name:  Bo Han, Bo Zhang
// username of iLab:
// iLab Server:

#include <sys/time.h>
#include "my_pthread_t.h"

#define STACKSIZE 10*1024
#define TIMEQUANTUM 10000

static int initialized = 0;
static int thread_counter = 0;

//struct thread_scheduler *scheduler = NULL;
thread_queue *queue = NULL;
ucontext_t *return_context = NULL;
thread_queue_node *current_running_thread_node = NULL;
//thread_control_block *main_function_thread = NULL;
struct itimerval *quantum = NULL;
sigset_t signal_mask;

int get_thread_id();

void *thread_task_wrapper(void *(*function)(void *), void *arg);

int environment_initialize();

void insert_node(thread_queue *queue, thread_control_block *thread);

void schedule(int signum);

void schedule(int signum) {
    thread_queue_node *last_running_thread_node = current_running_thread_node;
    current_running_thread_node = queue->head->next;
//    swapcontext(&(last_running_thread_node->thread->thread_context), &(current_running_thread_node->thread->thread_context));
    swapcontext(&(current_running_thread_node->thread->thread_context),&(last_running_thread_node->thread->thread_context));
}

/* create a new thread */
int my_pthread_create(my_pthread_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg) {
    if (initialized == 0) {
        environment_initialize();
        initialized = 1;
        printf("Environment initialized \n");
    }
    if (thread == NULL) {
        if (!(thread = (thread_control_block *) malloc(sizeof(thread_control_block)))) {
            printf("Error when malloc a new thread control block");
        }
    }
//    sigprocmask(SIG_SETMASK, &signal_mask, NULL);
    if (!(thread->thread_context.uc_stack.ss_sp = malloc(STACKSIZE))) {
        printf("Error when initializing thread_context \n");
    }
    thread->thread_context.uc_stack.ss_size = STACKSIZE;
    thread->thread_context.uc_stack.ss_flags = 0;
    thread->thread_context.uc_link = &return_context;
    getcontext(&(thread->thread_context));
    makecontext(&(thread->thread_context), thread_task_wrapper, function, arg);
    insert_node(queue, thread);
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    raise(SIGVTALRM);
    return 0;
}

/* give CPU procession to other user level threads voluntarily */
int my_pthread_yield() {
    //调用schedule
    return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
    return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
    return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
    return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
    return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
    return 0;
};


void *thread_task_wrapper(void *(*function)(void *), void *arg) {
    current_running_thread_node->thread->retval = function(arg);
    current_running_thread_node->thread->status = FINISHED;
}

void abrt_handler(int signum){
    printf("SIGABRT \n");
}

int environment_initialize() {
    signal(SIGABRT,abrt_handler);
    if (!(queue = (thread_queue *) malloc(sizeof(thread_queue)))) {
        printf("Error when initializing schedule queue! \n");
    }
    queue->size = 0;
    if (!(quantum = (struct itimerval *) malloc(sizeof(struct itimerval)))) {
        printf("Error when initializing time quantum \n");
    }
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGVTALRM);
    printf("Quantum initialized \n");
    thread_control_block *main_function_thread = NULL;
    if (!(main_function_thread = (thread_control_block *) malloc(sizeof(thread_control_block)))) {
        printf("Error when creating main function TCB \n");
    }
    if (!(main_function_thread->thread_context.uc_stack.ss_sp = malloc(STACKSIZE))) {
        printf("Error when initializing main function thread_context \n");
    }
    main_function_thread->thread_context.uc_stack.ss_size = STACKSIZE;
    main_function_thread->thread_context.uc_stack.ss_flags = 0;
    main_function_thread->thread_context.uc_link = &return_context;
    //getcontext(&(main_function_thread->thread_context));
    main_function_thread->thread_id = get_thread_id();
    thread_queue_node *node;
    node = (thread_queue_node *) malloc(sizeof(thread_queue_node));
    int a = 0;
    node->thread = main_function_thread;
    current_running_thread_node = node;
    insert_node(queue, main_function_thread);
    quantum->it_value.tv_sec = 0;
    quantum->it_interval.tv_sec = 0;
    quantum->it_value.tv_usec = TIMEQUANTUM;
    quantum->it_interval.tv_usec = TIMEQUANTUM;

    signal(SIGVTALRM, schedule);
    setitimer(ITIMER_VIRTUAL, &quantum, NULL);
    return 0;
}

void insert_node(thread_queue *queue, thread_control_block *thread) {
    //thread_queue_node *temp = NULL;
//    thread_queue_node *node = (thread_queue_node *) malloc(sizeof(thread_queue_node));
    thread_queue_node *node;
    node = (thread_queue_node *) malloc(sizeof(thread_queue_node));
    node->thread = thread;
    node->next = queue->head;
    (queue->size)++;
    queue->head = node;
    printf("thread node %d is put into queue \n", thread->thread_id);
}

int get_thread_id() {
    thread_counter++;
    return thread_counter;
}
