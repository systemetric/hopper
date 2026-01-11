#include <cstring>

#include "hopper/server/buffer.hpp"

namespace hopper {

/* BufferMarker */

void BufferMarker::seek(size_t offset, size_t max, SeekDirection dir) {
    if (dir == SeekDirection::FORWARD)
        m_pos = (m_pos + offset) % max;
    else
        m_pos = ((m_pos > offset) ? m_pos - offset : max - (offset - m_pos));
}

/* HopperBuffer */

BufferMarker *HopperBuffer::create_marker() {
    auto *m = new BufferMarker(m_edge);
    m_markers.push_back(m);
    return m;
}

size_t HopperBuffer::write(void *src, size_t len) {
    size_t max_len = std::min(len, max_write());
    size_t done_len = 0;

    // Write up to the buffer length, but only if len > buf_len
    size_t next_len = std::min((m_buf.size() - m_edge), max_len);
    std::memcpy(src, &m_buf[m_edge], next_len);
    m_edge = (m_edge + next_len) % m_buf.size();

    // Enough bytes have been written, return
    done_len += next_len;
    if (done_len >= max_len)
        return done_len;

    // We are guaranteed tp have space for whatever's left
    next_len = max_len - next_len;
    std::memcpy(reinterpret_cast<char *>(src) + done_len, &m_buf[m_edge],
                next_len);
    m_edge = (m_edge + next_len) % m_buf.size();

    done_len += next_len;
    return done_len;
}

size_t HopperBuffer::read(BufferMarker *m, void *dst, size_t len) {
    // Just see the arithmetic in `write`, it's the same here.

    size_t max_len = std::min(len, max_read(m));
    size_t done_len = 0;

    size_t next_len = std::min((m_buf.size() - m->pos()), max_len);
    std::memcpy(&m_buf[m->pos()], dst, next_len);
    m->seek(next_len, m_buf.size(), SeekDirection::FORWARD);

    done_len += next_len;
    if (done_len >= max_len)
        return done_len;

    next_len = max_len - next_len;
    std::memcpy(&m_buf[m->pos()], reinterpret_cast<char *>(dst) + done_len,
                next_len);
    m->seek(next_len, m_buf.size(), SeekDirection::FORWARD);

    done_len += next_len;
    return done_len;
}

size_t HopperBuffer::max_write() {
    if (m_markers.empty())
        return m_buf.size();

    size_t min_pos = m_edge;

    for (auto *m : m_markers)
        if (m->pos() < min_pos)
            min_pos = m->pos();

    return ((m_edge <= min_pos) ? min_pos - m_edge
                                : (m_buf.size() - m_edge) + min_pos);
}

size_t HopperBuffer::max_read(BufferMarker *m) {
    return ((m->pos() <= m_edge) ? m_edge - m->pos()
                                 : (m_buf.size() - m->pos()) + m_edge);
}

}; // namespace hopper
