#define _GNU_SOURCE

#include <fcntl.h>
#include <libgen.h>
#include <pthread.h>
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
    pthread_t thread;
    struct PipeInfo *info;
};

/// A structure to hold all server variables
struct HopperData {
    struct PipeSet *pipes[MAX_PIPES];
    int n_pipes;
    int devnull;
};

/// A structure for holding all thread-specific data
struct ThreadData {
    struct HopperData *global;
    struct PipeSet *set;
};

/// Maps a handler string to an ID number
short map_handler_to_id(char *handler) { return HANDLER_UNKNOWN; }

/// Join all threads associated with a pipe set
void join_pipe_set(struct PipeSet *set) {
    if (set->thread > 0) {
        void *res;
        pthread_join(set->thread, &res);
        set->thread = 0;
    }
}

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
    set->thread = 0;

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

/// Thread function to copy data from source to destination FIFOs
static void *input_thread_start(void *arg) {
    struct ThreadData *data = (struct ThreadData *)arg;
    printf("Created new thread for pipe %d/%s\n", data->set->info->handler,
           data->set->info->id);

    struct PipeSet *outputs[MAX_PIPES];
    int n_outputs = 0;

    for (int i = 0; i < data->global->n_pipes && i < MAX_PIPES; i++) {
        struct PipeSet *output = data->global->pipes[i];

        if (output->info->type != PIPE_DST)
            continue;

        if (output->info->handler != data->set->info->handler)
            continue;

        // Ensure the IDs are different to avoid echoing
        if (!strcmp(output->info->id, data->set->info->id))
            continue;

        outputs[n_outputs] = output;
        n_outputs++;
    }

    ssize_t bytes_read = 0;

    while ((bytes_read = splice(data->set->fd, NULL, data->set->buf[1], NULL,
                                MAX_COPY_SIZE, 0)) > 0) {
        for (int i = 0; i < n_outputs; i++) {
            ssize_t remaining = bytes_read, done;
            while (remaining > 0) {
                done =
                    tee(data->set->buf[0], outputs[i]->buf[1], bytes_read, 0);
                if (done <= 0)
                    break;

                remaining -= done;
            }
        }

        for (int i = 0; i < n_outputs; i++) {
            ssize_t remaining = bytes_read, done;
            while (remaining > 0) {
                done = splice(outputs[i]->buf[0], NULL, outputs[i]->fd, NULL,
                              bytes_read, 0);
                if (done <= 0)
                    break;

                remaining -= done;
            }
        }

        splice(data->set->buf[0], NULL, data->global->devnull, NULL, bytes_read,
               0);

        printf("%d/%s: copied %zd bytes\n", data->set->info->handler,
               data->set->info->id, bytes_read);
    }

    // Passed arguments are owned by this thread, and not freed anywhere else
    free(arg);

    return NULL;
}

int spawn_pipe_thread(struct PipeSet *set, struct HopperData *data) {
    struct ThreadData *tdata =
        (struct ThreadData *)malloc(sizeof(struct ThreadData));
    if (!tdata) {
        perror("malloc");
        return 1;
    }

    tdata->global = data;
    tdata->set = set;

    if (pthread_create(&tdata->set->thread, NULL, &input_thread_start, tdata) !=
        0) {
        perror("pthread_create");
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <pipe> ...\n", argv[0]);
        return 1;
    }

    struct HopperData data = {0};

    for (int i = 1; i < argc && i < MAX_PIPES; i++) {
        struct PipeSet *set = open_pipe_set(argv[i]);
        if (!set)
            goto cleanup_pipes;

        data.pipes[i - 1] = set;

        printf("%d:\n", i - 1);
        printf("\tType: %d\n", set->info->type);
        printf("\tHandler: %d\n", set->info->handler);
        printf("\tID: %s\n", set->info->id);

        data.n_pipes++;
    }

    data.devnull = open("/dev/null", O_WRONLY);
    if (!data.devnull) {
        perror("open");
        goto cleanup_pipes;
    }

    for (int i = 0; i < data.n_pipes; i++)
        if (data.pipes[i]->info->type == PIPE_SRC)
            if (!spawn_pipe_thread(data.pipes[i], &data))
                goto cleanup_pipes;

    while (1) {
        sleep(1);
    }

cleanup_pipes:

    for (int i = 0; i < data.n_pipes && i < MAX_PIPES; i++) {
        if (data.pipes[i]) {
            join_pipe_set(data.pipes[i]);
            close_pipe_set(data.pipes[i]);
            free_pipe_set(&data.pipes[i]);
        }
    }

    close(data.devnull);

    return 0;
}
