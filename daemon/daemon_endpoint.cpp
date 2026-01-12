#include "hopper/daemon/daemon.hpp"
#include <iostream>

namespace hopper {

uint32_t HopperDaemon::create_endpoint(const std::filesystem::path &path) {
    uint32_t endpoint_id = next_endpoint_id();
    if (endpoint_id == 0)
        return 0;

    int inotify_watch_fd =
        inotify_add_watch(m_inotify_fd, path.c_str(), IN_CREATE | IN_DELETE);
    if (inotify_watch_fd < 0) {
        return 0;
    }

    auto *endpoint = new HopperEndpoint(endpoint_id, inotify_watch_fd, path);
    m_endpoints[endpoint_id] = endpoint;

    std::cout << "CREATE " << endpoint->path() << std::endl;

    return endpoint_id;
}

void HopperDaemon::delete_endpoint(uint32_t id) {
    if (!m_endpoints.contains(id))
        return;

    std::cout << "DELETE " << m_endpoints[id]->path() << std::endl;

    delete m_endpoints[id];
    m_endpoints.erase(id);
}

void HopperDaemon::delete_endpoint(const std::filesystem::path &path) {
    for (const auto &[_, endpoint] : m_endpoints) {
        if (endpoint->path() == path) {
            delete_endpoint(endpoint->id());
            break;
        }
    }
}

HopperEndpoint *HopperDaemon::endpoint_by_watch(int watch) {
    for (const auto &[_, endpoint] : m_endpoints)
        if (endpoint->watch_fd() == watch)
            return endpoint;
    return nullptr;
}

}; // namespace hopper
