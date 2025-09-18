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

void pipe_set_status_inactive(struct PipeSet *set, struct HopperData *data);
void pipe_set_status_active(struct PipeSet *set, struct HopperData *data);

#endif // server_h_INCLUDED
