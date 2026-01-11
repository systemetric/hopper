#include <filesystem>
#include <sys/inotify.h>
#include <utility>

#include "hopper/daemon/daemon.hpp"
#include "hopper/daemon/endpoint.hpp"
#include "hopper/daemon/util.hpp"

namespace hopper {

HopperEndpoint::HopperEndpoint(int id, std::filesystem::path path)
    : m_path(path), m_id(id) {
    m_name = path.filename();

    // Why have I made my life so hard?

    m_in_dir = path / "in";
    std::pair<int, int> in_notify = create_inotify(m_in_dir, nullptr);
    validate_inotify(in_notify);
    m_inotify_in_fd = in_notify.first;
    m_inotify_in_watch = in_notify.second;

    m_out_dir = path / "out";
    std::pair<int, int> out_notify = create_inotify(m_out_dir, nullptr);
    validate_inotify(out_notify);
    m_inotify_out_fd = out_notify.first;
    m_inotify_out_watch = out_notify.second;
}

std::pair<int, int>
HopperEndpoint::create_inotify(std::filesystem::path path,
                               std::function<int(HopperEvent *)> callback) {
    if (!std::filesystem::exists(path))
        std::filesystem::create_directories(path);

    int fd = inotify_init();
    if (fd < 0)
        return std::make_pair(-1, -1);

    int watch = inotify_add_watch(fd, path.c_str(), IN_CREATE | IN_DELETE);
    if (watch < 0)
        return std::make_pair(fd, -1);

    HopperEvent *ev = new HopperEvent;
    ev->fd = fd;
    ev->data.u64 = 0;
    ev->callback = callback;

    HopperEndpointOperation *add_ev = new HopperEndpointOperation();
    add_ev->ev = ev;
    add_ev->type = HopperEndpointOperationType::CREATE_EV;

    m_operations.push_back(add_ev);

    return std::make_pair(fd, watch);
}

void HopperEndpoint::validate_inotify(std::pair<int, int> &inotify) {
    if (inotify.first == -1)
        throw_errno("inotify_init");
    if (inotify.second == -1)
        throw_errno("inotify_add_watch");
}

int HopperEndpoint::refresh(HopperDaemon *daemon) {
    while (!m_operations.empty()) {
        HopperEndpointOperation *op = m_operations.back();

        int r = 0;
        switch (op->type) {
            case HopperEndpointOperationType::CREATE_EV:
                r = daemon->add_event(op->ev);
                break;
            case HopperEndpointOperationType::DELETE_EV:
                r = daemon->remove_event(op->ev->id);
                break;
        }

        delete op;

        m_operations.pop_back();

        if (r != 0)
            return r;
    }

    return 0;
}

}; // namespace hopper
