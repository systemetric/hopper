#include <cstdint>
#include <limits.h>

#include <csignal>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

#include "hopper/daemon/daemon.hpp"
#include "hopper/daemon/endpoint.hpp"
#include "hopper/daemon/util.hpp"

namespace hopper
{

HopperDaemon::HopperDaemon(std::filesystem::path path, Logger &logger,
                           unsigned int rq_gid, int max_events, int timeout)
    : m_max_events(max_events), m_timeout(timeout), m_path(path),
      m_logger(logger), m_rq_gid(rq_gid)
{
    if (!std::filesystem::exists(path))
        std::filesystem::create_directories(path);

    // set requested group for hopper
    if (chown(path.c_str(), (uid_t)-1, (gid_t)rq_gid) == -1)
        throw_errno("chown");
    // rwxrwx--x
    if (chmod(path.c_str(), (mode_t)0771) == -1)
        throw_errno("chmod");

    // just ignore SIGPIPEs
    std::signal(SIGPIPE, SIG_IGN);

    if ((m_epoll_fd = epoll_create1(0)) < 0)
        throw_errno("epoll_create1");

    setup_inotify();
}

HopperDaemon::~HopperDaemon()
{
    for (const auto &[_, endpoint] : m_endpoints)
        delete endpoint;
}

void
HopperDaemon::process_events(struct epoll_event *events, int n_events)
{
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

int
HopperDaemon::run()
{
    int res = 0;

    setup_root_endpoints();

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

bool
HopperDaemon::check_target_mode(const std::filesystem::path &path)
{
    struct stat sb = {};
    if (stat(path.c_str(), &sb) == -1)
        throw_errno("stat");

    // gid should always match
    if (sb.st_gid != m_rq_gid) {
        m_logger.warn("GID improperly set for ", path, " expected ", m_rq_gid,
                      " got ", sb.st_gid);
        return false;
    }

    if ((sb.st_mode & S_IFDIR) && (sb.st_mode & S_IRWXG) == S_IRWXG)
        return true;

    if ((sb.st_mode & S_IFIFO) && (sb.st_mode & S_IRGRP) &&
        (sb.st_mode & S_IWGRP))
        return true;

    m_logger.warn("Permissions improperly set for ", path);

    return false;
}

}; // namespace hopper
