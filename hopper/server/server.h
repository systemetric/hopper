#ifndef server_h_INCLUDED
#define server_h_INCLUDED

#define MAX_EVENTS 64

struct HopperData {
    struct PipeSet *pipes;
    struct PipeSet **outputs;
    int n_pipes;
    int epoll_fd;
    int inotify_fd;
    int devnull;
    const char *pipe_dir;
};

#endif // server_h_INCLUDED
