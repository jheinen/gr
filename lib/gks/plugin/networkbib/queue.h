#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#ifndef QUEUEH
#define QUEUEH
#define SUCCESS 0
#define ERR_INVAL 1
#define ERR_NOMEM 2
#define FALSE 0
#define TRUE 1
//.
struct queue_node_n {
    struct queue_node_n *next;
    void *data;
};

struct queue_q{
    struct queue_node_n *start;
    struct queue_node_n *back;
};

int queue_destroy(struct queue_q **queue);

int queue_empty(struct queue_q *queue);

struct queue_q *queue_new(void);

void *queue_dequeue(struct queue_q *queue);

int queue_enqueue(struct queue_q *queue, void *data);

int queue_length(struct queue_q *queue);

#endif
