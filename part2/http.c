#include "http.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFSIZE 512

const char *get_mime_type(const char *file_extension) {
    if (strcmp(".txt", file_extension) == 0) {
        return "text/plain";
    } else if (strcmp(".html", file_extension) == 0) {
        return "text/html";
    } else if (strcmp(".jpg", file_extension) == 0) {
        return "image/jpeg";
    } else if (strcmp(".png", file_extension) == 0) {
        return "image/png";
    } else if (strcmp(".pdf", file_extension) == 0) {
        return "application/pdf";
    } else if (strcmp(".mp3", file_extension) == 0) {
        return "audio/mpeg";
    }

    return NULL;
}

static const char* not_found_string =
"HTTP/1.0 404 Not Found\r\n"
"Content-Length: 0\r\n"
"\r\n";

static const char* ok_string =
"HTTP/1.0 200 OK\r\n"
"Content-Type: %s\r\n"
"Content-Length: %d\r\n"
"\r\n";

int get_line(int fd, char* line_buf, int n) {
    for (int i = 0; i < n; i++) {
        if (read(fd, line_buf + i, 1) < 0) {
            perror("read");
            return -1;
        }
        else if (!*(line_buf + i)) { break; }

        if (*(line_buf + i) == '\n' && i) {
            if (*(line_buf + i - 1) == '\r') {
                break;
            }
        }
    }

    return 0;
}

int read_http_request(int fd, char *resource_name) {
    int ret_val = -1;

    char line_buf[BUFSIZE] = { 0 };
    // Read a line from fd
    if (get_line(fd, line_buf, BUFSIZE) < 0) {
        return -1;
    }
    while (*line_buf) {
        // Break if it's the empty line
        if (strcmp(line_buf, "\r\n") == 0) { break; }

        // Grab second token from request line if it's a GET request
        if (strncmp(line_buf, "GET", 3) == 0) {
            int resource_name_len = strcspn(line_buf + 5, " \0");
            strncpy(resource_name, line_buf + 5, resource_name_len);
            ret_val = 0;
        }

        // Reset the line buffer
        memset(line_buf, 0, BUFSIZE);
        // Read the next line
        if (get_line(fd, line_buf, BUFSIZE) < 0) {
            return -1;
        }
    }

    return ret_val;
}

int write_http_response(int fd, const char *resource_path) {
    struct stat statinfo;
    if (stat(resource_path, &statinfo) < 0) {
        printf("%s\n", resource_path);
        printf("404\n");
        if (write(fd, not_found_string, strlen(not_found_string) + 1) < 0) {
            perror("write");
            return -1;
        }
        return 0;
    }
 
    // Open content file
    int resource_fd = open(resource_path, O_RDONLY);
    if (resource_fd < 0) {
        perror("open");
        return -1;
    }

    char buf[BUFSIZE] = { 0 };

    // Write the top of the HTTP response
    const char* mime_type = get_mime_type(strrchr(resource_path, '.'));
    snprintf(buf, BUFSIZE, ok_string, mime_type, statinfo.st_size);
    if (write(fd, buf, strlen(buf)) < 0) {
        perror("write");
        close(resource_fd);
        return -1;
    }

    memset(buf, 0, BUFSIZE);

    // Write the body of the HTTP response
    int bytes_read = -1;
    while (bytes_read) {
        bytes_read = read(resource_fd, buf, BUFSIZE);
        if (bytes_read < 0) {
            perror("read");
            close(resource_fd);
            return -1;
        }
        else if (write(fd, buf, bytes_read) < 0) {
            perror("write");
            close(resource_fd);
            return -1;
        }
    }

    if (close(resource_fd) < 0) {
        perror("close");
        return -1;
    }
    return 0;
}
