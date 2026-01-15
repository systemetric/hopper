#include <cstdint>
#include <iostream>
#include <limits.h>

#include <csignal>
#include <filesystem>
#include <unistd.h>

#include "hopper/daemon/daemon.hpp"
#include "hopper/daemon/endpoint.hpp"
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
        if (ev.events & EPOLLHUP || ev.events & EPOLLERR)
            remove_pipe(endpoint, ev.data.u64);
    }
}

int HopperDaemon::run() {
    int res = 0;

    std::unique_ptr<struct epoll_event[]> events(
        new struct epoll_event[m_max_events]);

    while (res == 0) {

        int n = epoll_wait(m_epoll_fd, events.get(), m_max_events, m_timeout);
        if (n < 0) {
            if (errno == EINTR)
                continue;

            throw_errno("epoll_wait");
            return -1;
        }

        process_events(events.get(), n);
        refresh_pipes();
    }

    return res;
}

}; // namespace hopper
