#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "handler.h"
#include "pipe.h"

/// Close all file descriptors in a PipeSet object
/// Threads should be joined first
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
    struct PipeInfo *info = (struct PipeInfo *)malloc(sizeof(struct PipeInfo));
    if (!info) {
        perror("malloc");
        return NULL;
    }

    info->path = strdup(path);

    char *filename = basename((char *)path);

    info->name = strdup(filename);

    char *type = strtok(filename, "_");
    if (!type)
        goto err_bad_fname;

    switch (*type) {
        case 'I':
            info->type = PIPE_SRC;
            break;
        case 'O':
            info->type = PIPE_DST;
            break;
        default:
            goto err_bad_fname;
    }

    char *handler = strtok(NULL, "_");
    if (!handler)
        goto err_bad_fname;

    info->handler = map_handler_to_id(handler);

    char *id = strtok(NULL, "_");
    if (!id)
        goto err_bad_fname;

    int len = strlen(id) + 1;

    info->id = (char *)malloc(sizeof(char) * len);
    if (!info->id) {
        free(info);
        perror("malloc");
        return NULL;
    }

    strcpy(info->id, id);

    return info;

err_bad_fname:
    printf("Badly formatted filename: '%s'\n", filename);
    free(info);
    return NULL;
}

/// Try to reopen a previously closed pipe
int reopen_pipe_set(struct PipeSet *set) {
    if (set->status == PIPE_ACTIVE)
        return 0;

    set->status = PIPE_ACTIVE;

    if ((set->fd = open(set->info->path,
                        (set->info->type == PIPE_SRC ? O_RDONLY : O_WRONLY) |
                            O_NONBLOCK)) < 0) {
        if (errno == ENXIO && set->info->type == PIPE_DST) {
            set->status = PIPE_INACTIVE;
            return 1;
        }

        perror("open");
        return 1;
    }
    return 0;
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
    set->status = PIPE_INACTIVE;
    set->next = NULL;

    if (pipe(set->buf) < 0) {
        free_pipe_set(&set);
        perror("pipe");
        return NULL;
    }

    if ((set->fd = open(path, (info->type == PIPE_SRC ? O_RDONLY : O_WRONLY) |
                                  O_NONBLOCK)) < 0) {
        if (errno == ENXIO && info->type == PIPE_DST) {
            set->status = PIPE_INACTIVE;
            return set;
        }

        close_pipe_set(set);
        free_pipe_set(&set);
        perror("open");
        return NULL;
    }

    return set;
}
