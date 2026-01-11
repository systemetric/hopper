#ifndef daemon_hpp_INCLUDED
#define daemon_hpp_INCLUDED

#include <sys/epoll.h>

#include <cstdint>
#include <filesystem>
#include <map>
#include <vector>

#include "hopper/daemon/endpoint.hpp"
#include "hopper/daemon/event.hpp"

namespace hopper {

constexpr uint64_t INOTIFY_DATA = 0x1;

class HopperDaemon {
private:
    std::map<int, HopperEndpoint *> m_endpoints;
    std::vector<HopperEvent *> m_events;

    std::filesystem::path m_path;

    uint64_t m_last_event_id = 0;
    int m_last_endpoint_id = 0;

    int m_inotify_fd = -1;
    int m_inotify_watch_fd = -1;

    int m_epoll_fd = -1;

    int m_max_events = 64;
    int m_timeout = 250;

    uint64_t next_event_id() { return m_last_event_id++; }
    int next_endpoint_id() { return m_last_endpoint_id++; }

    int create_endpoint(std::filesystem::path path);
    int delete_endpoint(std::filesystem::path path);
    int delete_endpoint(int id);

    int handle_inotify(HopperEvent *ev);

public:
    HopperDaemon(std::filesystem::path path, int max_events = 64,
                 int m_timeout = 250);
    ~HopperDaemon();

    int run();

    int add_event(HopperEvent *event, int events = EPOLLIN);
    int remove_event(uint64_t id);
    int remove_event(HopperEvent *event);
    HopperEvent *get_event(uint64_t id);
};

}; // namespace hopper

#endif // daemon_hpp_INCLUDED
