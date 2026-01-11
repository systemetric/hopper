#include <fcntl.h>
#include <unistd.h>

#include "hopper/daemon/pipe.hpp"

namespace hopper {

/* HopperPipe */

HopperPipe::HopperPipe(std::string name, int handler, PipeType type,
                       std::filesystem::path path, BufferMarker *marker)
    : m_marker(marker), m_name(name), m_handler(handler), m_type(type),
      m_path(path) {
    if (this->open_pipe())
        m_status = PipeStatus::ACTIVE;
    else
        m_status = PipeStatus::INACTIVE;
}

HopperPipe::~HopperPipe() {
    if (m_fd != -1)
        close(m_fd);
}

int HopperPipe::open_pipe() {
    if (m_status == PipeStatus::ACTIVE)
        return 1;

    int fd = open(m_path.c_str(),
                  (m_type == PipeType::IN ? O_RDONLY : O_WRONLY) | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        return 0;
    }

    m_fd = fd;

    return 1;
}

size_t HopperPipe::write_pipe(void *src, size_t len) {
    if (m_type == PipeType::IN)
        return -1;

    if (len == 0)
        return 0;

    size_t done_len = 0;

    while (done_len < len) {
        ssize_t res = write(m_fd, reinterpret_cast<char *>(src) + done_len,
                            len - done_len);

        if (res == -1 && (errno == EAGAIN || errno == EINTR))
            return done_len;
        else if (res == -1) {
            perror("write");
            return -1;
        }

        done_len += res;
    }

    return done_len;
}

size_t HopperPipe::read_pipe(void *dst, size_t len) {
    if (m_type == PipeType::OUT)
        return -1;

    if (len == 0)
        return 0;

    size_t done_len = 0;

    while (done_len < len) {
        ssize_t res = read(m_fd, reinterpret_cast<char *>(dst) + done_len,
                           len - done_len);

        if (res == -1 && (errno == EAGAIN || errno == EINTR))
            return done_len;
        else if (res == -1) {
            perror("read");
            return -1;
        } else if (res == 0)
            break;

        done_len += res;
    }

    return done_len;
}
}; // namespace hopper
