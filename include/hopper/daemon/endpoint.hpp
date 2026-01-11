#ifndef endpoint_hpp_INCLUDED
#define endpoint_hpp_INCLUDED

#include <filesystem>
#include <map>

#include "hopper/daemon/buffer.hpp"
#include "hopper/daemon/event.hpp"
#include "hopper/daemon/pipe.hpp"

namespace hopper {

class HopperDaemon;

class HopperEndpoint {
private:
    std::map<std::filesystem::path, HopperPipe> m_inputs;
    std::map<std::filesystem::path, HopperPipe> m_outputs;
    std::vector<HopperEvent *> m_events;

    HopperBuffer m_buffer;

    std::filesystem::path m_path;

    int m_inotify_watch_in;
    int m_inotify_watch_out;

    std::string m_name;

public:
    HopperEndpoint(std::filesystem::path path);

    // This is a weird fucntion. This is called by the daemon once every loop
    // iteration, which then allows endpoints to manipulate daemon state such as
    // updating events, etc.
    int refresh(HopperDaemon *d);
};

}; // namespace hopper

#endif // endpoint_hpp_INCLUDED
