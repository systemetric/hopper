#include <fcntl.h>
#include <unistd.h>

#include "hopper/daemon/pipe.hpp"
#include "hopper/daemon/util.hpp"

namespace hopper {

/* HopperPipe */

HopperPipe::HopperPipe(uint64_t id, const std::string &endpoint_name,
                       PipeType type, std::filesystem::path path,
                       BufferMarker *marker)
    : m_marker(marker), m_type(type), m_path(path),
      m_endpoint_name(endpoint_name), m_id(id) {
    m_name = path.replace_extension("").filename();
    open_pipe();
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
        if (m_type == PipeType::OUT && errno == ENXIO) {
            // No readers available
            m_status = PipeStatus::INACTIVE;
            m_fd = -1;
            return 0;
        }

        m_status = PipeStatus::INACTIVE;
        m_fd = -1;
        throw_errno("open");
        return 0;
    }

    m_fd = fd;
    m_status = PipeStatus::ACTIVE;

    return 1;
}

void HopperPipe::close_pipe() {
    if (m_status == PipeStatus::INACTIVE)
        return;

    m_status = PipeStatus::INACTIVE;

    if (m_fd != -1)
        close(m_fd);
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
