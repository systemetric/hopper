#!/usr/bin/python3

import time, sys, os
from client import *
from common import *

root, name = os.path.split(sys.argv[1])
pn = PipeName(name, root)
c = RcMuxClient(pn)

while True:
    buf = c.read()
    if buf:
        print(buf.decode(), end="")
    time.sleep(0.5)

