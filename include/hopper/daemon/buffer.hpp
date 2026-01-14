#ifndef buffer_hpp_INCLUDED
#define buffer_hpp_INCLUDED

#include <vector>

#include "hopper/daemon/marker.hpp"

namespace hopper {

// Opaque class, see hopper/server/pipe.hpp
class HopperPipe;

class HopperBuffer {
private:
    std::vector<char> m_buf;
    std::vector<BufferMarker *> m_markers;

    size_t m_edge;

public:
    HopperBuffer(size_t len = 1024 * 1024)
        : m_buf(len) {} // Use 1 MiB size by default

    BufferMarker *create_marker();
    void delete_marker(BufferMarker *marker);

    size_t write(void *src, size_t len);
    size_t write(HopperPipe *pipe);

    size_t read(BufferMarker *marker, void *dst, size_t len);
    size_t read(HopperPipe *pipe);

    size_t max_write();
    size_t max_read(BufferMarker *marker);

    size_t edge() { return m_edge; }
};

}; // namespace hopper

#endif // buffer_hpp_INCLUDED
