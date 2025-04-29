#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5

int keep_going = 1;

void handle_sigint(int signo) {
    keep_going = 0;
}

int main(int argc, char **argv) {
    // First argument is directory to serve, second is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    // Uncomment the lines below to use these definitions:
    const char* serve_dir = argv[1];
    const char* port = argv[2];

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

    // TODO Main Server Loop

    // Cleanup
    if (close(sock_fd) < 0) {
        perror("close");
        return 1;
    }
    return 0;
}
