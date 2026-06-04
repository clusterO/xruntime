#include "thread_pool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/**
 * Worker thread function that processes tasks from the queue
 */
static void *worker_thread(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    
    while (1) {
        pthread_mutex_lock(&(pool->lock));
        
        // Wait for tasks or shutdown signal
        while (pool->task_count == 0 && !pool->shutdown) {
            pthread_cond_wait(&(pool->condition), &(pool->lock));
        }
        
        // Check for shutdown
        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }
        
        // Get the next task
        Task *task = pool->task_queue_head;
        if (task == NULL) {
            pthread_mutex_unlock(&(pool->lock));
            continue;
        }
        
        // Remove task from queue
        pool->task_queue_head = task->next;
        if (pool->task_queue_head == NULL) {
            pool->task_queue_tail = NULL;
        }
        pool->task_count--;
        
        pthread_mutex_unlock(&(pool->lock));
        
        // Execute the task
        task->function(task->argument);
        
        // Free the task
        free(task);
    }
    
    return NULL;
}

ThreadPool *thread_pool_create(int thread_count)
{
    if (thread_count <= 0) {
        fprintf(stderr, "Thread count must be positive\n");
        return NULL;
    }
    
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        perror("Failed to allocate thread pool");
        return NULL;
    }
    
    pool->thread_count = thread_count;
    pool->task_queue_head = NULL;
    pool->task_queue_tail = NULL;
    pool->task_count = 0;
    pool->shutdown = false;
    
    // Initialize mutex and condition variable
    if (pthread_mutex_init(&(pool->lock), NULL) != 0) {
        perror("Failed to initialize mutex");
        free(pool);
        return NULL;
    }
    
    if (pthread_cond_init(&(pool->condition), NULL) != 0) {
        perror("Failed to initialize condition variable");
        pthread_mutex_destroy(&(pool->lock));
        free(pool);
        return NULL;
    }
    
    // Allocate threads
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    if (pool->threads == NULL) {
        perror("Failed to allocate threads");
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->condition));
        free(pool);
        return NULL;
    }
    
    // Create worker threads
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, worker_thread, pool) != 0) {
            perror("Failed to create thread");
            
            // Cleanup already created threads
            for (int j = 0; j < i; j++) {
                pool->shutdown = true;
                pthread_cond_broadcast(&(pool->condition));
                pthread_join(pool->threads[j], NULL);
            }
            
            free(pool->threads);
            pthread_mutex_destroy(&(pool->lock));
            pthread_cond_destroy(&(pool->condition));
            free(pool);
            return NULL;
        }
    }
    
    printf("Thread pool created with %d worker threads\n", thread_count);
    return pool;
}

int thread_pool_submit(ThreadPool *pool, void (*function)(void *), void *argument)
{
    if (pool == NULL || function == NULL) {
        return -1;
    }
    
    // Create new task
    Task *task = (Task *)malloc(sizeof(Task));
    if (task == NULL) {
        perror("Failed to allocate task");
        return -1;
    }
    
    task->function = function;
    task->argument = argument;
    task->next = NULL;
    
    // Add task to queue
    pthread_mutex_lock(&(pool->lock));
    
    if (pool->task_queue_tail == NULL) {
        pool->task_queue_head = task;
        pool->task_queue_tail = task;
    } else {
        pool->task_queue_tail->next = task;
        pool->task_queue_tail = task;
    }
    pool->task_count++;
    
    // Signal a worker thread
    pthread_cond_signal(&(pool->condition));
    
    pthread_mutex_unlock(&(pool->lock));
    
    return 0;
}

void thread_pool_wait(ThreadPool *pool)
{
    if (pool == NULL) {
        return;
    }
    
    pthread_mutex_lock(&(pool->lock));
    
    // Wait for all tasks to complete
    while (pool->task_count > 0) {
        pthread_cond_wait(&(pool->condition), &(pool->lock));
    }
    
    pthread_mutex_unlock(&(pool->lock));
}

void thread_pool_destroy(ThreadPool *pool)
{
    if (pool == NULL) {
        return;
    }
    
    // Signal shutdown
    pthread_mutex_lock(&(pool->lock));
    pool->shutdown = true;
    pthread_cond_broadcast(&(pool->condition));
    pthread_mutex_unlock(&(pool->lock));
    
    // Wait for all threads to finish
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    // Free remaining tasks
    Task *task = pool->task_queue_head;
    while (task != NULL) {
        Task *next = task->next;
        free(task);
        task = next;
    }
    
    // Cleanup resources
    free(pool->threads);
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->condition));
    free(pool);
    
    printf("Thread pool destroyed\n");
}
