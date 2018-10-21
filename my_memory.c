//
// Created by bh398 on 10/21/18.
//

#include "my_memory_t.h"
#include <malloc.h>
#include <math.h>

#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define THREADREQ 1

static int initialized = 0;

char *memory;

void* myallocate(int x, char* file, int line, int thread_req){

}


void initialize() {
    memory = (char *) malloc(sizeof(char) * pow(2, 20) * 8);
}

