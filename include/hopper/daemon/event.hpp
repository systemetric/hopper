#ifndef event_hpp_INCLUDED
#define event_hpp_INCLUDED

#include <cstdint>
#include <functional>

namespace hopper {

union HopperEventData {
    uint64_t u64;
    void *ptr;
};

struct HopperEvent {
    int fd;
    uint64_t id;
    HopperEventData data;
    std::function<int(HopperEvent *)> callback;
};

}; // namespace hopper

#endif // event_hpp_INCLUDED
