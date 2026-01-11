#include <cstdint>
#include <cstdio>
#include <iostream>
#include <limits.h>
#include <sys/epoll.h>
#include <sys/inotify.h>

#include <csignal>
#include <filesystem>
#include <unistd.h>

#include "hopper/daemon/daemon.hpp"
#include "hopper/daemon/endpoint.hpp"
#include "hopper/daemon/util.hpp"

namespace hopper {

HopperDaemon::HopperDaemon(std::filesystem::path path, int max_events,
                           int timeout)
    : m_path(path), m_max_events(max_events), m_timeout(timeout) {
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }

    // just ignore SIGPIPEs
    std::signal(SIGPIPE, SIG_IGN);

    if ((m_epoll_fd = epoll_create1(0)) < 0)
        throw_errno("epoll_create1");

    if ((m_inotify_fd = inotify_init()) < 0)
        throw_errno("inotify_init");

    // Set up an event for inotify
    struct HopperEvent *inotify_ev = new HopperEvent;
    inotify_ev->fd = m_inotify_fd;
    inotify_ev->data.u64 = 0;

    // C++ lambda syntax is really weird
    inotify_ev->callback = [this](HopperEvent *ev) {
        return this->handle_inotify(ev);
    };

    if (add_event(inotify_ev) != 0)
        throw_errno("HopperDaemon::add_event");

    if ((m_inotify_watch_fd =
             inotify_add_watch(m_inotify_fd, path.c_str(),
                               IN_CREATE | IN_DELETE | IN_DELETE_SELF)) < 0)
        throw_errno("inotify_add_watch");
}

HopperDaemon::~HopperDaemon() {
    for (const auto &[_, endpoint] : m_endpoints)
        delete endpoint;

    for (auto *event : m_events)
        delete event;
}

int HopperDaemon::create_endpoint(std::filesystem::path path) {
    int endpoint_id = next_endpoint_id();
    auto *endpoint = new HopperEndpoint(endpoint_id, path);
    m_endpoints[endpoint_id] = endpoint;

    std::cout << "CREATE " << endpoint->path() << std::endl;

    return endpoint->refresh(this);
}

int HopperDaemon::delete_endpoint(int id) {
    if (m_endpoints[id] == nullptr)
        return 1;

    std::cout << "DELETE " << m_endpoints[id]->path() << std::endl;

    delete m_endpoints[id];
    m_endpoints.erase(id);

    return 0;
}

int HopperDaemon::delete_endpoint(std::filesystem::path path) {
    for (const auto &[_, endpoint] : m_endpoints) {
        if (endpoint->path() == path) {
            return delete_endpoint(endpoint->id());
        }
    }
    return 1;
}

int HopperDaemon::handle_inotify(HopperEvent *ev) {
    // I can't remember how to do this with new
    struct inotify_event *iev = reinterpret_cast<struct inotify_event *>(
        std::malloc(sizeof(struct inotify_event) + NAME_MAX + 1));

    if (read(ev->fd, iev, sizeof(struct inotify_event) + NAME_MAX + 1) < 0) {
        throw_errno("read");
        return -1;
    }

    if (iev->mask & IN_DELETE_SELF) {
        // The hopper got deleted, this is fatal
        std::cerr << "(ENOENT) Hopper " << m_path
                  << " was deleted, exiting... :(";
        _exit(1);
    }

    if (iev->mask & IN_CREATE) {
        std::string path;
        path.resize(PATH_MAX);
        std::snprintf(path.data(), PATH_MAX, "%s/%s", m_path.c_str(),
                      iev->name);

        std::free(iev);
        return create_endpoint(path);
    }

    if (iev->mask & IN_DELETE) {
        std::string path;
        path.resize(PATH_MAX);
        std::snprintf(path.data(), PATH_MAX, "%s/%s", m_path.c_str(),
                      iev->name);

        std::free(iev);
        return delete_endpoint(path);
    }

    std::free(iev);
    return 0;
}

int HopperDaemon::add_event(HopperEvent *event, int events) {
    uint64_t event_id = next_event_id();
    event->id = event_id;

    struct epoll_event ev = {};
    ev.events = events;
    ev.data.u64 = event_id;

    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, event->fd, &ev) != 0)
        return -1;

    m_events.push_back(event);

    return 0;
}

int HopperDaemon::remove_event(uint64_t id) {
    for (size_t i = 0; i < m_events.size(); i++) {
        if (m_events[i]->id == id) {
            if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_events[i]->fd,
                          nullptr) != 0)
                return -1;

            m_events.erase(m_events.begin() + i);
            break;
        }
    }

    return 0;
}

int HopperDaemon::remove_event(HopperEvent *event) {
    for (size_t i = 0; i < m_events.size(); i++) {
        if (m_events[i] == event) {
            if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_events[i]->fd,
                          nullptr) != 0)
                return -1;

            m_events.erase(m_events.begin() + i);
            break;
        }
    }

    return 0;
}

HopperEvent *HopperDaemon::get_event(uint64_t id) {
    for (size_t i = 0; i < m_events.size(); i++)
        if (m_events[i]->id == id)
            return m_events[i];

    return nullptr;
}

int HopperDaemon::run() {
    int res = 0;

    std::cout << "Started event loop, max " << m_max_events
              << " events, timeout " << m_timeout << " ms" << std::endl;

    while (res == 0) {
        struct epoll_event *events = new struct epoll_event[m_max_events];

        int n = epoll_wait(m_epoll_fd, events, m_max_events, m_timeout);
        if (n < 0) {
            delete[] events;
            throw_errno("epoll_wait");
            return -1;
        }

        HopperEvent *ev;

        for (int i = 0; i < n; i++) {
            if ((ev = get_event(i)) == nullptr)
                continue;

            if (ev->callback != nullptr) {
                int r = ev->callback(ev);
                if (r != 0)
                    std::cerr << "Failed to run callback for Ev(fd=" << ev->fd
                              << "), code " << r << std::endl;
            }
        }

        delete[] events;

        for (const auto &[_, endpoint] : m_endpoints) {
            // This is absolutely disgusting, but I haven't thought of a better
            // way yet
            int r = endpoint->refresh(this);
            if (r != 0)
                std::cerr << "Failed to run refresh for Endpoint(path="
                          << endpoint->path() << ")" << std::endl;
        }
    }

    return res;
}

}; // namespace hopper
