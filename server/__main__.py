#!/usr/bin/python3

import sys
import logging
from .server import *

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <pipes directory>")
        exit(1)

    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
    
    m = RcMuxServer(sys.argv[1])
    registerDefaultHandlers(m)

    while True:
        m.cycle()

