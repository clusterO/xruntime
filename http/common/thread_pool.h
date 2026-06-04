#ifndef _THREAD_POOL_HEADER_
#define _THREAD_POOL_HEADER_

#include <pthread.h>
#include <stdbool.h>

/**
 * Task structure for work items in the thread pool
 */
typedef struct Task {
    void (*function)(void *);
    void *argument;
    struct Task *next;
} Task;

/**
 * Thread pool structure for managing worker threads
 */
typedef struct {
    pthread_t *threads;
    Task *task_queue_head;
    Task *task_queue_tail;
    int thread_count;
    int task_count;
    bool shutdown;
    pthread_mutex_t lock;
    pthread_cond_t condition;
} ThreadPool;

/**
 * Create a new thread pool with specified number of threads
 * 
 * @param thread_count Number of worker threads to create
 * @return Pointer to the created thread pool, or NULL on failure
 */
ThreadPool *thread_pool_create(int thread_count);

/**
 * Submit a task to the thread pool for execution
 * 
 * @param pool Pointer to the thread pool
 * @param function Function pointer to the task
 * @param argument Argument to pass to the task function
 * @return 0 on success, -1 on failure
 */
int thread_pool_submit(ThreadPool *pool, void (*function)(void *), void *argument);

/**
 * Wait for all tasks to complete and shutdown the thread pool
 * 
 * @param pool Pointer to the thread pool
 */
void thread_pool_wait(ThreadPool *pool);

/**
 * Destroy the thread pool and free all resources
 * 
 * @param pool Pointer to the thread pool
 */
void thread_pool_destroy(ThreadPool *pool);

#endif
