#!/usr/bin/python3

import threading, os, sys, time, io

BUF_SIZE = 64
IN = "a"
OUT = "b"

def create(path):
    if not os.path.exists(path):
        os.mkfifo(path)

def writer():
    create(OUT)
    f = os.open(OUT, os.O_NONBLOCK | os.O_RDWR)
    while True:
        try:
            s = input()
            b = bytes(s, "utf-8") + b'\n'
            os.write(f, b)
        except:
            break

    os.close(f)

def reader():
    create(IN)
    f = os.open(IN, os.O_NONBLOCK | os.O_RDWR)
    while True:
        try:
            buf = os.read(f, BUF_SIZE)
            print(buf.decode(), end="")
        except io.BlockingIOError:
            continue
        except:
            break

    os.close(f)

OUT, IN = sys.argv[1:3]

r = threading.Thread(target=reader, daemon=True)
w = threading.Thread(target=writer, daemon=True)

r.start()
w.start()

while True:
    time.sleep(1)