cmake_minimum_required(VERSION 3.12)
project(Benchmark)

set(CMAKE_CXX_STANDARD 14)

add_executable(Benchmark
        externalCal.c
        genRecord.sh
        Makefile
        parallelCal.c
        vectorMultiply.c)
