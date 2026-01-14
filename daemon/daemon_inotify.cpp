#include "hopper/daemon/daemon.hpp"
#include "hopper/daemon/util.hpp"

#include <iostream>
#include <limits.h>
#include <unistd.h>

namespace hopper {

void HopperDaemon::setup_inotify() {
    if ((m_inotify_fd = inotify_init()) < 0)
        throw_errno("inotify_init");

    struct epoll_event inotify_ev = {};
    inotify_ev.events = EPOLLIN;
    inotify_ev.data.u64 = 0;

    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_inotify_fd, &inotify_ev) != 0)
        throw_errno("epoll_ctl ADD");

    if ((m_inotify_root_watch = inotify_add_watch(
             m_inotify_fd, m_path.c_str(),
             IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_ISDIR)) < 0)
        throw_errno("inotify_add_watch");
}

void HopperDaemon::handle_root_inotify(struct inotify_event *ev) {
    if (ev->mask & IN_DELETE_SELF) {
        std::cerr << "Hopper " << m_path << " got deleted, exiting... :("
                  << std::endl;
        _exit(1);
    }

    if (ev->mask & IN_CREATE) {
        std::filesystem::path p = m_path;
        p /= ev->name;

        if (create_endpoint(p) == 0)
            std::cerr << "Endpoint creation failed! Out of IDs?" << std::endl;
    }

    if (ev->mask & IN_DELETE) {
        std::filesystem::path p = m_path;
        p /= ev->name;

        delete_endpoint(p);
    }
}

void HopperDaemon::handle_endpoint_inotify(struct inotify_event *ev,
                                           HopperEndpoint *endpoint) {
    if (ev->mask & IN_CREATE) {
        std::filesystem::path p = endpoint->path();
        p /= ev->name;

        PipeType pipe_type = detect_pipe_type(p);
        if (pipe_type == PipeType::NONE)
            return;

        HopperPipe *pipe =
            (pipe_type == PipeType::IN ? endpoint->add_input_pipe(p)
                                       : endpoint->add_output_pipe(p));

        if (pipe != nullptr)
            add_pipe(pipe);
    }

    if (ev->mask & IN_DELETE) {
        std::filesystem::path p = endpoint->path();
        p /= ev->name;

        HopperPipe *pipe = endpoint->pipe_by_path(p);
        if (pipe != nullptr && pipe->status() == PipeStatus::ACTIVE)
            remove_pipe(endpoint, pipe->id());

        endpoint->remove_by_id(pipe->id());
    }
}

void HopperDaemon::handle_inotify() {
    struct inotify_event *iev = reinterpret_cast<struct inotify_event *>(
        std::malloc(sizeof(struct inotify_event) + NAME_MAX + 1));

    if (read(m_inotify_fd, iev, sizeof(struct inotify_event) + NAME_MAX + 1) <=
        0)
        return;

    if (iev->wd == m_inotify_root_watch) {
        handle_root_inotify(iev);
        std::free(iev);
        return;
    }

    HopperEndpoint *endpoint = endpoint_by_watch(iev->wd);
    if (endpoint == nullptr) {
        std::cout << "No endpoint found for watch ID " << iev->wd << std::endl;
        std::free(iev);
        return;
    }

    handle_endpoint_inotify(iev, endpoint);

    // The endpoint is now closed
    if (iev->mask & IN_IGNORED)
        delete_endpoint(endpoint->id());

    std::free(iev);
}

}; // namespace hopper
