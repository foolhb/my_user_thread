//
// Created by bh398 on 10/25/18.
//

//#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)

#include <malloc.h>
#include "my_pthread_t.h"
#include "my_memory_t.h"

#define STACKSIZE 1024 * 4

typedef struct __test_struct {
    int a;
    int b;
    char c;
} test_struct;

int func(int i) {
    return i;
}

void main() {
    thread_control_block *tcb = (thread_control_block *) malloc(sizeof(thread_control_block));
    printf("%ld\n", sizeof(thread_control_block));
    test_struct *p = myallocate(sizeof(test_struct), __FILE__, __LINE__, 0);
    p->a = 0;
    p->b = 1;
    p->c = 'h';
    printf("%d\n", p->a);
    return;
}


