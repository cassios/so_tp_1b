#include <stdlib.h>
#include <stdio.h>
#include "dccthread.h"

int global = 0;

void tloop(int cnt) {
	int i;
	for(i = 0; i < cnt; i++) {
		if(global & 0x1) { global |= 0x2; }
		if(global & 0x4) { global |= 0x8; }
	}
	dccthread_exit();
}

void test(int cnt) {
	dccthread_t *t = dccthread_create("aux", tloop, cnt);
	global |= 0x1;
	int i;
	for(i = 0; i < cnt; i++) {
		if(global & 0x2) { global |= 0x4; }
	}
	printf("global counter is 0x%x\n", global);
	dccthread_exit();
}

int main(int argc, char **argv)
{
	dccthread_init(test, 100000000);
}





// #include <unistd.h>
// #include <time.h>
// #include <stdio.h>
// #include <stdlib.h>
//
// #define USED_CLOCK CLOCK_MONOTONIC // CLOCK_MONOTONIC_RAW if available
// #define NANOS 1000000000LL
//
// int main(int argc, char *argv[]) {
//     /* Whatever */
//     struct timespec begin, current;
//     long long start, elapsed, microseconds;
//     /* set up start time data */
//     if (clock_gettime(USED_CLOCK, &begin)) {
//         /* Oops, getting clock time failed */
//         exit(EXIT_FAILURE);
//     }
//     /* Start time in nanoseconds */
//     start = begin.tv_sec*NANOS + begin.tv_nsec;
//
//     /* Do something interesting */
//
//     /* get elapsed time */
//     if (clock_gettime(USED_CLOCK, &current)) {
//         /* getting clock time failed, what now? */
//         exit(EXIT_FAILURE);
//     }
//
//     /* Elapsed time in nanoseconds */
//     elapsed = current.tv_sec*NANOS + current.tv_nsec - start;
//     microseconds = elapsed / 1000 + (elapsed % 1000 >= 500); // round up halves
//
//     /* Display time in microseconds or something */
// 	printf("%lld\n", elapsed);
// 	printf("%lld\n", microseconds);
// 	printf("%d\n", CLOCK_PROCESS_CPUTIME_ID);
//
//     return EXIT_SUCCESS;
// }
