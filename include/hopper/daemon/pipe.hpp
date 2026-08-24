#ifndef pipe_hpp_INCLUDED
#define pipe_hpp_INCLUDED

#include <filesystem>
#include <string>
#include <sys/stat.h>

#include "hopper/daemon/marker.hpp"

namespace hopper
{

enum PipeType {
    NONE,
    IN,
    OUT,
};

enum PipeStatus {
    ACTIVE,
    INACTIVE,
};

class HopperPipe
{
private:
    BufferMarker *m_marker = nullptr;

    std::string m_name;
    PipeStatus m_status = PipeStatus::INACTIVE;
    PipeType m_type;
    std::filesystem::path m_path;

    const std::string &m_endpoint_name;

    int m_fd = -1;
    uint64_t m_id;
    ino_t m_inode = 0;

public:
    HopperPipe(uint64_t id, const std::string &endpoint_name, PipeType type,
               std::filesystem::path path, ino_t inode,
               BufferMarker *marker = nullptr);
    ~HopperPipe();

    int open_pipe();
    void close_pipe();
    size_t write_pipe(void *src, size_t len, bool *more);
    size_t read_pipe(void *dst, size_t len, bool *more);

    const std::filesystem::path &
    path()
    {
        return m_path;
    }
    BufferMarker *
    marker()
    {
        return m_marker;
    }
    PipeStatus
    status()
    {
        return m_status;
    }
    PipeType
    type()
    {
        return m_type;
    }
    const std::string &
    name()
    {
        return m_name;
    }
    uint64_t
    id()
    {
        return m_id;
    }
    int
    fd()
    {
        return m_fd;
    }
    ino_t
    inode()
    {
        return m_inode;
    }

    friend std::ostream &
    operator<<(std::ostream &os, const HopperPipe &pipe)
    {
        os << (pipe.m_type == PipeType::IN ? "+" : "-") << pipe.m_name << "("
           << pipe.m_endpoint_name << ")";
        return os;
    };
};

}; // namespace hopper

#endif // pipe_hpp_INCLUDED
