#include "hopper/daemon/daemon.hpp"
#include "hopper/daemon/util.hpp"
#include <iostream>

namespace hopper {

void HopperDaemon::remove_pipe(HopperEndpoint *endpoint, uint64_t pipe_id) {
    PipeType type = (pipe_id & 0x1 ? PipeType::IN : PipeType::OUT);
    for (const auto &[id, pipe] :
         (type == PipeType::IN ? endpoint->inputs() : endpoint->outputs())) {

        if (pipe->id() != pipe_id)
            continue;

        if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, pipe->fd(), nullptr) != 0)
            throw_errno("epoll_ctl DEL");
        pipe->close_pipe();

        std::cout << "DOWN " << pipe->name() << "(" << endpoint->name()
                  << ")\n";
    }
}

void HopperDaemon::add_pipe(HopperEndpoint *endpoint, HopperPipe *pipe) {
    if (pipe == nullptr)
        return;

    // Pipe has bad ID or bad FD
    if (pipe->id() == 0 || pipe->fd() == -1)
        return;

    struct epoll_event ev = {};
    ev.events = (pipe->type() == PipeType::IN ? EPOLLIN | EPOLLHUP | EPOLLET
                                              : EPOLLHUP);
    ev.data.u64 = pipe->id();

    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, pipe->fd(), &ev) != 0)
        throw_errno("epoll_ctl ADD");

    std::cout << "UP " << pipe->name() << "(" << endpoint->name() << ")\n";
}

void HopperDaemon::refresh_pipes() {
    // Try to open any inactive pipes again
    for (const auto &[_, endpoint] : m_endpoints) {
        for (const auto &[id, pipe] : endpoint->inputs()) {
            if (pipe->status() == PipeStatus::ACTIVE || !pipe->open_pipe())
                continue;
            add_pipe(endpoint, pipe);
        }
        for (const auto &[id, pipe] : endpoint->outputs()) {
            if (pipe->status() == PipeStatus::ACTIVE || !pipe->open_pipe())
                continue;
            add_pipe(endpoint, pipe);
        }

        // Try to empty buffers into pipes
        endpoint->flush_pipes();
    }
}

}; // namespace hopper
