#ifndef endpoint_hpp_INCLUDED
#define endpoint_hpp_INCLUDED

#include <unordered_map>

#include "hopper/daemon/buffer.hpp"
#include "hopper/daemon/pipe.hpp"

namespace hopper {

class HopperEndpoint {
private:
    std::unordered_map<uint64_t, HopperPipe *> m_inputs;
    std::unordered_map<uint64_t, HopperPipe *> m_outputs;

    HopperBuffer m_buffer;

    uint64_t m_last_pipe_id = 1;
    uint64_t next_pipe_id(uint8_t type) {
        if (m_last_pipe_id > ((1ULL << 39) - 1))
            return 0;

        // I can say with high confidence, that we will probably
        // never hit this limit.

        // Bit mask for pipe ID (64-bit):
        // EEEEEEEEEEEEEEEEEEEEEEEEPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPT
        // EEE: Endpoint ID, 24-bit, ~ 16 million endpoints
        // PPP: Pipe ID, 39-bit, ~ 550 billion pipes per endpoint
        // T: Type, 1 for input, 0 for output
        return (((uint64_t)m_id) << 40) |
               ((m_last_pipe_id++ << 1) & 0xFFFFFFFFFF) | (type & 0x1);
    }

    std::filesystem::path m_path;
    std::string m_name;
    uint32_t m_id;
    int m_watch_fd;

public:
    HopperEndpoint(uint32_t id, int watch_fd, std::filesystem::path path);
    ~HopperEndpoint();

    void on_pipe_readable(uint64_t id);
    void on_pipe_writable(uint64_t id);

    std::pair<uint64_t, int> add_input_pipe(const std::filesystem::path &path);
    std::pair<uint64_t, int> add_output_pipe(const std::filesystem::path &path);
    void remove_input_pipe(const std::filesystem::path &path);
    void remove_output_pipe(const std::filesystem::path &path);

    const std::filesystem::path &path() { return m_path; }
    const std::string &name() { return m_name; }
    const std::unordered_map<uint64_t, HopperPipe *> inputs() {
        return m_inputs;
    }
    const std::unordered_map<uint64_t, HopperPipe *> outputs() {
        return m_outputs;
    }
    int id() { return m_id; }
    int watch_fd() { return m_watch_fd; }
};

}; // namespace hopper

#endif // endpoint_hpp_INCLUDED
