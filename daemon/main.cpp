#include <cstdlib>
#include <exception>
#include <iostream>

#include "hopper/daemon/daemon.hpp"
#include "hopper/daemon/logging.hpp"

int main(int argc, char *argv[]) {
    hopper::LogLevel log_level = hopper::LogLevel::Debug;

    char *l = getenv("HOPPER_LOG_LEVEL");
    if (l && l[1] == '\0' && l[0] >= '0' && l[0] <= '4') {
        log_level = static_cast<hopper::LogLevel>(l[0] - '0');
    }

    auto logger = hopper::Logger(log_level);

    try {
        if (argc > 2) {
            std::cout << "Usage: hopperd <hopper path>" << std::endl;
            return 1;
        } else if (argc == 2) {
            logger.info("HOPPER ", argv[1]);
            auto daemon = hopper::HopperDaemon(argv[1], logger);
            return daemon.run();
        } else if (char *p = getenv("HOPPER_PATH")) {
            logger.info("HOPPER ", p);
            auto daemon = hopper::HopperDaemon(p, logger);
            return daemon.run();
        } else {
            logger.error("Could not find Hopper instance, try setting "
                         "HOPPER_PATH, or passing as argument");
            return 1;
        }
    } catch (const std::exception &e) {
        logger.error("Exception: ", e.what());
        return 1;
    } catch (...) {
        logger.error("Unknown exception");
        return 1;
    }
}
