//
// Created by bh398 on 11/3/18.
//


#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>

#include "my_pthread_t.h"
#include "my_memory_t.h"

#define THREAD_NUM 5
#define N 2000  // N is the large across-page variable size
#define M 100   // M is the small single-page variable size
#define THREADREQ 1
#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)

void func() {
    int i;
    int *temp1 = (int *) malloc(sizeof(int) * N);
    char *temp2 = (char *) malloc(sizeof(char) * M);
    char *temp3 = (char *) malloc(sizeof(char) * M);
    for (i = 0; i < N; i++) {
        temp1[i] = 0;
    }
    for (i = 0; i < M; i++) {
        temp2[i] = 'H';
        temp3[i] = 'A';
    }
    thread_control_block *tcb = get_current_running_thread();
    printf("thread %d yields, its user pages will be evicted\n",tcb->thread_id);
    my_pthread_yield();
    printf("\n");
    printf("Thread: %d executing, its user pages will be swapped in\n"
           "Check whether its variable remains\n", tcb->thread_id);
    int counter1 = 0;
    for (i = 0; i < N; i++) {
        printf("%d", temp1[i]);
        if (temp1[i] == 0) counter1++;
    }
    int counter2 = 0;
    printf("\nThere is: %d 0s. This is a large array over more than 1 page\n", counter1);
    for (i = 0; i < M; i++) {
        printf("%c", temp2[i]);
        printf("%c", temp3[i]);
        if (temp2[i] == 'H' && temp3[i] == 'A') counter2++;
    }
    printf("\nThere is: %d HAs. This is a small array inside one page\n", counter2);
}

int main(int argc, char **argv) {
    int i;
    int *temp1 = (int *) malloc(sizeof(int) * N);
    char *temp2 = (char *) malloc(sizeof(char) * M);
    char *temp3 = (char *) malloc(sizeof(char) * M);
    for (i = 0; i < N; i++) {
        temp1[i] = 0;
    }
    for (i = 0; i < M; i++) {
        temp2[i] = 'H';
        temp3[i] = 'A';
    }
    pthread_t thread[THREAD_NUM];
    for (i = 0; i < THREAD_NUM; ++i)
        pthread_create(&thread[i], NULL, &func, 0);
    for (i = 0; i < THREAD_NUM; ++i)
        pthread_join(thread[i], NULL);

    thread_control_block *tcb = get_current_running_thread();
    printf("\n");
    printf("Main thread executing\n"
           "Check whether its variable remains\n");
    int counter1 = 0;
    for (i = 0; i < N; i++) {
        printf("%d", temp1[i]);
        if (temp1[i] == 0) counter1++;
    }
    int counter2 = 0;
    printf("\nThere is: %d 0s. This is a large array over more than 1 page\n", counter1);
    for (i = 0; i < M; i++) {
        printf("%c", temp2[i]);
        printf("%c", temp3[i]);
        if (temp2[i] == 'H' && temp3[i] == 'A') counter2++;
    }
    free(temp1);
    free(temp2);
    free(temp3);
    printf("\nThere is: %d HAs. This is a small array inside one page\n", counter2);
    return 1;
}
