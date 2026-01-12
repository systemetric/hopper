#include <filesystem>
#include <iostream>

#include "hopper/daemon/endpoint.hpp"
#include "hopper/daemon/pipe.hpp"

namespace hopper {

HopperEndpoint::HopperEndpoint(uint32_t id, int watch_fd, std::filesystem::path path)
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
    std::cout << "ID " << id << " is readable\n";
}

void HopperEndpoint::on_pipe_writable(uint64_t id) {
    std::cout << "ID " << id << " is writable\n";
}

std::pair<uint64_t, int> HopperEndpoint::add_input_pipe(const std::filesystem::path &path) {
    if (!std::filesystem::is_fifo(path))
        return std::make_pair(0, -1);
    
    uint64_t id = next_pipe_id(1); // Type 1 for input
    if (id == 0)    // ID 0 is never valid
        return std::make_pair(0, -1);

    std::cout << "OPEN IN " << path << "\n";
    
    HopperPipe *p = new HopperPipe(id, PipeType::IN, path, nullptr);
    m_inputs[id] = p;

    return std::make_pair(id, p->fd());
}

std::pair<uint64_t, int> HopperEndpoint::add_output_pipe(const std::filesystem::path &path) {
    if (!std::filesystem::is_fifo(path))
        return std::make_pair(0, -1);

    BufferMarker *marker = m_buffer.create_marker();
    uint64_t id = next_pipe_id(0); // Type 0 for output
    if (id == 0)
        return std::make_pair(0, -1);
    
    std::cout << "OPEN OUT " << path << "\n";
    
    HopperPipe *p = new HopperPipe(id, PipeType::OUT, path, marker);
    m_outputs[id] = p;

    return std::make_pair(id, p->fd());
}

void HopperEndpoint::remove_input_pipe(const std::filesystem::path &path) {
    for (const auto &[_, pipe] : m_inputs) {
        if (pipe->path() == path) {
            std::cout << "CLOSE IN " << path << "\n";
            m_inputs.erase(pipe->id());
            break;
        }
    }
}

void HopperEndpoint::remove_output_pipe(const std::filesystem::path &path) {
    for (const auto &[_, pipe] : m_outputs) {
        if (pipe->path() == path) {
            std::cout << "CLOSE OUT " << path << "\n";
            m_outputs.erase(pipe->id());
            break;
        }
    }
}

}; // namespace hopper
