#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection_queue.h"
#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5
#define N_THREADS 5

int keep_going = 1;
const char *serve_dir;

void handle_sigint(int signo) {
    keep_going = 0;
}

void* worker_thread_func(void* arg) {
    connection_queue_t* queue = (connection_queue_t*)arg;
    int client_fd = -1;
    while ((client_fd = connection_queue_dequeue(queue)) != -1) {
        char resource_name[BUFSIZE] = { 0 };
        if (read_http_request(client_fd, resource_name) < 0) {
            close(client_fd);
            continue;
        }

        char resource_path[BUFSIZE] = { 0 };
        strncpy(resource_path, serve_dir, BUFSIZE - 1);
        strncat(resource_path, "/", 2);
        strncat(resource_path, resource_name, BUFSIZE - strlen(resource_path) - 1);

        if (write_http_response(client_fd, resource_path) < 0) {
            close(client_fd);
            continue;
        }

        if (close(client_fd) < 0) {
            perror("close");
        }
    }
    
    return NULL;
}

int main(int argc, char **argv) {
    // First argument is directory to serve, second is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    // Server directory and port definitions:
    serve_dir = argv[1];
    const char* port = argv[2];

    // Set up connection queue
    connection_queue_t queue;
    connection_queue_init(&queue);

    // Block SIGINT for now
    sigset_t sig_blocked;
    sigset_t sig_unblocked;
    sigemptyset(&sig_blocked);
    sigaddset(&sig_blocked, SIGINT);
    sigprocmask(SIG_BLOCK, &sig_blocked, &sig_unblocked);

    // Set up SIGINT handler
    struct sigaction sig;
    sig.sa_handler = handle_sigint;
    sigfillset(&sig.sa_mask);
    sig.sa_flags = 0;
    if (sigaction(SIGINT, &sig, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    // Socket Setup
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo* server;
    
    // Set up address info
    if (getaddrinfo(NULL, port, &hints, &server) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    // Create socket file descriptor
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (sock_fd < 0) {
        perror("socket");
        freeaddrinfo(server);
        return 1;
    }
    
    // Bind socket to port
    if (bind(sock_fd, server->ai_addr, server->ai_addrlen) < 0) {
        perror("bind");
        freeaddrinfo(server);
        close(sock_fd);
        return 1;
    }

    // Free the no longer needed address info
    freeaddrinfo(server);

    // Designate socket as server socket
    if (listen(sock_fd, LISTEN_QUEUE_LEN) < 0) {
        perror("listen");
        close(sock_fd);
        return 1;
    }

    // Create worker threads
    int thread_result;
    pthread_t worker_threads[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        if ((thread_result = pthread_create(&worker_threads[i], NULL, worker_thread_func, &queue)) != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(thread_result));
        }
    }

    // Unblock SIGINT
    sigprocmask(SIG_SETMASK, &sig_unblocked, NULL);

    // Main Server Loop
    while (keep_going) {
        int client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno != EINTR) {
                perror("accept");
                close(sock_fd);
                return 1;
            }
            break;
        }

        if (connection_queue_enqueue(&queue, client_fd) < 0) {
            keep_going = 0;
        }
    }

    connection_queue_shutdown(&queue);

    // Clean up worker threads
    for (int i = 0; i < N_THREADS; i++) {
        if ((thread_result = pthread_join(worker_threads[i], NULL)) != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(thread_result));
        }
    }

    // Cleanup
    connection_queue_free(&queue);

    if (close(sock_fd) < 0) {
        perror("close");
        return 1;
    }
    return 0;
}
