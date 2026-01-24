#include "hopper/daemon/daemon.hpp"
#include "hopper/daemon/util.hpp"
#include <filesystem>
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

    std::cout << "CREATE " << *endpoint << std::endl;

    // Open anything that may already exist in the endpoint
    for (const auto &dir_entry : std::filesystem::directory_iterator{path}) {
        const auto &p = dir_entry.path();

        PipeType pipe_type = detect_pipe_type(p);
        if (pipe_type == PipeType::NONE)
            continue;

        HopperPipe *pipe =
            (pipe_type == PipeType::IN ? endpoint->add_input_pipe(p)
                                       : endpoint->add_output_pipe(p));

        if (pipe != nullptr)
            add_pipe(pipe);
    }

    return endpoint_id;
}

void HopperDaemon::delete_endpoint(uint32_t id) {
    if (!m_endpoints.contains(id))
        return;

    std::cout << "DELETE " << *(m_endpoints[id]) << std::endl;

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
