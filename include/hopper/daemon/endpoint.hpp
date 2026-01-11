#ifndef endpoint_hpp_INCLUDED
#define endpoint_hpp_INCLUDED

#include <filesystem>
#include <map>

#include "hopper/daemon/buffer.hpp"
#include "hopper/daemon/event.hpp"
#include "hopper/daemon/pipe.hpp"

namespace hopper {

class HopperDaemon;

enum HopperEndpointOperationType {
    CREATE_EV,
    DELETE_EV,
};

struct HopperEndpointOperation {
    HopperEndpointOperationType type;
    HopperEvent *ev;
};

class HopperEndpoint {
private:
    std::map<std::filesystem::path, HopperPipe> m_inputs;
    std::map<std::filesystem::path, HopperPipe> m_outputs;
    std::vector<HopperEvent *> m_events;
    std::vector<HopperEndpointOperation *> m_operations;

    HopperBuffer m_buffer;

    std::filesystem::path m_path;
    std::filesystem::path m_in_dir;
    std::filesystem::path m_out_dir;

    int m_inotify_in_fd;
    int m_inotify_in_watch;

    int m_inotify_out_fd;
    int m_inotify_out_watch;

    std::string m_name;
    int m_id;

    // First value is inotify fd, second is watch id
    std::pair<int, int>
    create_inotify(std::filesystem::path path,
                   std::function<int(HopperEvent *)> callback);
    void validate_inotify(std::pair<int, int> &inotify);

public:
    HopperEndpoint(int id, std::filesystem::path path);

    // This is a weird fucntion. This is called by the daemon once every loop
    // iteration, which then allows endpoints to manipulate daemon state such as
    // updating events, etc.
    int refresh(HopperDaemon *d);

    int id() { return m_id; }
    const std::string &name() { return m_name; }
    const std::filesystem::path &path() { return m_path; }
};

}; // namespace hopper

#endif // endpoint_hpp_INCLUDED
