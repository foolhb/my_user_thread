//
// Created by bh398 on 10/21/18.
//

#ifndef MY_USER_THREAD_MY_MEMORY_H
#define MY_USER_THREAD_MY_MEMORY_H

#endif //MY_USER_THREAD_MY_MEMORY_H


#include"my_pthread_t.h"

typedef struct _memory_block_head{
    struct _memory_block_head* pre;
    struct _memory_block_head* next;
    uint free;
    int size;
} memo_head;