#!/bin/bash

gcc -c *.c

# for i in `seq 1 12`;
# do
#   echo \***************************
#   echo Compiling test$i
#   gcc -g -Wall -I. tests/test$i.c dccthread.o dlist.o -o test
#   echo Running
#   ./test
#   rm -f test
#   echo \***************************
# done

gcc -g -Wall -I. main.c dccthread.o dlist.o -o test
echo Running
./test
rm -f test
