#define _GNU_SOURCE

#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "handler.h"
#include "pipe.h"

#define MAX_PIPES 256

struct HopperData {
    struct PipeSet *pipes;
    struct PipeSet **outputs;
    int n_pipes;
    int epoll_fd;
    int inotify_fd;
    const char *pipe_dir;
};

void free_pipe_list(struct PipeSet *head) {
    if (head) {
        free_pipe_list(head->next);
        head->next = NULL;
        free(head);
    }
}

void prepend_pipe_list(struct PipeSet **head, struct PipeSet *set) {
    set->next = *head;
    *head = set;
}

/// Safely free a HopperData structure
void free_hopper_data(struct HopperData *data) {
    if (!data)
        return;

    free_pipe_list(data->pipes);

    if (data->outputs)
        for (int i = 0; i < N_HANDLERS; i++)
            free_pipe_list(data->outputs[i]);
}

/// Allocate a new HopperData structure
struct HopperData *alloc_hopper_data() {
    struct HopperData *data =
        (struct HopperData *)malloc(sizeof(struct HopperData));
    if (!data)
        goto err_malloc;

    data->outputs =
        (struct PipeSet **)malloc(sizeof(struct PipeSet *) * N_HANDLERS);
    if (!data->outputs)
        goto err_malloc;

    return data;

err_malloc:
    perror("malloc");
    free_hopper_data(data);
    return NULL;
}

int load_pipes_directory(struct HopperData *data) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(data->pipe_dir);
    if (!dir) {
        perror("opendir");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_FIFO && entry->d_type != DT_UNKNOWN)
            continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", data->pipe_dir, entry->d_name);

        struct PipeSet *set = open_pipe_set(path);
        if (!set)
            continue;

        prepend_pipe_list(&data->pipes, set);

        printf("added fifo '%s'\n", path);
    }

    closedir(dir);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <pipes directory>\n", argv[0]);
        return 1;
    }

    int ret = 0;

    struct HopperData *data = alloc_hopper_data();
    if (!data) {
        ret = 1;
        goto cleanup;
    }

    data->pipe_dir = argv[1];

    if ((data->epoll_fd = epoll_create1(0)) < 0) {
        perror("epoll_create");
        ret = 1;
        goto cleanup;
    }

    if (load_pipes_directory(data) != 0) {
        ret = 1;
        goto cleanup;
    }

cleanup:
    free_hopper_data(data);
    return ret;
}
