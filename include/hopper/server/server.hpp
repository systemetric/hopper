#ifndef hopper_hpp_INCLUDED
#define hopper_hpp_INCLUDED

#include <map>
#include <string>
#include <vector>

#include "hopper/server/buffer.hpp"
#include "hopper/server/pipe.hpp"

namespace hopper {

class HopperServer {
private:
    std::map<std::string, int> m_handlers;

    std::vector<HopperPipe> m_inputs;
    std::map<int, HopperBuffer> m_buffers;
    std::map<int, std::vector<HopperPipe>> m_outputs;
};

}; // namespace hopper

#endif // hopper_hpp_INCLUDED
