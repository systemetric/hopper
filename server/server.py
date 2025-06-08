#!/usr/bin/python3

import os
from server.pipe import *
from server.handler import *
from server.spec import *

"""
Pipe naming scheme:
    - Fields seperated by underscores (_)
    - Fields:
        I/O     = Input or output 
        handler = Handler string, cannot contain underscores
        id      = ID of the pipe 
    - Example: I_log_test
        I       = Input pipe
        log     = Log pipe handler
        test    = Pipe ID
"""

class Mux:
    def __init__(self, pipe_dir):
        self.pipe_dir = pipe_dir
        self.pipes = []
        self.handlers = {}
        self.spec = getHandlerSpec()

    def add_handler(self, h):
        self.handlers[h.type] = h

    def cycle(self):
        self.scan()
        self.updatePipes()

    def scan(self):
        new_pipes = os.listdir(self.pipe_dir)

        for p in self.pipes:
            if not p.name in new_pipes:
                print(f"Removed {p.name}")
                self.pipes.remove(p)

        for p in new_pipes:
            if not p in self.getMatchingPipeNames(p):
                path = self.pipe_dir + "/" + p
                pipe = Pipe(p, path)

                if pipe.pipe_type == PipeType.INPUT and pipe.type not in self.spec:
                    raise

                try:
                    pipe.assign_handler(self.handlers[pipe.type])
                except KeyError:
                    continue

                self.pipes.append(pipe)
                print(f"Added {p}")

    def updatePipes(self):
        for p in self.pipes:
            if p.pipe_type == PipeType.INPUT:
                self.processPipe(p)

    def getMatchingPipeNames(self, name):
        return [p.name for p in self.pipes if p.name == name]

    def getMatchingPipeTypes(self, pipe):
        return [p for p in self.pipes if p.type in self.spec[pipe.type] and p.pipe_type == PipeType.OUTPUT]

    def processPipe(self, p):
        t = self.getMatchingPipeTypes(p)

        d = p.read()

        if d == None:
            return

        for op in t:
            out = op.handler.get_output(d)
            op.write(out)

def registerDefaultHandlers(m):
    m.add_handler(LogHandler())
    m.add_handler(FullLogHandler())
    m.add_handler(CompleteLogHandler("tmplog.txt"))