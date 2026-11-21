#!/bin/bash
# Сборка всего, что делалось в лабораторной 3 (запускать из этой папки)
set -x

# Этап 1: статическая библиотека
gcc -c vector_math.c -o vector_math.o
g++ -c main.cpp -o main.o
ar rcs libmath_static.a vector_math.o
g++ main.o -L. -lmath_static -o app_static
./app_static

# Этап 2: динамическая библиотека
gcc -c -fPIC vector_math.c -o vector_math_pic.o
gcc -shared -o libmath_dynamic.so vector_math_pic.o
g++ main.o -L. -lmath_dynamic -o app_dynamic
LD_LIBRARY_PATH=. ./app_dynamic

size app_static app_dynamic
