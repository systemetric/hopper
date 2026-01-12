#ifndef pipe_hpp_INCLUDED
#define pipe_hpp_INCLUDED

#include <filesystem>
#include <string>

#include "hopper/daemon/marker.hpp"

namespace hopper {

enum PipeType {
    NONE,
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
    PipeStatus m_status = PipeStatus::INACTIVE;
    PipeType m_type;
    std::filesystem::path m_path;

    int m_fd = -1;
    int m_id;

public:
    HopperPipe(int id, PipeType type, std::filesystem::path path,
               BufferMarker *marker = nullptr);
    ~HopperPipe();

    int open_pipe();
    size_t write_pipe(void *src, size_t len);
    size_t read_pipe(void *dst, size_t len);

    const std::filesystem::path &path() { return m_path; }
    BufferMarker *marker() { return m_marker; }
    PipeStatus status() { return m_status; }
    int id() { return m_id; }
    int fd() { return m_fd; }
};

}; // namespace hopper

#endif // pipe_hpp_INCLUDED
