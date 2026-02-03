#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hopper/hopper.h"

static int get_open_flags(int flags) {
    int open_flags = 0;

    if (flags & HOPPER_IN)
        open_flags |= O_WRONLY;
    if (flags & HOPPER_OUT)
        open_flags |= O_RDONLY;
    if (flags & HOPPER_NONBLOCK)
        open_flags |= O_NONBLOCK;

    return open_flags;
}

static char *get_endpoint_path(struct hopper_pipe *pipe) {
    char *path = (char *)malloc(sizeof(char) * PATH_MAX);
    sprintf(path, "%s/%s", pipe->hopper, pipe->endpoint);
    return path;
}

static char *get_pipe_path(struct hopper_pipe *pipe) {
    char *path = (char *)malloc(sizeof(char) * PATH_MAX);
    const char *suffix = (pipe->flags & HOPPER_IN ? "in" : "out");

    // {HOPPER}/{ENDPOINT}/{NAME}.{TYPE}
    sprintf(path, "%s/%s/%s.%s", pipe->hopper, pipe->endpoint, pipe->name,
            suffix);

    return path;
}

int hopper_open(struct hopper_pipe *pipe) {
    int res = 0;

    if (pipe->name == NULL || pipe->endpoint == NULL || pipe->hopper == NULL) {
        // pipe, endpoint name, hopper path are (obviously) required
        errno = EINVAL;
        return -1;
    }

    if (!(pipe->flags & HOPPER_IN) == !(pipe->flags & HOPPER_OUT)) {
        // either both or none of the input/output flags are set
        errno = EINVAL;
        return -1;
    }

    char *endpoint_path = get_endpoint_path(pipe);
    res = mkdir(endpoint_path, 0755);
    free(endpoint_path);
    if (res == -1 && errno != EEXIST) {
        pipe->fd = -1;
        return -1;
    }

    char *pipe_path = get_pipe_path(pipe);
    if (mkfifo(pipe_path, 0660) < 0 && errno != EEXIST) {
        // mkfifo failed in some way, preserve errno
        res = -1;
        goto cleanup;
    }

    int open_flags = get_open_flags(pipe->flags);
    int fd = open(pipe_path, open_flags);
    if (fd < 0) {
        // preserve errno if open fails
        res = -1;
        goto cleanup;
    }

    // acquire a file lock on the fifo, we don't want other things using it
    if (flock(fd, (pipe->flags & HOPPER_IN ? LOCK_EX : LOCK_SH) | LOCK_NB) !=
        0) {
        if (errno == EWOULDBLOCK)
            errno = EBUSY; // this makes more sense for clients

        int errsv = errno; // preserve errno across close
        close(fd);
        errno = errsv;

        res = -1;
        goto cleanup;
    }

    pipe->fd = fd;

cleanup:
    free(pipe_path);
    if (res != 0)
        pipe->fd = -1;

    return res;
}

int hopper_close(struct hopper_pipe *pipe) {
    if (pipe->fd == -1)
        return 0;

    if (flock(pipe->fd, LOCK_UN) != 0)
        return -1;

    if (close(pipe->fd) != 0)
        return -1;

    pipe->fd = -1;

    return 0;
}

ssize_t hopper_read(struct hopper_pipe *pipe, void *dst, size_t len) {
    ssize_t res = read(pipe->fd, dst, len);
    if (res < 0 && errno == EWOULDBLOCK)
        return 0; // EWOULDBLOCK isn't an error for non-block pipes
    return res;
}

ssize_t hopper_write(struct hopper_pipe *pipe, void *src, size_t len) {
    ssize_t res = write(pipe->fd, src, len);
    if (res < 0 && errno == EWOULDBLOCK)
        return 0; // EWOULDBLOCK isn't an error for non-block pipes
    return res;
}

