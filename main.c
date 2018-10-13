//
// Created by 韩博 on 2018/10/12.
//

#include "my_pthread_t.h"

int print1() {
    printf("1111111111! \n");
    return 1;
}

int print2() {
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
    my_pthread_join(*thread2, status);
    printf("I am down! \n");
    return;

}
