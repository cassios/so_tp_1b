// #include <stdlib.h>
// #include <stdio.h>
// #include "dccthread.h"
//
// int global = 0;
//
// void tloop(int cnt) {
// 	int i;
// 	for(i = 0; i < cnt; i++) {
// 		if(global & 0x1) { global |= 0x2; }
// 		if(global & 0x4) { global |= 0x8; }
// 	}
// 	dccthread_exit();
// }
//
// void test(int cnt) {
// 	dccthread_t *t = dccthread_create("aux", tloop, cnt);
// 	global |= 0x1;
// 	int i;
// 	for(i = 0; i < cnt; i++) {
// 		if(global & 0x2) { global |= 0x4; }
// 	}
// 	printf("global counter is 0x%x\n", global);
// 	dccthread_exit();
// }
//
// int main(int argc, char **argv)
// {
// 	dccthread_init(test, 100000000);
// }

#include <stdlib.h>
#include <stdio.h>
#include "dccthread.h"

int global = 0;

void tloop(int cnt) {
	int i;
	for(i = 0; i < cnt; i++) {
		if(global & 0x1) { global |= 0x2; }
		if(global & 0x4) { global |= 0x8; }
		if (i % 1000 == 0)
			teste_yield(i);
	}
	dccthread_exit();
}

void test(int cnt) {
	char sname[200];
	int i;
	global |= 0x1;

	for (i = 0; i < 10; i++) {
		sprintf(sname, "aux%d", i);
		dccthread_create(sname, tloop, cnt);
	}
	for(i = 0; i < cnt; i++) {
		if(global & 0x2) { global |= 0x4; }
		if (i % 1000 == 0)
			teste_yield(i);
	}
	printf("global counter is 0x%x\n", global);
	dccthread_exit();
}

int main(int argc, char **argv)
{
	dccthread_init(test, 10000000);
}

// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
// #include "dccthread.h"
//
// int global = 0;
//
// void tloop(int cnt) {
// 	int i;
// 	for(i = 0; i < cnt; i++) {
// 		if(global & 0x1) { global |= 0x2; }
// 		if(global & 0x4) { global |= 0x8; }
// 	}
// 	dccthread_exit();
// }
//
// void test(int cnt) {
// 	char sname[200];
// 	int i;
// 	for (i = 0; i < 10; i++) {
// 		sprintf(sname, "aux%d", i);
// 		dccthread_create(sname, tloop, cnt);
// 	}
// 	global |= 0x1;
// 	for(i = 0; i < cnt; i++) {
// 		if(global & 0x2) { global |= 0x4; }
// 	}
// 	printf("global counter is 0x%x\n", global);
// 	dccthread_exit();
// }
//
// int main(int argc, char **argv)
// {
// 	dccthread_init(test, 100000000);
// }
