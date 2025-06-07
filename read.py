#!/usr/bin/python3

import os, time

BUF_SIZE = 64

f = os.open("test", os.O_RDWR | os.O_NONBLOCK)

while True:
    try:
        buf = os.read(f, BUF_SIZE)
        print(buf.decode(), end="")
    except:
        continue;

os.close(f)

