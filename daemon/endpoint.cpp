#include "hopper/daemon/endpoint.hpp"

namespace hopper {

HopperEndpoint::HopperEndpoint(std::filesystem::path path): m_path(path) {

}

int HopperEndpoint::refresh(HopperDaemon *daemon) {
    return 1;
}

};
