#!/bin/bash

gcc -c *.c

for i in `seq 1 6`;
do
  echo \***************************
  echo Compiling test$i
  gcc -g -Wall -I. tests/test$i.c dccthread.o dlist.o -o test
  echo Running
  ./test
  echo \***************************
done

rm -f test
