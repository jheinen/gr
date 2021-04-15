#ifdef _MSC_VER
#define NO_THREADS 1
#endif
#ifndef NO_THREADS
#include "threadpool.h"

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef void *threadpool_work_t;
threadpool_work_t next_work_item;

static void *threadpool_worker(void *arg)
{
  threadpool_t *tp = arg;
  threadpool_work_t work;

  pthread_mutex_lock(&(tp->work_mutex));
  while (1)
    {
      while (next_work_item == NULL && !tp->stop) pthread_cond_wait(&(tp->wait_for_work_cond), &(tp->work_mutex));

      if (tp->stop) break;

      work = next_work_item;
      next_work_item = NULL;
      pthread_cond_signal(&(tp->work_fetched));
      tp->working_cnt++;
      pthread_mutex_unlock(&(tp->work_mutex));

      if (work != NULL)
        {
          tp->worker_func(work);
        }

      pthread_mutex_lock(&(tp->work_mutex));
      tp->working_cnt--;
      if (!tp->stop && tp->working_cnt == 0 && next_work_item == NULL) pthread_cond_signal(&(tp->all_work_done_cond));
    }

  tp->thread_cnt--;
  pthread_cond_signal(&(tp->all_work_done_cond));
  pthread_mutex_unlock(&(tp->work_mutex));
  return NULL;
}

void threadpool_create(threadpool_t *tp, size_t num, worker_func_t worker_func)
{
  size_t i;
  int ret;

  num = max(num, 1);

  tp->worker_func = worker_func;
  tp->thread_cnt = num;
  tp->threads = calloc(num, sizeof(pthread_t));
  pthread_mutex_init(&(tp->work_mutex), NULL);
  pthread_cond_init(&(tp->wait_for_work_cond), NULL);
  pthread_cond_init(&(tp->work_fetched), NULL);
  pthread_cond_init(&(tp->all_work_done_cond), NULL);
  tp->working_cnt = 0;
  tp->stop = 0;

  for (i = 0; i < num; i++)
    {
      ret = pthread_create(&tp->threads[i], NULL, threadpool_worker, tp);
    }
}

void threadpool_destroy(threadpool_t *tp)
{
  int thread_cnt;
  int i;

  if (tp == NULL) return;

  threadpool_wait(tp);

  pthread_mutex_lock(&(tp->work_mutex));
  thread_cnt = tp->thread_cnt;
  tp->stop = 1;
  pthread_cond_broadcast(&(tp->wait_for_work_cond));
  pthread_mutex_unlock(&(tp->work_mutex));

  for (i = 0; i < thread_cnt; ++i)
    {
      pthread_join(tp->threads[i], NULL);
    }

  pthread_mutex_destroy(&(tp->work_mutex));
  pthread_cond_destroy(&(tp->wait_for_work_cond));
  pthread_cond_destroy(&(tp->work_fetched));
  pthread_cond_destroy(&(tp->all_work_done_cond));
  free(tp->threads);
  free(tp);
}

void threadpool_add_work(threadpool_t *tp, void *arg)
{
  pthread_mutex_lock(&(tp->work_mutex));

  if (next_work_item != NULL)
    {
      pthread_cond_wait(&(tp->work_fetched), &(tp->work_mutex));
    }
  next_work_item = arg;

  pthread_cond_signal(&(tp->wait_for_work_cond));
  pthread_mutex_unlock(&(tp->work_mutex));
}

void threadpool_wait(threadpool_t *tp)
{
  if (tp == NULL) return;

  pthread_mutex_lock(&(tp->work_mutex));
  while ((!tp->stop && tp->working_cnt != 0) || (tp->stop && tp->thread_cnt != 0))
    {
      pthread_cond_wait(&(tp->all_work_done_cond), &(tp->work_mutex));
    }
  pthread_mutex_unlock(&(tp->work_mutex));
}
#endif
