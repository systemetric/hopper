#include <cstdint>
#include <iostream>
#include <limits.h>

#include <csignal>
#include <filesystem>
#include <unistd.h>

#include "hopper/daemon/daemon.hpp"
#include "hopper/daemon/endpoint.hpp"
#include "hopper/daemon/pipe.hpp"
#include "hopper/daemon/util.hpp"

namespace hopper {

HopperDaemon::HopperDaemon(std::filesystem::path path, int max_events,
                           int timeout)
    : m_max_events(max_events), m_timeout(timeout), m_path(path) {
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }

    // just ignore SIGPIPEs
    std::signal(SIGPIPE, SIG_IGN);

    if ((m_epoll_fd = epoll_create1(0)) < 0)
        throw_errno("epoll_create1");

    setup_inotify();
}

HopperDaemon::~HopperDaemon() {
    for (const auto &[_, endpoint] : m_endpoints)
        delete endpoint;
}

void HopperDaemon::try_add_pipe(std::pair<uint64_t, int> pipe, PipeType type) {
    // Pipe has bad ID or bad FD
    if (pipe.first == 0 || pipe.second == -1)
        return;

    struct epoll_event ev = {};
    ev.events = (type == PipeType::IN ? EPOLLIN | EPOLLHUP : EPOLLHUP);
    ev.data.u64 = pipe.first;

    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, pipe.second, &ev) != 0)
        throw_errno("epoll_ctl ADD");
}

void HopperDaemon::refresh_pipes() {
    // Try to open any inactive pipes again
    for (const auto &[_, endpoint] : m_endpoints) {
        for (const auto &[id, pipe] : endpoint->outputs()) {
            if (pipe->status() == PipeStatus::ACTIVE || !pipe->open_pipe())
                continue;
            try_add_pipe(std::make_pair(id, pipe->fd()), PipeType::OUT);
        }
    }
}

void HopperDaemon::process_events(struct epoll_event *events, int n_events) {
    for (int i = 0; i < n_events; i++) {
        struct epoll_event ev = events[i];

        // inotify events use 0 as ID
        if (ev.data.u64 == 0) {
            handle_inotify();
            continue;
        }

        uint32_t endpoint_id = (ev.data.u64 >> 40) & 0xFFFFFFFFFF;
        if (!m_endpoints.contains(endpoint_id))
            continue;

        HopperEndpoint *endpoint = m_endpoints[endpoint_id];

        if (ev.events & EPOLLIN)
            endpoint->on_pipe_readable(ev.data.u64);
        else if (ev.events & EPOLLOUT)
            endpoint->on_pipe_writable(ev.data.u64);
    }
}

int HopperDaemon::run() {
    int res = 0;

    std::cout << "Started event loop, max " << m_max_events
              << " events, timeout " << m_timeout << " ms" << std::endl;

    while (res == 0) {
        struct epoll_event *events = new struct epoll_event[m_max_events];

        int n = epoll_wait(m_epoll_fd, events, m_max_events, m_timeout);
        if (n < 0) {
            if (errno == EINTR)
                continue;

            delete[] events;
            throw_errno("epoll_wait");
            return -1;
        }

        process_events(events, n);
        refresh_pipes();
    }

    return res;
}

}; // namespace hopper
