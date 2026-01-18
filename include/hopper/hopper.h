#ifndef hopper_h_INCLUDED
#define hopper_h_INCLUDED

#include <unistd.h>

#define HOPPER_IN 1
#define HOPPER_OUT 2
#define HOPPER_NONBLOCK 4

/// Structure representing a Hopper pipe
struct hopper_pipe {
    const char *name;
    const char *endpoint;
    const char *hopper;
    int fd;
    int flags;
};

/// Open a new Hopper pipe specified by `pipe`. -1 is returned on error, and
/// errno is set.
int hopper_open(struct hopper_pipe *pipe);

/// Close a Hopper pipe previously opened by `hopper_open_pipe`.
int hopper_close(struct hopper_pipe *pipe);

/// Read up to `len` bytes from a Hopper pipe. Value returned indicates
/// the number of bytes read. -1 is returned on error and errno is set.
ssize_t hopper_read(struct hopper_pipe *pipe, void *dst, size_t len);

/// Write up to `len` bytes into a Hopper pipe. Value returned indicates
/// the number of bytes written. -1 is returned on error and errno is set.
ssize_t hopper_write(struct hopper_pipe *pipe, void *src, size_t len);

#endif // hopper_h_INCLUDED

