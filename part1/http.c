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

int get_line(int fd, char* line_buf, int n) {
    for (int i = 0; i < n; i++) {
        if (read(fd, line_buf + i, 1) != 1) { break; }

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
    get_line(fd, line_buf, BUFSIZE);
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
        get_line(fd, line_buf, BUFSIZE);
    }

    return ret_val;
}

int write_http_response(int fd, const char *resource_path) {
    // TODO Not yet implemented
    return 0;
}
