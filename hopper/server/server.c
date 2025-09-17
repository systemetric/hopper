#define _GNU_SOURCE

#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "handler.h"
#include "pipe.h"
#include "server.h"

#define MAX_COPY_SIZE 64 * 1024

void free_pipe_list(struct PipeSet *head) {
    struct PipeSet *set;
    while (head) {
        set = head->next;
        free(head);
        head = set;
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
        free(data->outputs);
}

void close_hopper_fds(struct HopperData *data) {
    if (!data)
        return;

    close(data->epoll_fd);
    close(data->inotify_fd);
    close(data->devnull);

    struct PipeSet *set = data->pipes;

    do {
        close(set->fd);
        close(set->buf[0]);
        close(set->buf[1]);
        set = set->next;
    } while (set);
}

/// Allocate a new HopperData structure
struct HopperData *alloc_hopper_data() {
    struct HopperData *data =
        (struct HopperData *)malloc(sizeof(struct HopperData));
    if (!data)
        goto err_malloc;

    data->outputs =
        (struct PipeSet **)calloc(N_HANDLERS, sizeof(struct PipeSet *));
    if (!data->outputs)
        goto err_malloc;

    return data;

err_malloc:
    perror("alloc");
    free_hopper_data(data);
    return NULL;
}

int load_new_pipe(struct HopperData *data, struct PipeSet *set) {
    prepend_pipe_list(&data->pipes, set);

    if (set->info->type == PIPE_DST)
        prepend_pipe_list(&data->outputs[set->info->handler], set);
    else if (set->info->type == PIPE_SRC) {
        struct epoll_event ev = {};
        ev.events = EPOLLIN | EPOLLHUP;
        ev.data.ptr = (void *)set;

        int res;
        if ((res = epoll_ctl(data->epoll_fd, EPOLL_CTL_ADD, set->fd, &ev)) !=
            0) {
            perror("epoll_ctl ADD");
            return res;
        }
    }

    return 0;
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

        if (load_new_pipe(data, set) < 0)
            continue;

        printf("added fifo '%s'\n", path);
    }

    closedir(dir);

    return 0;
}

int run_epoll_cycle(struct HopperData *data) {
    struct epoll_event events[MAX_EVENTS];
    int res;

    int n = epoll_wait(data->epoll_fd, events, MAX_EVENTS, -1);
    if (n < 0) {
        perror("epoll_wait");
        return n;
    }

    for (int i = 0; i < n; i++) {
        struct PipeSet *set = (struct PipeSet *)events[i].data.ptr;

        if (events[i].events & EPOLLIN)
            if ((res = transfer_buffers(data, set, MAX_COPY_SIZE)) < 0)
                return res;

        if (events[i].events & EPOLLHUP) {
            close(set->fd);
            set->fd = -1;
            set->status = PIPE_INACTIVE;
            printf("'%s' set to INACTIVE\n", set->info->name);
        }
    }

    return n;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <pipes directory>\n", argv[0]);
        return 1;
    }

    int ret = 0;

    signal(SIGPIPE, SIG_IGN);

    struct HopperData *data = alloc_hopper_data();
    if (!data) {
        ret = 1;
        goto cleanup;
    }

    data->pipe_dir = argv[1];

    if ((data->devnull = open("/dev/null", O_WRONLY)) < 0) {
        perror("open");
        ret = 1;
        goto cleanup;
    }

    if ((data->epoll_fd = epoll_create1(0)) < 0) {
        perror("epoll_create");
        ret = 1;
        goto cleanup;
    }

    if (load_pipes_directory(data) != 0) {
        ret = 1;
        goto cleanup;
    }

    while (run_epoll_cycle(data)) {
    }

cleanup:
    close_hopper_fds(data);
    free_hopper_data(data);
    return ret;
}
