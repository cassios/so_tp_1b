
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

typedef struct dccthread {
    char name[DCCTHREAD_MAX_NAME_SIZE];
    ucontext_t *context;
    unsigned int yielded; //0 if not, 1 otherwise
    unsigned int sleeping; //0 if not, 1 otherwise
    dccthread_t *waiting_for; //pointer to a thread that it is waiting to finish, NULL otherwise
} dccthread_t;

struct dlist *doneQueue;
ucontext_t manager;
sigset_t mask;

/* `dccthread_init` initializes any state necessary for the
 * threadling library and starts running `func`.  this function
 * never returns. */
void dccthread_init(void (*func)(int), int param) {
    doneQueue = dlist_create();

    dccthread_create("main", func, param);

    /************ Setando temporizador de sinal ****************/
    struct sigevent tEvent;
    struct sigaction tAction;
    struct itimerspec tInterval;
    timer_t timerid;
    
    tEvent.sigev_signo = SIGRTMIN;
    tEvent.sigev_notify = SIGEV_SIGNAL;
    tAction.sa_handler = (void*)dccthread_yield;
    tAction.sa_flags = 0;       // Do nothing

    tInterval.it_value.tv_sec = tInterval.it_interval.tv_sec = 0;
    tInterval.it_value.tv_nsec = tInterval.it_interval.tv_nsec = 10000000; // 10 ms

    sigaction(SIGRTMIN, &tAction, NULL);
    timer_create(CLOCK_PROCESS_CPUTIME_ID, &tEvent, &timerid);
    timer_settime(timerid, 0, &tInterval, NULL);

    sigemptyset(&mask);
    sigaddset(&mask, SIGRTMIN);
    /**********************************************************/

    sigprocmask(SIG_BLOCK, &mask, NULL);
    while(dlist_empty(doneQueue) == 0) {
        dccthread_t *next_thread = dlist_get_index(doneQueue, 0);

        if(next_thread->sleeping || next_thread->waiting_for != NULL) {
            dlist_pop_left(doneQueue);
            dlist_push_right(doneQueue, next_thread);
            continue;
        }

        swapcontext(&manager, next_thread->context);

        dlist_pop_left(doneQueue);

        if(next_thread->yielded || next_thread->sleeping || next_thread->waiting_for != NULL) {
            next_thread->yielded = 0;
            dlist_push_right(doneQueue, next_thread);
        }
    }
    exit(0);
}

/* on success, `dccthread_create` allocates and returns a thread
 * handle.  returns `NULL` on failure.  the new thread will execute
 * function `func` with parameter `param`.  `name` will be used to
 * identify the new thread. */
dccthread_t * dccthread_create(
                          const char *name,
                          void (*func)(int ), int param) {

    ucontext_t *newContext = malloc(sizeof(ucontext_t));
    getcontext(newContext);
    
    sigprocmask(SIG_BLOCK, &mask, NULL);

    newContext->uc_link = &manager;
    newContext->uc_stack.ss_sp = malloc( THREAD_STACK_SIZE );
    newContext->uc_stack.ss_size = THREAD_STACK_SIZE;
    newContext->uc_stack.ss_flags = 0;

    if (newContext->uc_stack.ss_sp == 0) {
        perror( "malloc: Could not allocate stack" );
        return NULL;
    }

    dccthread_t *newThread = malloc(sizeof(dccthread_t));
    strcpy(newThread->name, name);
    newThread->context = newContext;
    newThread->yielded = 0;
    newThread->sleeping = 0;
    newThread->waiting_for = NULL;
    dlist_push_right(doneQueue, newThread);

    makecontext(newContext, (void (*)(void))func, 1, param);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    
    return newThread;
}

/* `dccthread_yield` will yield the CPU (from the current thread to
 * another). */
void dccthread_yield() {
    sigprocmask(SIG_BLOCK, &mask, NULL);

    dccthread_t *currThread = dccthread_self();
    currThread->yielded = 1;
    
    swapcontext(currThread->context, &manager);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

/* `dccthread_exit` terminates the current thread, freeing all
 * associated resources. */
void dccthread_exit(void) {

    sigprocmask(SIG_BLOCK, &mask, NULL);

    dccthread_t *currThread = dccthread_self();
    int index;

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
    
    sigprocmask(SIG_BLOCK, &mask, NULL);

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
    
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void dccthread_wakeup_handler(int sig, siginfo_t *si, void *uc) {
    sigprocmask(SIG_BLOCK, &mask, NULL);
    int index;
    for(index = 0; index < doneQueue->count; index++) {
        dccthread_t *thread = dlist_get_index(doneQueue, index);
        if(thread->context == uc) {
            thread->sleeping = 0;
            break;
        }
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

/* `dccthread_sleep` stops the current thread for the time period
 * specified in `ts`. */
void dccthread_sleep(struct timespec ts) {
    //sigprocmask(SIG_BLOCK, &mask, NULL);

    //struct sigaction signal_action;
    //struct sigevent signal_event;
    //struct itimerspec interval_ts;
    //timer_t sleep_timerid;

    //dccthread_t *currThread = dccthread_self();
    //currThread->sleeping = 1;
    //
    //signal_action.sa_flags = SA_SIGINFO;
    //signal_action.sa_sigaction = dccthread_wakeup_handler;
    //sigaction(SIGRTMIN+1, &signal_action, NULL);

    //signal_event.sigev_notify = SIGEV_SIGNAL;
    //signal_event.sigev_signo = SIGRTMIN+1;

    //interval_ts.it_value = ts;
    //
    //timer_create(CLOCK_PROCESS_CPUTIME_ID, &signal_event, &sleep_timerid);
    //timer_settime(sleep_timerid, 0, &interval_ts, NULL);
    //
    //swapcontext(currThread->context, &manager);
    //
    //sigprocmask(SIG_UNBLOCK, &mask, NULL);
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
