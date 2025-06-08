#!/usr/bin/python3

import os, sys
from server.server import *

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <pipes directory>")
        exit(1)
    
    m = Mux(sys.argv[1])
    registerDefaultHandlers(m)

    while True:
        m.cycle()

