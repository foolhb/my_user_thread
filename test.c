//
// Created by bh398 on 11/3/18.
//


#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>

#include "my_pthread_t.h"
#include "my_memory_t.h"

#define THREAD_NUM 2
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
        temp1[i] = 9;
    }
    for (i = 0; i < M; i++) {
        temp2[i] = 'H';
        temp3[i] = 'a';
    }
    printf("Distance between temp2 and temp3 is: %ld\n", temp3 - temp2);
    printf("Thread variable initialization is done!\n");
    my_pthread_yield();
    thread_control_block *tcb = get_current_running_thread();
    printf("Now thread: %d\n", tcb->thread_id);
    int counter1 = 0;
    for (i = 0; i < N; i++) {
        printf("%d", temp1[i]);
        if (temp1[i] == 9) counter1++;
    }
    printf("\nThere is: %d 9s\n", counter1);
    for (i = 0; i < M; i++) {
        printf("%c", temp2[i]);
        printf("%c", temp3[i]);
    }
    printf("\n");
    printf("Thread func is done!\n");
}

int main(int argc, char **argv) {
    int i;
    int *temp1 = (int *) malloc(sizeof(int) * N);
    char *temp2 = (char *) malloc(sizeof(char) * M);
    char *temp3 = (char *) malloc(sizeof(char) * M);
    for (i = 0; i < N; i++) {
        temp1[i] = 9;
    }
    for (i = 0; i < M; i++) {
        temp2[i] = 'H';
        temp3[i] = 'a';
    }
    pthread_t thread[THREAD_NUM];
    for (i = 0; i < THREAD_NUM; ++i)
        pthread_create(&thread[i], NULL, &func, 0);
    for (i = 0; i < THREAD_NUM; ++i)
          pthread_join(thread[i], NULL);

    thread_control_block *tcb = get_current_running_thread();
    printf("Now thread: %d\n", tcb->thread_id);
    int counter1 = 0;
    for (i = 0; i < N; i++) {
        printf("%d", temp1[i]);
        if (temp1[i] == 9) counter1++;
    }
    printf("\nThere is: %d 9s\n", counter1);
    for (i = 0; i < M; i++) {
        printf("%c", temp2[i]);
        printf("%c", temp3[i]);
    }
    printf("\n");
    printf("Main func is done!\n");
    return 1;
}
