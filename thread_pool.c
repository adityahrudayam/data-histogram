#include <stdlib.h>
#include <stdio.h>

#include "thread_pool.h"


#ifdef _WIN32
// For Windows OS
#include <windows.h>

int get_num_cores(void){
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
}

#elif defined(__linux__) || defined(__APPLE__)
// For Linux OS or MacOS (POSIX)
#include <unistd.h>

int get_num_cores(void){
    int num_cores=(int)sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores<1)
        perror("An error occurred while trying to get processor count.\n");
    return num_cores;
}

#else
    perror("Could not identify the underlying operating system!\n");
#endif


// Blocking Queue implementation

int max_size=1e5; // 10^5 tasks at max in the queue at any point in the program execution


queue_node* create_node(queue_node *prev,queue_node *next,task_def *task,int tid){
    queue_node *curr=(queue_node*)malloc(sizeof(queue_node));
    if(!curr) return NULL;
    curr->prev=prev;
    curr->next=next;
    curr->task=task;
    curr->tid=tid;
    return curr;
}


blocking_queue* init_queue(void){
    blocking_queue *queue=(blocking_queue*)malloc(sizeof(blocking_queue));
    if(!queue) return NULL;
    pthread_cond_init(&queue->not_empty,NULL);
    pthread_cond_init(&queue->not_full,NULL);
    pthread_mutex_init(&queue->mtx,NULL);
    queue->head=queue->tail=NULL;
    queue->next_free_id=0;
    queue->tasks_count=0;
    return queue;
}


char enqueue_task(blocking_queue *queue,task_def *task){
    pthread_mutex_lock(&queue->mtx); // lock acquired
    while(queue->tasks_count==max_size){
        printf("Queue is full and cannot load any more!\n");
        pthread_cond_wait(&queue->not_full,&queue->mtx);
    }
    queue_node *curr=create_node(NULL,queue->tail,task,queue->next_free_id);
    if(!curr){
        free(task);
        perror("Memory could not be allocated!\n");
        pthread_mutex_unlock(&queue->mtx); // lock released
        return 0; // could not be enqueued
    }
    if(queue->tasks_count==0)
        queue->head=queue->tail=curr;
    else{
        queue->tail->prev=curr;
        queue->tail=curr;
    }
    queue->tasks_count++;
    queue->next_free_id++; // Can be replaced with a heap to store the first free number
    pthread_cond_broadcast(&queue->not_empty); // condition signalled for 'not empty'
    pthread_mutex_unlock(&queue->mtx); // lock released
    return 1; // successfully enqueued
}


queue_node* dequeue_task_and_return(blocking_queue *queue){
    pthread_mutex_lock(&queue->mtx); // lock acquired
    while(queue->tasks_count==0){
        printf("Queue is empty and cannot take out items any more!\n");
        // wait for the queue to fill up with tasks
        pthread_cond_wait(&queue->not_empty,&queue->mtx);
    }
    queue_node *res=queue->head;
    if(queue->head==queue->tail)
        queue->head=queue->tail=NULL;
    else{
        if(queue->head){
            queue->head=queue->head->prev;
            queue->head->next=NULL;
            res->prev=NULL;
        }
    }
    queue->tasks_count--;
    pthread_cond_broadcast(&queue->not_full); // condition signalled for 'not full'
    pthread_mutex_unlock(&queue->mtx); // lock released
    return res;
}


// thread-pool implementation

thread_pool* init_thread_pool(void){
    thread_pool *pool=(thread_pool*)malloc(sizeof(thread_pool));
    if(!pool) return NULL;
    pool->not_terminated=1;
    pool->tasks_queue=init_queue();
    pool->thread_count=get_num_cores();
    pool->threads=(pthread_t*)malloc(sizeof(pthread_t)*pool->thread_count);
    if(!pool->threads){
        free(pool);
        return NULL;
    }
    pthread_rwlock_init(&pool->lock,NULL);
    for(int i=0;i<pool->thread_count;i++){
        if(pthread_create(&pool->threads[i],NULL,collect_task_and_execute,pool)!=0){
            free(pool->threads);
            free(pool->tasks_queue);
            free(pool);
            perror("An error occurred while trying to create threads!\n");
            
            #if defined(__linux__) || defined(__APPLE__)
            exit(EXIT_FAILURE);
            #endif
            
            return NULL;
        }
        if(pthread_detach(pool->threads[i])!=0){
            perror("pthread_detach failed for threads!\n");
            #if defined(__linux__) || defined(__APPLE__)
            exit(EXIT_FAILURE);
            #endif
        }
    }
    return pool;
}


void destroy_thread_pool(thread_pool *pool){
    // if tasksqueue is not empty, finish executing the rest of the tasks
    while(pool->tasks_queue->tasks_count>0){
        queue_node *node=dequeue_task_and_return(pool->tasks_queue);
        node->task->execute(node->task->args);
        // free memory - node
        free(node->task);
        free(node);
    }
    pthread_mutex_destroy(&pool->tasks_queue->mtx);
    pthread_cond_destroy(&pool->tasks_queue->not_empty);
    pthread_cond_destroy(&pool->tasks_queue->not_full);
    pthread_rwlock_destroy(&pool->lock);
    // free memory - pool
    free(pool->threads);
    free(pool->tasks_queue);
    free(pool);
}


void terminate_threads(thread_pool *pool){
    pthread_rwlock_wrlock(&pool->lock);
    pool->not_terminated=0;
    pthread_rwlock_unlock(&pool->lock);
}


void submit_task_to_pool(thread_pool *pool,void (*f)(void*),void *args){
    task_def *task=(task_def*)malloc(sizeof(task_def));
    if(!task) return;
    task->args=args;
    task->execute=f;
    enqueue_task(pool->tasks_queue,task);
}


void* collect_task_and_execute(void *arg_pool){
    thread_pool *pool=(thread_pool*)arg_pool;
    while(1){
        pthread_rwlock_rdlock(&pool->lock);
        if(!pool->not_terminated){
            pthread_rwlock_unlock(&pool->lock);
            break;
        }
        pthread_rwlock_unlock(&pool->lock);
        queue_node *node=dequeue_task_and_return(pool->tasks_queue);
        node->task->execute(node->task->args);
        // free memory
        free(node->task);
        free(node);
    }
    pthread_exit(NULL);
    return NULL;
}

