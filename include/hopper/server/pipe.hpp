#ifndef pipe_hpp_INCLUDED
#define pipe_hpp_INCLUDED

#include <filesystem>
#include <string>

#include "hopper/server/marker.hpp"

namespace hopper {

enum PipeType {
    IN,
    OUT,
};

class HopperPipe {
private:
    BufferMarker *m_marker;

    std::string m_name;
    int m_handler;
    PipeType m_type;

    std::filesystem::path m_path;

    int m_fd;

public:
    HopperPipe(std::filesystem::path path, BufferMarker *marker);

    size_t write(void *src, size_t len);
    size_t read(void *dst, size_t len);

    int fd() { return m_fd; }

    const std::string &name() { return m_name; }
    int handler() { return m_handler; }
    PipeType type() { return m_type; }

    const std::filesystem::path &path() { return m_path; }

    BufferMarker *marker() { return m_marker; }
};

}; // namespace hopper

#endif // pipe_hpp_INCLUDED
