#include <fcntl.h>
#include <filesystem>
#include <sys/ioctl.h>
#include <unistd.h>

#include "hopper/daemon/endpoint.hpp"
#include "hopper/daemon/pipe.hpp"
#include "hopper/daemon/util.hpp"

namespace hopper {

HopperEndpoint::HopperEndpoint(uint32_t id, int watch_fd,
                               std::filesystem::path path, std::string name,
                               Logger &logger)
    : m_path(path), m_name(name), m_logger(logger), m_id(id),
      m_watch_fd(watch_fd) {}

HopperEndpoint::~HopperEndpoint() {
    for (const auto &[_, pipe] : m_inputs)
        delete pipe;
    for (const auto &[_, pipe] : m_outputs)
        delete pipe;
}

void HopperEndpoint::on_pipe_readable(uint64_t id) {
    if (!m_inputs.contains(id))
        return;

    HopperPipe *pipe = m_inputs[id];

/*
 *  Calculation for "pipe pressure"
    int p_sz, p_nb;
    if ((p_sz = fcntl(pipe->fd(), F_GETPIPE_SZ)) != -1 &&
        (ioctl(pipe->fd(), FIONREAD, &p_nb) > -1)) {
        m_logger.trace(*pipe, ": ", (float)p_nb / (float)p_sz);
    }
*/

    bool more = false;
    size_t res = m_buffer.write(pipe, &more);
    if (res == (size_t)-1)
        throw_errno("read");

    if (more && !m_more.contains(id))
        m_more.insert(id);

    m_logger.trace(*pipe, " -> ", res, " bytes", (more ? " ++" : ""));
}

void HopperEndpoint::flush_pipes() {
    auto c_more = std::unordered_set<uint64_t>(m_more.begin(), m_more.end());
    for (const uint64_t id : c_more) {
        m_more.erase(id);
        on_pipe_readable(id);
    }

    for (const auto &[_, pipe] : m_outputs) {
        if (pipe->status() == PipeStatus::INACTIVE)
            continue;

        bool more = false;
        size_t res = m_buffer.read(pipe, &more);
        if (res > 0)
            m_logger.trace(*pipe, " <- ", res, " bytes", (more ? " ++" : ""));
    }
}

HopperPipe *HopperEndpoint::pipe_by_path(const std::filesystem::path &path) {
    for (const auto &[_, pipe] : m_outputs)
        if (pipe->path() == path)
            return pipe;
    for (const auto &[_, pipe] : m_inputs)
        if (pipe->path() == path)
            return pipe;

    return 0;
}

HopperPipe *HopperEndpoint::add_input_pipe(const std::filesystem::path &path) {
    if (!std::filesystem::is_fifo(path))
        return nullptr;

    uint64_t id = next_pipe_id(1); // Type 1 for input
    if (id == 0)                   // ID 0 is never valid
        return nullptr;

    HopperPipe *p = new HopperPipe(id, m_name, PipeType::IN, path, nullptr);
    m_inputs[id] = p;

    m_logger.debug("OPEN ", *p);

    return p;
}

HopperPipe *HopperEndpoint::add_output_pipe(const std::filesystem::path &path) {
    if (!std::filesystem::is_fifo(path))
        return nullptr;

    BufferMarker *marker = m_buffer.create_marker();
    uint64_t id = next_pipe_id(0); // Type 0 for output
    if (id == 0)
        return nullptr;

    HopperPipe *p = new HopperPipe(id, m_name, PipeType::OUT, path, marker);
    m_outputs[id] = p;

    m_logger.debug("OPEN ", *p);

    return p;
}

void HopperEndpoint::remove_by_id(uint64_t pipe_id) {
    PipeType type = (pipe_id & 0x1 ? PipeType::IN : PipeType::OUT);

    if (type == PipeType::IN && m_inputs.contains(pipe_id)) {
        HopperPipe *pipe = m_inputs[pipe_id];
        m_buffer.delete_marker(pipe->marker());

        m_logger.debug("CLOSE ", *pipe);

        delete pipe;
        m_inputs.erase(pipe_id);
        m_more.erase(pipe_id);
    } else if (type == PipeType::OUT && m_outputs.contains(pipe_id)) {
        HopperPipe *pipe = m_outputs[pipe_id];
        m_buffer.delete_marker(pipe->marker());

        m_logger.debug("CLOSE ", *pipe);

        delete pipe;
        m_outputs.erase(pipe_id);
    }
}

void HopperEndpoint::remove_input_pipe(const std::filesystem::path &path) {
    for (const auto &[id, pipe] : m_inputs) {
        if (pipe->path() == path) {
            remove_by_id(id);
            break;
        }
    }
}

void HopperEndpoint::remove_output_pipe(const std::filesystem::path &path) {
    for (const auto &[id, pipe] : m_outputs) {
        if (pipe->path() == path) {
            remove_by_id(id);
            break;
        }
    }
}

}; // namespace hopper
