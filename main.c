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


/*****************************************************************************************************************
 * Case II
 *****************************************************************************************************************/
//#define DEFAULT_THREAD_NUM 2
//
////#define VECTOR_SIZE 3000000
//#define VECTOR_SIZE 30000
//
//pthread_mutex_t   mutex;
//
//int thread_num;
//
//int* counter;
//pthread_t *thread;
//
//int r[VECTOR_SIZE];
//int s[VECTOR_SIZE];
//int res = 0;
//
///* A CPU-bound task to do vector multiplication */
//void vector_multiply(void* arg) {
//    int i = 0;
//    int n = *((int*) arg);
//
//    for (i = n; i < VECTOR_SIZE; i += thread_num) {
//        my_pthread_mutex_lock(&mutex);
//        res += r[i] * s[i];
//        my_pthread_mutex_unlock(&mutex);
//        printf("%d\n",i);
//    }
//}
//
//void verify() {
//    int i = 0;
//    res = 0;
//    for (i = 0; i < VECTOR_SIZE; i += 1) {
//        res += r[i] * s[i];
//    }
//    printf("verified res is: %d\n", res);
//}
//
//int main(int argc, char **argv) {
//    int i = 0;
//
//    if (argc == 1 || 1) {
//        thread_num = DEFAULT_THREAD_NUM;
//    } else {
//        if (argv[1] < 1) {
//            printf("enter a valid thread number\n");
//            return 0;
//        } else
//            thread_num = atoi(argv[1]);
//    }
//
//    // initialize counter
//    counter = (int*)malloc(thread_num*sizeof(int));
//    for (i = 0; i < thread_num; ++i)
//        counter[i] = i;
//
//    // initialize pthread_t
//    thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));
//
//    // initialize data array
//    for (i = 0; i < VECTOR_SIZE; ++i) {
//        r[i] = i;
//        s[i] = i;
//    }
//
//    my_pthread_mutex_init(&mutex, NULL);
//
//    struct timespec start, end;
//    clock_gettime(CLOCK_REALTIME, &start);
//
//    for (i = 0; i < thread_num; ++i)
//        my_pthread_create(&thread[i], NULL, &vector_multiply, &counter[i]);
//
//    for (i = 0; i < thread_num; ++i)
//        my_pthread_join(thread[i], NULL);
//
//    clock_gettime(CLOCK_REALTIME, &end);
//    printf("running time: %lu micro-seconds\n", (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);
//
//    printf("res is: %d\n", res);
//
//    my_pthread_mutex_destroy(&mutex);
//
//    // feel free to verify your answer here:
//    verify();
//
//    // Free memory on Heap
//    free(thread);
//    free(counter);
//
//    return 0;
//}


/*****************************************************************************************************************
 * Case II
 *****************************************************************************************************************/
#include <stdio.h>
#include <unistd.h>
#define DEFAULT_THREAD_NUM 4

#define C_SIZE 100000
#define R_SIZE 1000

pthread_mutex_t   mutex;

int thread_num;

int* counter;
pthread_t *thread;

int*    a[R_SIZE];
int	 pSum[R_SIZE];
int  sum = 0;

/* A CPU-bound task to do parallel array addition */
void parallel_calculate(void* arg) {
    int i = 0, j = 0;
    int n = *((int*) arg);

    for (j = n; j < R_SIZE; j += thread_num) {
        for (i = 0; i < C_SIZE; ++i) {
            pSum[j] += a[j][i] * i;
        }
    }
    for (j = n; j < R_SIZE; j += thread_num) {
        my_pthread_mutex_lock(&mutex);
        sum += pSum[j];
        my_pthread_mutex_unlock(&mutex);
    }
}

/* verification function */
void verify() {
    int i = 0, j = 0;
    sum = 0;
    memset(&pSum, 0, R_SIZE*sizeof(int));

    for (j = 0; j < R_SIZE; j += 1) {
        for (i = 0; i < C_SIZE; ++i) {
            pSum[j] += a[j][i] * i;
        }
    }
    for (j = 0; j < R_SIZE; j += 1) {
        sum += pSum[j];
    }
    printf("verified sum is: %d\n", sum);
}

int main(int argc, char **argv) {
    int i = 0, j = 0;

    if (argc == 1) {
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
    for (i = 0; i < R_SIZE; ++i)
        a[i] = (int*)malloc(C_SIZE*sizeof(int));

    for (i = 0; i < R_SIZE; ++i)
        for (j = 0; j < C_SIZE; ++j)
            a[i][j] = j;

    memset(&pSum, 0, R_SIZE*sizeof(int));

    // mutex init
    my_pthread_mutex_init(&mutex, NULL);

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    for (i = 0; i < thread_num; ++i)
        my_pthread_create(&thread[i], NULL, &parallel_calculate, &counter[i]);

    for (i = 0; i < thread_num; ++i)
        my_pthread_join(thread[i], NULL);

    printf("sum is: %d\n", sum);
    clock_gettime(CLOCK_REALTIME, &end);
    printf("running time: %lu micro-seconds\n", (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);

    // mutex destroy
    my_pthread_mutex_destroy(&mutex);


    // feel free to verify your answer here:
    verify();

    // Free memory on Heap
    free(thread);
    free(counter);
    for (i = 0; i < R_SIZE; ++i)
        free(a[i]);

    printf("I'm done!\n");
    return 0;
}


///****************************************************************************************
// * Case III
// ****************************************************************************************/
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <signal.h>
//
//#define DEFAULT_THREAD_NUM 2
//#define RAM_SIZE 160
//#define RECORD_NUM 10
//#define RECORD_SIZE 1024
//
//pthread_mutex_t   mutex;
//
//int thread_num;
//
//int* counter;
//pthread_t *thread;
//
//int *mem = NULL;
//
//int sum = 0;
//
//int itr = RECORD_SIZE / 16;
//
//void external_calculate(void* arg) {
//    int i = 0, j = 0, k = 0;
//    int n = *((int*) arg);
//
//    for (k = n; k < RECORD_NUM; k += thread_num) {
//
//        char a[3];
//        char path[20] = "./record/";
//
//        sprintf(a, "%d", k);
//        strcat(path, a);
//
//        FILE *f;
//        f = fopen(path, "r");
//        if (!f) {
//            printf("failed to open file %s, please run ./genRecord.sh first\n", path);
//            exit(0);
//        }
//
//        for (i = 0; i < itr; ++i) {
//            // read 16B from nth record into memory from mem[n*4]
//            for (j = 0; j < 4; ++j) {
//                fscanf(f, "%d", &mem[k*4 + j]);
//                my_pthread_mutex_lock(&mutex);
//                sum += mem[k*4 + j];
//                my_pthread_mutex_unlock(&mutex);
//            }
//        }
//        fclose(f);
//    }
//}
//
//void verify() {
//    int i = 0, j = 0, k = 0;
//
//    char a[3];
//    char path[20] = "./record/";
//
//    sum = 0;
//    memset(mem, 0, RAM_SIZE);
//
//    for (k = 0; k < 10; ++k) {
//        strcpy(path, "./record/");
//        sprintf(a, "%d", k);
//        strcat(path, a);
//
//        FILE *f;
//        f = fopen(path, "r");
//        if (!f) {
//            printf("failed to open file %s, please run ./genRecord.sh first\n", path);
//            exit(0);
//        }
//
//        for (i = 0; i < itr; ++i) {
//            // read 16B from nth record into memory from mem[n*4]
//            for (j = 0; j < 4; ++j) {
//                fscanf(f, "%d\n", &mem[k*4 + j]);
//                sum += mem[k*4 + j];
//            }
//        }
//        fclose(f);
//    }
//
//    printf("verified sum is: %d\n", sum);
//}
//
//void sig_handler(int signum) {
//    printf("%d\n", signum);
//}
//
//int main(int argc, char **argv) {
//    int i = 0;
//
//    if (argc == 1) {
//        thread_num = DEFAULT_THREAD_NUM;
//    } else {
//        if (argv[1] < 1) {
//            printf("enter a valid thread number\n");
//            return 0;
//        } else
//            thread_num = atoi(argv[1]);
//    }
//
//    // initialize counter
//    counter = (int*)malloc(thread_num*sizeof(int));
//    for (i = 0; i < thread_num; ++i)
//        counter[i] = i;
//
//    // initialize pthread_t
//    thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));
//
//    mem = (int*)malloc(RAM_SIZE);
//    memset(mem, 0, RAM_SIZE);
//
//    my_pthread_mutex_init(&mutex, NULL);
//
//    struct timespec start, end;
//    clock_gettime(CLOCK_REALTIME, &start);
//
//    for (i = 0; i < thread_num; ++i)
//        my_pthread_create(&thread[i], NULL, &external_calculate, &counter[i]);
//
//    signal(SIGABRT, sig_handler);
//    signal(SIGSEGV, sig_handler);
//
//    for (i = 0; i < thread_num; ++i)
//        my_pthread_join(thread[i], NULL);
//
//    printf("sum is: %d\n", sum);
//
//    clock_gettime(CLOCK_REALTIME, &end);
//    printf("running time: %lu micro-seconds\n", (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);
//
//    my_pthread_mutex_destroy(&mutex);
//
//    // feel free to verify your answer here:
//    verify();
//
//    free(mem);
//    free(thread);
//    free(counter);
//
//    printf("Case 3 done! \n");
//    return 0;
//}

