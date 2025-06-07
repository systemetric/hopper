#!/usr/bin/python3

import os, time

def create(path):
    if not os.path.exists(path):
        os.mkfifo(path)

create("pipes/I_fulllog_test")

f = os.open("pipes/I_fulllog_test", os.O_NONBLOCK | os.O_RDWR)

while True:
    try:
        s = input()
        b = bytes(s, "utf-8") + b'\n'
        os.write(f, b)
    except:
        break;

os.close(f)