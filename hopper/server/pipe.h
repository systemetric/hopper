#ifndef pipe_h_INCLUDED
#define pipe_h_INCLUDED

#define PIPE_SRC 1
#define PIPE_DST 2

#define PIPE_INACTIVE 0
#define PIPE_ACTIVE 1

/// A structure containing file information about a I/O pipe
struct PipeInfo {
    short type;
    short handler;
    char *id;
    char *name;
    char *path;
};

/// A structure for holding I/O pipe data
struct PipeSet {
    int buf[2];
    int fd;
    short status;
    struct PipeInfo *info;
    struct PipeSet *next;
};

void close_pipe_set(struct PipeSet *set);
void free_pipe_set(struct PipeSet **set);
struct PipeInfo *get_pipe_info(const char *path);
struct PipeSet *open_pipe_set(const char *path);
int reopen_pipe_set(struct PipeSet *set);

#endif // pipe_h_INCLUDED
