#ifndef THREADPOOL_H_INCLUDED
#define THREADPOOL_H_INCLUDED

#ifdef _MSC_VER
#define NO_THREADS 1
#endif
#ifndef NO_THREADS

#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef void (*worker_func_t)(void *arg);
typedef struct threadpool
{
  worker_func_t worker_func;
  pthread_mutex_t work_mutex;
  pthread_cond_t wait_for_work_cond;
  pthread_cond_t work_fetched;
  pthread_cond_t all_work_done_cond;
  size_t working_cnt;
  size_t thread_cnt;
  pthread_t *threads;
  int stop;
} threadpool_t;

void threadpool_create(threadpool_t *tp, size_t num, worker_func_t func);
void threadpool_destroy(threadpool_t *tp);
void threadpool_add_work(threadpool_t *tp, void *arg);
void threadpool_wait(threadpool_t *tp);

#endif
#endif /* ifndef THREADPOOL_H_INCLUDED */
