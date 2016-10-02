
#include "dccthread.h"
#include "dlist.h"

#include <malloc.h>
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// for Preemption
#include <signal.h>
#include <time.h>
// #include <sys/time.h>


/*
 * Usado pra DEBUG
 */
#define DEBUG
int temp;

typedef struct dccthread {
    char name[DCCTHREAD_MAX_NAME_SIZE];
    ucontext_t *context;
    unsigned int yielded; //0 if not 1 otherwise
    dccthread_t *waiting_for; //pointer to a thread that it is waiting to finish, NULL otherwise
} dccthread_t;

typedef struct waitingThread {
    dccthread_t *w_by;
    dccthread_t *w_for;
} waitingThread_t;

ucontext_t manager;
struct dlist *doneQueue;

// TIMER
struct sigevent tEvent;
struct sigaction tAction;
struct itimerspec tInterval;
sigset_t mask;
timer_t timerid;

/*
struct timespec {
   time_t tv_sec;                // Seconds
   long   tv_nsec;               // Nanoseconds
};

struct itimerspec {
   struct timespec it_interval;  // Timer interval
   struct timespec it_value;     // Initial expiration
};

timer_settime() arms or disarms the timer identified by timerid.
The new_value argument is pointer to an itimerspec structure that
specifies the new initial value and the new interval for the timer.
The itimerspec structure is defined as follows:

int timer_settime(timer_t timerid, int flags,
                         const struct itimerspec *new_value,
                         struct itimerspec * old_value);
int timer_gettime(timer_t timerid, struct itimerspec *curr_value);
*/



/* `dccthread_init` initializes any state necessary for the
 * threadling library and starts running `func`.  this function
 * never returns. */
void dccthread_init(void (*func)(int), int param) {

    #ifdef DEBUG
    printf("Initing dccthread_init\n");
    #endif

    ucontext_t *main = malloc(sizeof(ucontext_t));

    //get the current context
    getcontext(main);

    //modify the current context
    main->uc_link = &manager;
    main->uc_stack.ss_sp = malloc( THREAD_STACK_SIZE );
    main->uc_stack.ss_size = THREAD_STACK_SIZE;
    main->uc_stack.ss_flags = 0;
    makecontext(main, (void (*)(void))func, 1, param);


    doneQueue = dlist_create();

    dccthread_t *newThread = malloc(sizeof(dccthread_t));
    strcpy(newThread->name, "main");
    newThread->context = main;
    newThread->yielded = 0;
    newThread->waiting_for = NULL;
    dlist_push_right(doneQueue, newThread);

    /****************************/
    // Setando temporizador de sinal
    tEvent.sigev_signo = SIGALRM;
    tEvent.sigev_notify = SIGEV_SIGNAL;
    tAction.sa_handler = (void*)dccthread_yield;
    tAction.sa_flags = 0;       // Do nothing

    tInterval.it_value.tv_sec = tInterval.it_interval.tv_sec = 0;
    tInterval.it_value.tv_nsec = tInterval.it_interval.tv_nsec = 10000000; // 10 ms

    sigaction(SIGALRM, &tAction, NULL);
    timer_create(CLOCK_PROCESS_CPUTIME_ID, &tEvent, &timerid);
    timer_settime(&timerid, 0, &tInterval, NULL);
    /****************************/

    while(dlist_empty(doneQueue) == 0) {
        dccthread_t *next_thread = dlist_get_index(doneQueue, 0);

        if(next_thread->waiting_for != NULL) {
            dlist_push_right(doneQueue, next_thread);
            continue;
        }
        #ifdef DEBUG
        printf("next_thread: %s\n", dccthread_name(next_thread));
        #endif

        swapcontext(&manager, next_thread->context);
        sigprocmask(SIG_BLOCK, &mask, NULL);

        dlist_pop_left(doneQueue);

        if(next_thread->yielded || next_thread->waiting_for != NULL) {
            next_thread->yielded = 0;
            dlist_push_right(doneQueue, next_thread);
        }
    }
    #ifdef DEBUG
    printf("Done\n");
    #endif
    exit(0);
}

/* on success, `dccthread_create` allocates and returns a thread
 * handle.  returns `NULL` on failure.  the new thread will execute
 * function `func` with parameter `param`.  `name` will be used to
 * identify the new thread. */
dccthread_t * dccthread_create(
                          const char *name,
                          void (*func)(int ), int param) {
    // sigemptyset(&mask);
    // sigaddset(&mask, SIGALRM);
    // sigprocmask(SIG_BLOCK, &mask, NULL);

    #ifdef DEBUG
    printf("Creating %s, param %d\n", name, param);
    #endif

    ucontext_t *newContext = malloc(sizeof(ucontext_t));

    getcontext(newContext);
    newContext->uc_link = &manager;
    newContext->uc_stack.ss_sp = malloc( THREAD_STACK_SIZE );
    newContext->uc_stack.ss_size = THREAD_STACK_SIZE;
    newContext->uc_stack.ss_flags = 0;

    if (newContext->uc_stack.ss_sp == 0) {
        perror( "malloc: Could not allocate stack" );
        return NULL;
    }

    makecontext(newContext, (void (*)(void))func, 1, param);

    dccthread_t *newThread = malloc(sizeof(dccthread_t));
    strcpy(newThread->name, name);
    newThread->context = newContext;
    newThread->yielded = 0;
    newThread->waiting_for = NULL;
    dlist_push_right(doneQueue, newThread);

    // sigprocmask(SIG_UNBLOCK, &mask, NULL);
    return newThread;
}

/* `dccthread_yield` will yield the CPU (from the current thread to
 * another). */
void dccthread_yield(int a) {
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    dccthread_t *currThread = dccthread_self();

    #ifdef DEBUG
    printf("yield: %s %d\t", dccthread_name(currThread), a);
    // printf("currThread->yielded = %d\n", currThread->yielded);
    #endif
    a = 0;

    currThread->yielded = 1;
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    swapcontext(currThread->context, &manager);
}

void teste_yield(int a) {
    #ifdef DEBUG
    printf("Manual\t");
    #endif
    dccthread_yield(a);
}

/* `dccthread_exit` terminates the current thread, freeing all
 * associated resources. */
void dccthread_exit(void) {
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    dccthread_t *currThread = dccthread_self();
    int index;

    #ifdef DEBUG
    printf("exit: %s\n", dccthread_name(currThread));
    #endif
    //checks if there is any thread waiting for it to finish
    for(index = 0; index < doneQueue->count; index++) {
        dccthread_t *thread = dlist_get_index(doneQueue, index);
        if(thread->waiting_for == currThread) {
            thread->waiting_for = NULL;
        }
    }
    free(currThread->context->uc_stack.ss_sp);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

/* `dccthread_wait` blocks the current thread until thread `tid`
 * terminates. */
void dccthread_wait(dccthread_t *tid) {

    #ifdef DEBUG
    printf("Initing dccthread_wait\n");
    #endif
    int index;
    for(index = 0; index < doneQueue->count; index++) {
        dccthread_t *thread = dlist_get_index(doneQueue, index);
        if(thread == tid) break;
    }

    //otherwise the target thread has already finised
    if(index != doneQueue->count) {
        dccthread_t *currThread = dccthread_self();
        currThread->waiting_for = tid;
        swapcontext(currThread->context, &manager);
    }
}

/* `dccthread_sleep` stops the current thread for the time period
 * specified in `ts`. */
void dccthread_sleep(struct timespec ts) {

}

/* `dccthread_self` returns the current thread's handle. */
dccthread_t * dccthread_self(void) {
    dccthread_t *curr_thread = dlist_get_index(doneQueue, 0);
    return curr_thread;
}


/* `dccthread_name` returns a pointer to the string containing the
 * name of thread `tid`.  the returned string is owned and managed
 * by the library. */
const char * dccthread_name(dccthread_t *tid) {
    return tid->name;
}
