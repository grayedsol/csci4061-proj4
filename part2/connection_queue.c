#include "connection_queue.h"

#include <stdio.h>
#include <string.h>

int connection_queue_init(connection_queue_t *queue) {
    for (int i = 0; i < CAPACITY; i++) {
        queue->client_fds[i] = -1;
    }
    queue->length = 0;
    queue->read_idx = 0;
    queue->write_idx = 0;
    queue->shutdown = 0;
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->queue_full, NULL);
    pthread_cond_init(&queue->queue_empty, NULL);

    return 0;
}

int connection_queue_enqueue(connection_queue_t *queue, int connection_fd) {
    pthread_mutex_lock(&queue->lock);

    while (queue->length == CAPACITY) {
        pthread_cond_wait(&queue->queue_full, &queue->lock);
    }
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    // Add item to queue
    queue->client_fds[queue->write_idx] = connection_fd;
    queue->write_idx = queue->write_idx + 1 == CAPACITY ? 0 : queue->write_idx + 1;
    queue->length++;

    if (queue->length == 1) { pthread_cond_signal(&queue->queue_empty); }

    pthread_mutex_unlock(&queue->lock);

    return 0;
}

int connection_queue_dequeue(connection_queue_t *queue) {
    int result;
    int ret_val = -1;
    if((result = pthread_mutex_lock(&queue->lock)) != 0){
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }

    while (queue->length == 0) {
        if((pthread_cond_wait(&queue->queue_empty, &queue->lock)) != 0){
            fprintf(stderr, "pthread_cond_wait: %s\n", strerror(result));
            return -1;
        }
    }
    if (queue->shutdown) {
        if((result = pthread_mutex_unlock(&queue->lock)) != 0){
            fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
            return -1;
        }
        return -1;
    }
    // Remove item from queue
    ret_val = queue->client_fds[queue->read_idx];
    queue->client_fds[queue->read_idx] = -1;
    queue->read_idx = queue->read_idx + 1 == CAPACITY ? 0 : queue->read_idx + 1;
    queue->length--;

    if (queue->length == CAPACITY - 1) { 
        if((result = pthread_cond_signal(&queue->queue_full)) != 0){
            fprintf(stderr, "pthread_cond_signal: %s\n", strerror(result));
            return -1;
        }
    }

    if((result = pthread_mutex_unlock(&queue->lock)) != 0){
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }

    return ret_val;
}

int connection_queue_shutdown(connection_queue_t *queue) {
    int result;
    if((result = pthread_mutex_lock(&queue->lock)) != 0){
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }
    queue->shutdown = 1;
    queue->length = -1;
    if((result = pthread_cond_broadcast(&queue->queue_full)) != 0){
        fprintf(stderr, "pthread_cond_broadcast: %s\n", strerror(result));
        return -1;
    }
    if((result = pthread_cond_broadcast(&queue->queue_empty)) != 0){
        fprintf(stderr, "pthread_cond_broadcast: %s\n", strerror(result));
        return -1;
    }
    if((result = pthread_mutex_unlock(&queue->lock)) != 0){
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }
    return 0;
}

int connection_queue_free(connection_queue_t *queue) {
    int result;
    if((result = pthread_mutex_destroy(&queue->lock)) != 0){
        fprintf(stderr, "pthread_mutex_destroy: %s\n", strerror(result));
        return -1;
    }
    if((result = pthread_cond_destroy(&queue->queue_full)) != 0){
        fprintf(stderr, "pthread_cond_destroy: %s\n", strerror(result));
        return -1;
    }
    if((result = pthread_cond_destroy(&queue->queue_empty)) != 0){
        fprintf(stderr, "pthread_cond_destroy: %s\n", strerror(result));
        return -1;
    }
    return 0;
}
