#!/usr/bin/python3

import time, sys, os
from rcmux.client import *
from rcmux.common import *

root, name = os.path.split(sys.argv[1])
pn = PipeName(name, root)
c = RcMuxClient()
c.open_pipe(pn)

while True:
    buf = c.read(pn)
    if buf:
        print(buf.decode(), end="")
    time.sleep(0.5)

