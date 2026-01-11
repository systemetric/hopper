#ifndef buffer_hpp_INCLUDED
#define buffer_hpp_INCLUDED

#include "hopper/server/pipe.hpp"
#include <vector>

namespace hopper {

enum SeekDirection {
    FORWARD,
    REVERSE,
};

class BufferMarker {
private:
    size_t m_pos;

public:
    BufferMarker(size_t pos = 0) : m_pos(pos) {}

    void seek(size_t offset, size_t max,
              SeekDirection dir = SeekDirection::FORWARD);

    size_t pos() { return m_pos; }
};

class HopperBuffer {
private:
    std::vector<char> m_buf;
    std::vector<BufferMarker *> m_markers;

    size_t m_edge;

public:
    HopperBuffer(size_t len = 1024 * 1024)
        : m_buf(len) {} // Use 1 MiB size by default

    BufferMarker* create_marker();

    size_t write(void *src, size_t len);
    size_t write(const HopperPipe &pipe, size_t len);

    size_t read(BufferMarker *marker, void *dst, size_t len);
    size_t read(BufferMarker *marker, const HopperPipe &pipe, size_t len);

    size_t max_write();
    size_t max_read(BufferMarker *marker);

    size_t edge() { return m_edge; }
};

}; // namespace hopper

#endif // buffer_hpp_INCLUDED
