#!/usr/bin/python3

import os, sys
from client import *
from common import *

root, name = os.path.split(sys.argv[1])
pn = PipeName(name, root)
c = RcMuxClient(pn)

while True:
    try:
        s = input()
        b = bytes(s, "utf-8") + b'\n'
        c.write(b)
    except:
        break;