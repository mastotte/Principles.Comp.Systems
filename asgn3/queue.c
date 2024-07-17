#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "queue.h"

typedef struct queue {
    void **element_list;
    size_t size;
    size_t num_elements;
    size_t pop_index;
    size_t push_index;
    pthread_mutex_t mutex;
    sem_t num_free_cells;
    sem_t num_used_cells;
    int in;
    int out;
} queue_t;

queue_t *queue_new(int size) {
    queue_t *q = malloc(sizeof(queue_t));
    if (q) {
        q->size = size;
        q->element_list = (void **) malloc(size * sizeof(void *));
        q->in = 0;
        q->out = 0;
        q->num_elements = 0;
        sem_init(&q->num_used_cells, 0, 1);
        sem_init(&q->num_free_cells, 0, 1);
        pthread_mutex_init(&q->mutex, NULL);
    }
    return q;
}

void queue_delete(queue_t **q) {
    if (*q) {
        sem_destroy(&(*q)->num_used_cells);
        sem_destroy(&(*q)->num_used_cells);
        sem_destroy(&(*q)->num_used_cells);
        free((*q)->element_list);
        (*q)->element_list = NULL;
        free(*q);
        *q = NULL;
    }
}
bool queue_push(queue_t *q, void *elem) {

    if (q == NULL) {
        return false;
    }

    while (q->num_elements == q->size) {
        sem_wait(&q->num_free_cells);
    }
    pthread_mutex_lock(&q->mutex);

    q->element_list[q->in] = elem;
    q->in = (q->in + 1) % (q->size);
    q->num_elements++;

    pthread_mutex_unlock(&q->mutex);
    sem_post(&q->num_used_cells);

    return true;
}
bool queue_pop(queue_t *q, void **elem) {

    if (q == NULL) {
        return false;
    }

    while (q->num_elements == 0) {
        sem_wait(&q->num_used_cells);
    }
    pthread_mutex_lock(&q->mutex);

    *elem = q->element_list[q->out];
    q->out = (q->out + 1) % (q->size);
    q->num_elements--;

    pthread_mutex_unlock(&q->mutex);
    sem_post(&q->num_free_cells);

    return true;
}
