#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Optional: use these functions to add debug or error prints to your application
// #define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    thread_func_args->thread_complete_success = false;

    struct timespec ts_sleep;
    ts_sleep.tv_sec = thread_func_args->milliseconds_to_sleep / 1000;
    ts_sleep.tv_nsec = (thread_func_args->milliseconds_to_sleep % 1000) * 1000000;
    DEBUG_LOG("Start sleep..");
    nanosleep(&ts_sleep, NULL);
    
    DEBUG_LOG("Lock mutex..");
    int ret_lock = pthread_mutex_lock(thread_func_args->mutex);
    if (0 != ret_lock) {
        ERROR_LOG("pthread_mutex_lock failed.");
        return thread_param;
    }

    ts_sleep.tv_sec = thread_func_args->milliseconds_until_release / 1000;
    ts_sleep.tv_nsec = (thread_func_args->milliseconds_until_release % 1000) * 1000000;
    nanosleep(&ts_sleep, NULL);

    DEBUG_LOG("Unlock mutex..");
    int ret_unlock = pthread_mutex_unlock(thread_func_args->mutex);
    if (0 != ret_unlock) {
        ERROR_LOG("pthread_mutex_unlock failed.");
        return thread_param;
    }

    thread_func_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    struct thread_data* thread_data_ = malloc(sizeof(struct thread_data));
    thread_data_->milliseconds_to_sleep = wait_to_obtain_ms;
    thread_data_->milliseconds_until_release = wait_to_release_ms;
    thread_data_->mutex = mutex;

    DEBUG_LOG("Create thread..");
    int ret_thread_create = pthread_create(thread,
        NULL,
        &threadfunc,
        (void*) thread_data_);
    if (0 != ret_thread_create) {
        ERROR_LOG("Creating thread failed.");
        return false;
    }

    return true;
}

