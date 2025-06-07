#!/usr/bin/python3

import os, time

BUF_SIZE = 64

def create(path):
    if not os.path.exists(path):
        os.mkfifo(path)

create("pipes/O_log_test")

f = os.open("pipes/O_log_test", os.O_RDWR | os.O_NONBLOCK)

while True:
    try:
        buf = os.read(f, BUF_SIZE)
        print(buf.decode(), end="")
    except:
        continue;

os.close(f)

