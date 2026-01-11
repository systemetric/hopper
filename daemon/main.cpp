#include <cstdlib>
#include <exception>
#include <iostream>

#include "hopper/daemon/daemon.hpp"

int main(int argc, char *argv[]) {
    try {
        if (argc > 2) {
            std::cout << "Usage: hopperd <hopper path>" << std::endl;
            return 1;
        } else if (argc == 2) {
            std::cout << "Using Hopper at " << argv[1] << std::endl;
            auto daemon = hopper::HopperDaemon(argv[1]);
            return daemon.run();
        } else if (char *p = getenv("HOPPER_PATH")) {
            std::cout << "Using Hopper at " << p << std::endl;
            auto daemon = hopper::HopperDaemon(p);
            return daemon.run();
        } else {
            std::cerr
                << "Could not find Hopper instance, try setting HOPPER_PATH, "
                   "or passing as argument!"
                << std::endl;
            return 1;
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return 1;
    }
}
