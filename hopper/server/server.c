#define _GNU_SOURCE

#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_COPY_SIZE 64 * 1024

#define MAX_PIPES 256

#define PIPE_SRC 1
#define PIPE_DST 2

#define HANDLER_UNKNOWN 0

/// A structure for holding pipe information
struct PipeInfo {
    short type;
    short handler;
    char *id;
};

/// A structure for holding I/O pipe file descriptors
struct PipeSet {
    int buf[2];
    int fd;
    struct PipeInfo *info;
};

/// Maps a handler string to an ID number
short map_handler_to_id(char *handler) { return HANDLER_UNKNOWN; }

/// Close all file descriptors in a PipeSet object
void close_pipe_set(struct PipeSet *set) {
    close(set->buf[0]);
    close(set->buf[1]);
    close(set->fd);

    set->buf[0] = -1;
    set->buf[1] = -1;
    set->fd = -1;
}

/// Free a PipeSet, file descriptors should be closed first
void free_pipe_set(struct PipeSet **set) {
    if (!set)
        return;

    struct PipeSet *_set = *set;
    free(_set->info->id);
    free(_set->info);
    free(_set);

    // Set the pointer to NULL so it isn't reused
    (*set) = NULL;
}

/// Generate a PipeInfo object from a file path
struct PipeInfo *get_pipe_info(const char *path) {
    char *filename = basename((char *)path);

    struct PipeInfo *info = (struct PipeInfo *)malloc(sizeof(struct PipeInfo));
    if (!info) {
        perror("malloc");
        return NULL;
    }

    char *type = strtok(filename, "_");
    if (!type)
        goto bad_filename;

    switch (*type) {
        case 'I':
            info->type = PIPE_SRC;
            break;
        case 'O':
            info->type = PIPE_DST;
            break;
        default:
            goto bad_filename;
    }

    char *handler = strtok(NULL, "_");
    if (!handler)
        goto bad_filename;

    info->handler = map_handler_to_id(handler);

    char *id = strtok(NULL, "_");
    if (!id)
        goto bad_filename;

    int len = strlen(id) + 1;

    info->id = (char *)malloc(sizeof(char) * len);
    if (!info->id) {
        free(info);
        perror("malloc");
        return NULL;
    }

    strcpy(info->id, id);

    return info;

bad_filename:
    printf("Badly formatted filename: '%s'\n", filename);
    free(info);
    return NULL;
}

/// Open a PipeSet object from a file
struct PipeSet *open_pipe_set(const char *path) {
    char *path_copy = strdup(path);

    struct PipeSet *set = (struct PipeSet *)malloc(sizeof(struct PipeSet));
    if (!set) {
        perror("malloc");
        return NULL;
    }

    struct PipeInfo *info = get_pipe_info(path_copy);
    if (!info)
        return NULL;

    set->info = info;

    if (pipe(set->buf) < 0) {
        free_pipe_set(&set);
        perror("pipe");
        return NULL;
    }

    // Use O_RDWR here to avoid failing to open write FIFOs if no process is
    // reading, don't use O_WRONLY
    if ((set->fd = open(path, (info->type == PIPE_SRC ? O_RDONLY : O_RDWR) |
                                  O_NONBLOCK)) < 0) {
        close_pipe_set(set);
        free_pipe_set(&set);
        perror("open");
        return NULL;
    }

    // Remove the non-blocking flag, as this server requires blocking I/O
    int flags = fcntl(set->fd, F_GETFL);
    if (flags >= 0) {
        fcntl(set->fd, F_SETFL, flags & ~O_NONBLOCK);
    }

    return set;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <pipe> ...\n", argv[0]);
        return 1;
    }

    struct PipeSet *pipes[MAX_PIPES];
    int n_pipes = 0;

    for (int i = 1; i < argc && i < MAX_PIPES; i++) {
        struct PipeSet *set = open_pipe_set(argv[i]);
        if (!set)
            goto close_pipes;

        pipes[i - 1] = set;

        printf("%d:\n", i);
        printf("\tType: %d\n", set->info->type);
        printf("\tHandler: %d\n", set->info->handler);
        printf("\tID: %s\n", set->info->id);

        n_pipes++;
    }

    /*
    int devnull = open("/dev/null", O_WRONLY);

    ssize_t bytes_copied = 0;

    while ((bytes_copied = splice(src->fd, NULL, src->buf[1], NULL,
                                  MAX_COPY_SIZE, 0)) > 0) {
        tee(src->buf[0], dst->buf[1], MAX_COPY_SIZE, 0);
        splice(dst->buf[0], NULL, dst->fd, NULL, MAX_COPY_SIZE, 0);

        splice(src->buf[0], NULL, devnull, NULL, MAX_COPY_SIZE, 0);

        printf("Copied %zd bytes\n", bytes_copied);
    }


    close (devnull);
    */

close_pipes:

    for (int i = 0; i < n_pipes && i < MAX_PIPES; i++) {
        if (pipes[i]) {
            close_pipe_set(pipes[i]);
            free_pipe_set(&pipes[i]);
        }
    }

    return 0;
}
