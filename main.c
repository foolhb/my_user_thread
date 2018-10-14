//
// Created by 韩博 on 2018/10/12.
//

#include "my_pthread_t.h"

void test(){
    printf("Do triger a signal \n");
}
int print1() {
    int i = 0;
    for(i=0;i<10000000;i++);
    printf("1111111111! \n");
    return 1;
}

int print2() {
    int i = 0;
    for(i=0;i<10000000;i++);
    printf("2222222222! \n");
    return 2;
}

void main() {
    my_pthread_t *thread1 = (my_pthread_t *) malloc(sizeof(my_pthread_t));
    my_pthread_t *thread2 = (my_pthread_t *) malloc(sizeof(my_pthread_t));
    my_pthread_create(thread1, NULL, print1, NULL);
    my_pthread_create(thread2, NULL, print2, NULL);
    void **status;
    my_pthread_join(*thread1, status);
    printf("Thread1 join is done! \n");
    my_pthread_join(*thread2, status);
    printf("Thread2 join is done! \n");
    printf("I am done! \n");
    return;

}
