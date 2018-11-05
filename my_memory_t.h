//
// Created by bh398 on 10/21/18.
//

#ifndef MY_USER_THREAD_MY_MEMORY_H
#define MY_USER_THREAD_MY_MEMORY_H

#endif //MY_USER_THREAD_MY_MEMORY_H


#include"my_pthread_t.h"

void memory_manager();

void mydeallocate(char *p, char *file, int line, int thread_req);

void *myallocate(size_t size, char *file, int line, int thread_req);

void *shalloc(size_t size);




