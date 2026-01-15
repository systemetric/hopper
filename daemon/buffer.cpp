#include <algorithm>
#include <cstring>

#include "hopper/daemon/buffer.hpp"
#include "hopper/daemon/marker.hpp"
#include "hopper/daemon/pipe.hpp"

namespace hopper {

/* BufferMarker */

void BufferMarker::seek(size_t offset, size_t max, SeekDirection dir) {
    if (dir == SeekDirection::FORWARD)
        m_pos = (m_pos + offset) % max;
    else
        m_pos = ((m_pos > offset) ? m_pos - offset : max - (offset - m_pos));
}

/* HopperBuffer */

HopperBuffer::HopperBuffer(size_t len) : m_edge(0) { m_buf.resize(len); }

BufferMarker *HopperBuffer::create_marker() {
    auto *m = new BufferMarker(m_edge);
    m_markers.push_back(m);
    return m;
}

void HopperBuffer::delete_marker(BufferMarker *marker) {
    auto tgt = std::find(m_markers.begin(), m_markers.end(), marker);
    if (tgt != m_markers.end())
        m_markers.erase(tgt);
}

size_t HopperBuffer::write(void *src, size_t len) {
    size_t max_len = std::min(len, max_write());
    size_t done_len = 0;

    // Write up to the buffer length, but only if len > buf_len
    size_t next_len = std::min((m_buf.size() - m_edge), max_len);
    std::memcpy(&m_buf[m_edge], src, next_len);
    m_edge = (m_edge + next_len) % m_buf.size();

    // Enough bytes have been written, return
    done_len += next_len;
    if (done_len >= max_len)
        return done_len;

    // We are guaranteed tp have space for whatever's left
    next_len = max_len - next_len;
    std::memcpy(&m_buf[m_edge], reinterpret_cast<char *>(src) + done_len,
                next_len);
    m_edge = (m_edge + next_len) % m_buf.size();

    done_len += next_len;
    return done_len;
}

size_t HopperBuffer::write(HopperPipe *pipe) {
    size_t max_len = max_write();
    size_t done_len = 0;

    size_t next_len = std::min((m_buf.size() - m_edge), max_len);
    size_t res = pipe->read_pipe(&m_buf[m_edge], next_len);
    if (res == (size_t)-1)
        // -1 indicates read error
        return -1;

    m_edge = (m_edge + res) % m_buf.size();
    done_len += res;
    if (res <= next_len)
        // read_pipe reads as much as possible up to next_len, so the pipe is
        // empty here
        return done_len;

    next_len = max_len - next_len;
    res = pipe->read_pipe(&m_buf[m_edge], next_len);
    if (res == (size_t)-1)
        return -1;

    m_edge = (m_edge + res) % m_buf.size();
    done_len += res;
    return done_len;
}

size_t HopperBuffer::read(BufferMarker *m, void *dst, size_t len) {
    // Just see the arithmetic in `write`, it's the same here.

    size_t max_len = std::min(len, max_read(m));
    size_t done_len = 0;

    size_t next_len = std::min((m_buf.size() - m->pos()), max_len);
    std::memcpy(dst, &m_buf[m->pos()], next_len);
    m->seek(next_len, m_buf.size(), SeekDirection::FORWARD);

    done_len += next_len;
    if (done_len >= max_len)
        return done_len;

    next_len = max_len - next_len;
    std::memcpy(reinterpret_cast<char *>(dst) + done_len, &m_buf[m->pos()],
                next_len);
    m->seek(next_len, m_buf.size(), SeekDirection::FORWARD);

    done_len += next_len;
    return done_len;
}

size_t HopperBuffer::read(HopperPipe *pipe) {
    BufferMarker *m = pipe->marker();
    size_t max_len = max_read(m);
    size_t done_len = 0;

    size_t next_len = std::min((m_buf.size() - m->pos()), max_len);
    size_t res = pipe->write_pipe(&m_buf[m->pos()], next_len);
    if (res == (size_t)-1)
        return -1;

    m->seek(res, m_buf.size(), SeekDirection::FORWARD);
    done_len += res;
    if (res <= next_len)
        return done_len;

    next_len = max_len - next_len;
    res = pipe->write_pipe(&m_buf[m->pos()], next_len);
    if (res == (size_t)-1)
        return -1;

    m->seek(res, m_buf.size(), SeekDirection::FORWARD);
    done_len += res;
    return done_len;
}

size_t HopperBuffer::max_write() {
    size_t cap = m_buf.size();
    if (m_markers.empty())
        return cap;

    size_t min_dist = cap;

    for (auto *m : m_markers) {
        size_t d = (m->pos() - m_edge + cap) % cap;

        if (d == 0) // Marker is at m_edge, full buffer left
            d = cap;

        if (d < min_dist)
            min_dist = d;
    }

    return min_dist;
}

size_t HopperBuffer::max_read(BufferMarker *m) {
    size_t cap = m_buf.size();
    return (m_edge - m->pos() + cap) % cap;
}

}; // namespace hopper
