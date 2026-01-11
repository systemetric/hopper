#ifndef daemon_hpp_INCLUDED
#define daemon_hpp_INCLUDED

#include <map>
#include <string>
#include <vector>

#include "hopper/daemon/buffer.hpp"
#include "hopper/daemon/pipe.hpp"

namespace hopper {

class HopperDaemon {
private:
    std::map<std::string, int> m_handlers;

    std::vector<HopperPipe> m_inputs;
    std::map<int, HopperBuffer> m_buffers;
    std::map<int, std::vector<HopperPipe>> m_outputs;
};

}; // namespace hopper

#endif // server_hpp_INCLUDED
