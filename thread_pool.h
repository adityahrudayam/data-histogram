#ifndef thread_pool_h
#define thread_pool_h

#include <pthread.h>

// returns the no. of cores/processors in the underlying system architecture or hardware
int get_num_cores(void);

typedef struct {
    void (*execute)(void*); // ptr to task
    void *args; // task params
} task_def;

typedef struct queue_node {
    struct queue_node *prev;
    struct queue_node *next;
    task_def *task;
    int tid;
} queue_node;

queue_node* create_node(queue_node *,queue_node *,task_def *,int);

typedef struct {
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    pthread_mutex_t mtx;
    queue_node *head;
    queue_node *tail;
    int tasks_count;
    int next_free_id; // Can be replaced with a heap to store the first free number
} blocking_queue;

blocking_queue* init_queue(void);

char enqueue_task(blocking_queue *,task_def *);

queue_node* dequeue_task_and_return(blocking_queue *);

// thread_pool data structure
typedef struct {
    pthread_rwlock_t lock; // not_terminated
    blocking_queue *tasks_queue;
    pthread_t  *threads;
    int thread_count;
    unsigned char not_terminated;
} thread_pool;

// Function for initializing the thread pool
thread_pool* init_thread_pool(void);

void destroy_thread_pool(thread_pool*);

void terminate_threads(thread_pool*);

void submit_task_to_pool(thread_pool *,void (*)(void*),void *);

void* collect_task_and_execute(void *);


#endif

