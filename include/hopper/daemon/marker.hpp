#ifndef marker_hpp_INCLUDED
#define marker_hpp_INCLUDED

#include <cstddef>

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

}; // namespace hopper

#endif // marker_hpp_INCLUDED
