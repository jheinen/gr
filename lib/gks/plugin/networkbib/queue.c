#include"queue.h"

int queue_destroy(struct queue_q **queue){
    if (*queue == NULL) {
        return SUCCESS;
    }
    while ((*queue)->start != NULL){
        struct queue_node_n *node = (*queue)->start;
        (*queue)->start = node->next;
        free(node);
    }
    free(*queue);
    *queue = NULL;
    return SUCCESS;
}

int queue_empty(struct queue_q *queue) {
    if (queue == NULL || queue->start == NULL) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

struct queue_q *queue_new(void) {
    struct queue_q* queue = malloc(sizeof(struct queue_q));
    if (queue == NULL) {
        return NULL;
    }
    queue->start = queue->back = NULL;
    return queue;
}

void *queue_dequeue(struct queue_q *queue) { /*take from queue*/
    if (queue == NULL || queue->start == NULL) {
        return NULL;
    }
    struct queue_node_n *node = queue->start;
    void *data = node->data;
    queue->start = node->next;
    if (queue->start == NULL) {
        queue->back = NULL;
    }
    free(node);
    return data; /*returns the old start node*/
}

int queue_enqueue(struct queue_q *queue, void *data) {
    if (queue == NULL) {
        return ERR_INVAL;
    }
    struct queue_node_n *node = malloc(sizeof(*node));
    if (node == NULL) {
        return ERR_NOMEM;
    }
    node->data = data;
    node->next = NULL;
    if (queue->back == NULL) {
        queue->start = queue->back = node;
    } else{
        queue->back->next = node;
        queue->back = node;
    }
    return SUCCESS;
}

int queue_length(struct queue_q *queue){
    int length = 0;
    if (queue == NULL || queue->start == NULL){
        return length;
    }
    length+= 1;
    struct queue_node_n* node = queue->start;

    while(1){
        if (node == queue->back){
            return length;
        }
        else{
            length+=1;
            node = node->next;
        }
    }
    return 0;
}

void queue_merge(struct queue_q **first, struct queue_q *second){
  if ((*first)->back != NULL){
    (*first)->back->next = second->start;
  }
  else{
    (*first)->start = second->start;
  }
  (*first)->back = second->back;
  second->start = second->back = NULL;
  return;
}
