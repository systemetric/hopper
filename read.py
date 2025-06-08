#!/usr/bin/python3

import os, sys

BUF_SIZE = 64

def create(path):
    if not os.path.exists(path):
        os.mkfifo(path)

create(sys.argv[1])

f = os.open(sys.argv[1], os.O_RDWR | os.O_NONBLOCK)

while True:
    try:
        buf = os.read(f, BUF_SIZE)
        print(buf.decode(), end="")
    except:
        continue;

os.close(f)

