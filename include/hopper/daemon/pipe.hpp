#ifndef pipe_hpp_INCLUDED
#define pipe_hpp_INCLUDED

#include <filesystem>
#include <string>

#include "hopper/daemon/marker.hpp"

namespace hopper {

enum PipeType {
    IN,
    OUT,
};

enum PipeStatus {
    ACTIVE,
    INACTIVE,
};

class HopperPipe {
private:
    BufferMarker *m_marker = nullptr;

    std::string m_name;
    int m_handler;
    PipeType m_type;

    std::filesystem::path m_path;

    int m_fd = -1;

    PipeStatus m_status = PipeStatus::INACTIVE;

public:
    HopperPipe(std::string name, int handler, PipeType type,
               std::filesystem::path path, BufferMarker *marker = nullptr);

    ~HopperPipe();

    int open_pipe();
    size_t write_pipe(void *src, size_t len);
    size_t read_pipe(void *dst, size_t len);

    int fd() { return m_fd; }

    const std::string &name() { return m_name; }
    int handler() { return m_handler; }
    PipeType type() { return m_type; }

    const std::filesystem::path &path() { return m_path; }

    BufferMarker *marker() { return m_marker; }

    PipeStatus status() { return m_status; }
};

}; // namespace hopper

#endif // pipe_hpp_INCLUDED
