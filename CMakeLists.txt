cmake_minimum_required(VERSION 3.12)
project(my_user_thread C)

set(CMAKE_C_STANDARD 99)

add_executable(my_user_thread
        my_pthread_t.h
        my_pthread.c
        my_memory_t.h
        my_memory.c
        main.c)