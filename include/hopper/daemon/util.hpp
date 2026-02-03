#ifndef util_hpp_INCLUDED
#define util_hpp_INCLUDED

#include "hopper/daemon/pipe.hpp"
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <string>
#include <system_error>

namespace hopper {

inline void throw_errno(const std::string &msg) {
    std::stringstream ss;
    ss << msg << ": " << std::strerror(errno);
    throw std::system_error(errno, std::generic_category(), ss.str());
}

PipeType detect_pipe_type(const std::filesystem::path &path);

}; // namespace hopper

#endif // util_hpp_INCLUDED
