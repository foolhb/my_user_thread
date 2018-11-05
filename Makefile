test1:
	gcc my_memory.c my_memory_t.h my_pthread.c my_pthread_t.h test1.c -o test1.out
	gcc my_memory.c my_memory_t.h my_pthread.c my_pthread_t.h parallelCal.c -o parallelCal
	gcc my_memory.c my_memory_t.h my_pthread.c my_pthread_t.h vectorMultiply.c -o vectorMultiply
