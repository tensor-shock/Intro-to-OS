// https://github.com/remzi-arpacidusseau/ostep-code/blob/master/threads-sema/zemaphore.h
#ifndef __zemaphore_h__
#define __zemaphore_h__ 
#include <pthread.h>
// #include "common.h"
#include "common_threads.h"

typedef struct __Zem_t {
    int value;
    pthread_cond_t  cond;
    pthread_mutex_t lock;
} Zem_t;

void Zem_init(Zem_t *z, int value) {
    z->value = value;
    Cond_init(&z->cond);
    Mutex_init(&z->lock);
}

void Zem_wait(Zem_t *z) {
    Mutex_lock(&z->lock);
    while (z->value <= 0)
	Cond_wait(&z->cond, &z->lock);
    z->value--;
    Mutex_unlock(&z->lock);
}

void Zem_post(Zem_t *z) {
    Mutex_lock(&z->lock);
    z->value++;
    Cond_signal(&z->cond);
    Mutex_unlock(&z->lock);
}

#ifdef __APPLE__
typedef Zem_t sem_t;

#define sem_wait(s)    Zem_wait(s)
#define sem_post(s)    Zem_post(s)
#define sem_init(s,a,v) Zem_init(s, v)
#endif

#endif // __zemaphore_h__