#!/usr/bin/python3

import os, sys
from .server import *

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <pipes directory>")
        exit(1)
    
    m = RcMuxServer(sys.argv[1])
    registerDefaultHandlers(m)

    while True:
        m.cycle()

