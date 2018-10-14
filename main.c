//
// Created by 韩博 on 2018/10/12.
//

#include "my_pthread_t.h"

//void test(){
//    printf("Do triger a signal \n");
//}
//int print1() {
//    int i = 0;
//    for(i=0;i<10000000;i++);
//    printf("1111111111! \n");
//    return 1;
//}
//
//int print2() {
//    int i = 0;
//    for(i=0;i<10000000;i++);
//    printf("2222222222! \n");
//    return 2;
//}
//
//void main() {
//    my_pthread_t *thread1 = (my_pthread_t *) malloc(sizeof(my_pthread_t));
//    my_pthread_t *thread2 = (my_pthread_t *) malloc(sizeof(my_pthread_t));
//    my_pthread_create(thread1, NULL, print1, NULL);
//    my_pthread_create(thread2, NULL, print2, NULL);
//    void **status;
//    my_pthread_join(*thread1, status);
//    printf("Thread1 join is done! \n");
//    my_pthread_join(*thread2, status);
//    printf("Thread2 join is done! \n");
//    printf("I am done! \n");
//    return;
//
//}

#define DEFAULT_THREAD_NUM 2

//#define VECTOR_SIZE 3000000
#define VECTOR_SIZE 30000

pthread_mutex_t   mutex;

int thread_num;

int* counter;
pthread_t *thread;

int r[VECTOR_SIZE];
int s[VECTOR_SIZE];
int res = 0;

/* A CPU-bound task to do vector multiplication */
void vector_multiply(void* arg) {
    int i = 0;
    int n = *((int*) arg);

    for (i = n; i < VECTOR_SIZE; i += thread_num) {
        my_pthread_mutex_lock(&mutex);
        res += r[i] * s[i];
        my_pthread_mutex_unlock(&mutex);
        printf("%d\n",i);
    }
}

void verify() {
    int i = 0;
    res = 0;
    for (i = 0; i < VECTOR_SIZE; i += 1) {
        res += r[i] * s[i];
    }
    printf("verified res is: %d\n", res);
}

int main(int argc, char **argv) {
    int i = 0;

    if (argc == 1 || 1) {
        thread_num = DEFAULT_THREAD_NUM;
    } else {
        if (argv[1] < 1) {
            printf("enter a valid thread number\n");
            return 0;
        } else
            thread_num = atoi(argv[1]);
    }

    // initialize counter
    counter = (int*)malloc(thread_num*sizeof(int));
    for (i = 0; i < thread_num; ++i)
        counter[i] = i;

    // initialize pthread_t
    thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

    // initialize data array
    for (i = 0; i < VECTOR_SIZE; ++i) {
        r[i] = i;
        s[i] = i;
    }

    my_pthread_mutex_init(&mutex, NULL);

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    for (i = 0; i < thread_num; ++i)
        my_pthread_create(&thread[i], NULL, &vector_multiply, &counter[i]);

    for (i = 0; i < thread_num; ++i)
        my_pthread_join(thread[i], NULL);

    clock_gettime(CLOCK_REALTIME, &end);
    printf("running time: %lu micro-seconds\n", (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);

    printf("res is: %d\n", res);

    my_pthread_mutex_destroy(&mutex);

    // feel free to verify your answer here:
    verify();

    // Free memory on Heap
    free(thread);
    free(counter);

    return 0;
}
