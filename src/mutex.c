```c
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include "read.h"

int rwlock_init(ReadWrite_Lock *rw)
{
    if (rw == NULL) {
        return -1;
    }

    rw->reader = 0;

    if (pthread_mutex_init(&rw->reader_count, NULL) != 0) {
        return -1;
    }

    if (pthread_mutex_init(&rw->writer_count, NULL) != 0) {
        pthread_mutex_destroy(&rw->reader_count);
        return -1;
    }

    if (sem_init(&rw->resource, 0, 1) != 0) {
        pthread_mutex_destroy(&rw->reader_count);
        pthread_mutex_destroy(&rw->writer_count);
        return -1;
    }

    return 0;
}


/* Reader Entry */
void reader_enter(ReadWrite_Lock *lock)
{
    /*
     * Block new readers while a writer is waiting or active.
     */
    pthread_mutex_lock(&lock->writer_count);

    pthread_mutex_lock(&lock->reader_count);

    lock->reader++;

    /*
     * First reader locks the shared resource,
     * preventing writers from entering.
     */
    if (lock->reader == 1) {
        sem_wait(&lock->resource);
    }

    pthread_mutex_unlock(&lock->reader_count);

    pthread_mutex_unlock(&lock->writer_count);
}


/* Reader Exit */
void reader_exit(ReadWrite_Lock *rw)
{
    pthread_mutex_lock(&rw->reader_count);

    rw->reader--;

    /*
     * Last reader releases the resource,
     * allowing a writer to enter.
     */
    if (rw->reader == 0) {
        sem_post(&rw->resource);
    }

    pthread_mutex_unlock(&rw->reader_count);
}


/* Writer Entry */
void writer_enter(ReadWrite_Lock *lock)
{
    /*
     * Only one writer can proceed at a time.
     */
    pthread_mutex_lock(&lock->writer_count);

    /*
     * Wait until all readers have left.
     */
    sem_wait(&lock->resource);
}


/* Writer Exit */
void writer_exit(ReadWrite_Lock *lock)
{
    /* Release the shared resource */
    sem_post(&lock->resource);

    /* Allow another writer or readers */
    pthread_mutex_unlock(&lock->writer_count);
}


void rwlock_destroy(ReadWrite_Lock *rw)
{
    if (rw == NULL) {
        return;
    }

    pthread_mutex_destroy(&rw->reader_count);
    pthread_mutex_destroy(&rw->writer_count);
    sem_destroy(&rw->resource);
}
```
