
#include "dccthread.h"
#include "dlist.h"

#include <malloc.h>
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct dccthread {
    char name[DCCTHREAD_MAX_NAME_SIZE];
    ucontext_t *context;
} dccthread_t;

typedef struct waitingThread {
    dccthread_t *w_by;
    dccthread_t *w_for;
} waitingThread_t;

ucontext_t manager;
struct dlist *doneQueue;
struct dlist *waitingQueue;


/* `dccthread_init` initializes any state necessary for the
 * threadling library and starts running `func`.  this function
 * never returns. */
void dccthread_init(void (*func)(int), int param) {
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
    waitingQueue = dlist_create();

    dccthread_t *newThread = malloc(sizeof(dccthread_t));
    strcpy(newThread->name, "main");
    newThread->context = main;
    dlist_push_right(doneQueue, newThread);    

    while(dlist_empty(doneQueue) == 0) {
        dccthread_t *next_thread = dlist_get_index(doneQueue, 0);
        swapcontext(&manager, next_thread->context);
        dlist_pop_left(doneQueue);
    }
    exit(0);
}

/* on success, `dccthread_create` allocates and returns a thread
 * handle.  returns `NULL` on failure.  the new thread will execute
 * function `func` with parameter `param`.  `name` will be used to
 * identify the new thread. */
dccthread_t * dccthread_create(const char *name,
		void (*func)(int ), int param) {
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
    dlist_push_right(doneQueue, newThread);    

    return newThread;    
}

/* `dccthread_yield` will yield the CPU (from the current thread to
 * another). */
void dccthread_yield(void) {
    dccthread_t *currThread = dlist_get_index(doneQueue, 0);
    dlist_push_right(doneQueue, currThread);
    swapcontext(currThread->context, &manager);
}


/* `dccthread_exit` terminates the current thread, freeing all
 * associated resources. */
void dccthread_exit(void) {
    dccthread_t *currThread = dccthread_self();
    int index;
    
    //checks if there is any thread waiting for it to finish
    for(index=0; index<waitingQueue->count; index++) {
        waitingThread_t *wt = dlist_get_index(waitingQueue, index);
        if(wt->w_for == currThread) {
            //needs to remove the item from the waiting list
            dlist_push_right(doneQueue, wt->w_by);
        }
    }
    free(currThread->context->uc_stack.ss_sp);
}

/* `dccthread_wait` blocks the current thread until thread `tid`
 * terminates. */
void dccthread_wait(dccthread_t *tid) {
    int index;
    for(index=0; index<doneQueue->count; index++) {
        dccthread_t *thread = dlist_get_index(doneQueue, index);
        if(thread == tid) break;
    }
    
    //otherwise the target thread has already finised
    if(index != doneQueue->count) {
        dccthread_t *currThread = dccthread_self();

        waitingThread_t *wt = malloc(sizeof(waitingThread_t));
        wt->w_by = currThread;
        wt->w_for = tid;
        dlist_push_right(waitingQueue, wt);
        
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


