#ifndef hopper_hpp_INCLUDED
#define hopper_hpp_INCLUDED

#include <map>
#include <vector>

#include "hopper/server/buffer.hpp"
#include "hopper/server/handler.hpp"
#include "hopper/server/pipe.hpp"

namespace hopper {

class HopperServer {
private:
    std::vector<HopperPipe> m_inputs;
    std::map<HandlerType, HopperBuffer> m_buffers;
    std::map<HandlerType, std::vector<HopperPipe>> m_outputs;
};

}; // namespace hopper

#endif // hopper_hpp_INCLUDED
