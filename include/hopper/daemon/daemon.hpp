#ifndef daemon_hpp_INCLUDED
#define daemon_hpp_INCLUDED

#include <sys/epoll.h>
#include <sys/inotify.h>

#include <cstdint>
#include <filesystem>
#include <unordered_map>

#include "hopper/daemon/endpoint.hpp"
#include "hopper/daemon/pipe.hpp"

namespace hopper {

constexpr uint64_t INOTIFY_DATA = 0x1;

class HopperDaemon {
private:
    std::unordered_map<uint32_t, HopperEndpoint *> m_endpoints;

    // endpoint IDs are 24 bit
    uint32_t m_last_endpoint_id = 1;
    uint32_t next_endpoint_id() {
        if (m_last_endpoint_id > ((1ULL << 24) - 1))
            return 0;

        return m_last_endpoint_id++;
    }

    int m_inotify_fd = -1;
    int m_inotify_root_watch = -1;
    int m_epoll_fd = -1;

    int m_max_events = 64;
    int m_timeout = 250;

    std::filesystem::path m_path;

    uint32_t create_endpoint(const std::filesystem::path &path);
    void delete_endpoint(const std::filesystem::path &path);
    void delete_endpoint(uint32_t id);

    HopperEndpoint *endpoint_by_watch(int watch);
    void setup_inotify();
    void handle_inotify();
    void handle_root_inotify(struct inotify_event *ev);
    void handle_endpoint_inotify(struct inotify_event *ev,
                                 HopperEndpoint *endpoint);

    void process_events(struct epoll_event *events, int n_events);
    void remove_pipe(HopperEndpoint *endpoint, uint64_t pipe_id);
    void add_pipe(HopperPipe *pipe);
    void refresh_pipes();

public:
    HopperDaemon(std::filesystem::path path, int max_events = 64,
                 int m_timeout = 250);
    ~HopperDaemon();

    int run();
};

}; // namespace hopper

#endif // daemon_hpp_INCLUDED
