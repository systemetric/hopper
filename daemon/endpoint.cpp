#include <filesystem>
#include <iostream>

#include "hopper/daemon/endpoint.hpp"
#include "hopper/daemon/pipe.hpp"
#include "hopper/daemon/util.hpp"

namespace hopper {

HopperEndpoint::HopperEndpoint(uint32_t id, int watch_fd,
                               std::filesystem::path path)
    : m_path(path), m_id(id), m_watch_fd(watch_fd) {
    m_name = path.filename();
}

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

    size_t res = m_buffer.write(pipe);
    if (res == (size_t)-1)
        throw_errno("read");

    std::cout << *pipe << " -> " << res << " bytes\n";
}

void HopperEndpoint::flush_pipes() {
    for (const auto &[_, pipe] : m_outputs) {
        if (pipe->status() == PipeStatus::INACTIVE)
            continue;

        size_t res = m_buffer.read(pipe);
        if (res > 0)
            std::cout << *pipe << " <- " << res << " bytes\n";
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

    std::cout << "OPEN " << *p << "\n";

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

    std::cout << "OPEN " << *p << "\n";

    return p;
}

void HopperEndpoint::remove_by_id(uint64_t pipe_id) {
    PipeType type = (pipe_id & 0x1 ? PipeType::IN : PipeType::OUT);

    if (type == PipeType::IN && m_inputs.contains(pipe_id)) {
        HopperPipe *pipe = m_inputs[pipe_id];
        m_buffer.delete_marker(pipe->marker());
        std::cout << "CLOSE " << *pipe << "\n";

        delete pipe;
        m_inputs.erase(pipe_id);
    } else if (type == PipeType::OUT && m_outputs.contains(pipe_id)) {
        HopperPipe *pipe = m_outputs[pipe_id];
        m_buffer.delete_marker(pipe->marker());
        std::cout << "CLOSE " << *pipe << "\n";

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
