#include "hopper/daemon/daemon.hpp"
#include "hopper/daemon/util.hpp"
#include <filesystem>

namespace hopper
{

uint32_t
HopperDaemon::create_endpoint(const std::filesystem::path &path)
{
    uint32_t endpoint_id = next_endpoint_id();
    if (endpoint_id == 0)
        return 0;

    int inotify_watch_fd =
        inotify_add_watch(m_inotify_fd, path.c_str(), IN_CREATE | IN_DELETE);
    if (inotify_watch_fd < 0)
        return 0;

    auto name = path.lexically_relative(m_path);
    auto *endpoint =
        new HopperEndpoint(endpoint_id, inotify_watch_fd, path, name, m_logger);
    m_endpoints[endpoint_id] = endpoint;

    m_logger.debug("CREATE ", *endpoint);

    // Open anything that may already exist in the endpoint
    for (const auto &dir_entry : std::filesystem::directory_iterator{path}) {
        const auto &p = dir_entry.path();

        PipeType pipe_type = detect_pipe_type(p);
        if (pipe_type == PipeType::NONE && std::filesystem::is_directory(p)) {
            // nested endpoint
            if (create_endpoint(p) == 0)
                m_logger.warn("Endpoint creation failed! Out of IDs?");
        } else if (pipe_type == PipeType::NONE) {
            // nothing of interest
            continue;
        } else {
            // pipe
            HopperPipe *pipe =
                (pipe_type == PipeType::IN ? endpoint->add_input_pipe(p)
                                           : endpoint->add_output_pipe(p));

            if (pipe != nullptr)
                add_pipe(pipe);
        }
    }

    return endpoint_id;
}

void
HopperDaemon::delete_endpoint(uint32_t id)
{
    if (!m_endpoints.contains(id))
        return;

    m_logger.debug("DELETE ", *(m_endpoints[id]));

    inotify_rm_watch(m_inotify_fd, m_endpoints[id]->watch_fd());

    delete m_endpoints[id];
    m_endpoints.erase(id);
}

void
HopperDaemon::delete_endpoint(const std::filesystem::path &path)
{
    for (const auto &[_, endpoint] : m_endpoints) {
        if (endpoint->path() == path) {
            delete_endpoint(endpoint->id());
            break;
        }
    }
}

HopperEndpoint *
HopperDaemon::endpoint_by_watch(int watch)
{
    for (const auto &[_, endpoint] : m_endpoints)
        if (endpoint->watch_fd() == watch)
            return endpoint;
    return nullptr;
}

HopperEndpoint *
HopperDaemon::endpoint_by_path(const std::filesystem::path &path)
{
    for (const auto &[_, endpoint] : m_endpoints)
        if (endpoint->path() == path)
            return endpoint;
    return nullptr;
}

void
HopperDaemon::setup_root_endpoints()
{
    // iter over all directories and add them as endpoints
    for (const auto &dir_entry : std::filesystem::directory_iterator{m_path}) {
        const auto &p = dir_entry.path();

        if (std::filesystem::is_directory(p) && create_endpoint(p) == 0)
            m_logger.warn("Endpoint creation failed! Out of IDs?");
    }
}

}; // namespace hopper
