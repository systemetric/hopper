#include "hopper/daemon/util.hpp"

namespace hopper {

PipeType detect_pipe_type(const std::filesystem::path &path) {
    if (!path.has_extension())
        return PipeType::NONE;

    if (path.extension() == ".in")
        return PipeType::IN;

    if (path.extension() == ".out")
        return PipeType::OUT;

    return PipeType::NONE;
}

}; // namespace hopper
