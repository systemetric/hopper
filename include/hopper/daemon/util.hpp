#ifndef util_hpp_INCLUDED
#define util_hpp_INCLUDED

#include <cerrno>
#include <cstring>
#include <sstream>
#include <string>
#include <system_error>

namespace hopper {

inline void throw_errno(const std::string &msg) {
    std::stringstream ss;
    ss << msg << ": " << std::strerror(errno);
    throw std::system_error(errno, std::generic_category(), ss.str());
}

}; // namespace hopper

#endif // util_hpp_INCLUDED
