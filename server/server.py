#!/usr/bin/python3

import os
from common import *
from .handler import *
from .spec import *
from time import sleep

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

class RcMuxServer:
    COOLDOWN = 1

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
        sleep(self.COOLDOWN)

    def scan(self):
        new_pipes = os.listdir(self.pipe_dir)

        for p in self.pipes:
            if not p.get_pipe_name() in new_pipes:
                print(f"Removed {p.get_pipe_name()}")
                self.pipes.remove(p)

        for p in new_pipes:
            matches = self.getMatchingPipeNames(p)
            if not p in matches:
                pipe_name = PipeName(p, self.pipe_dir)
                pipe = Pipe(pipe_name)

                pipe.set_handler(self.handlers)

                if pipe.get_type() == PipeType.INPUT and pipe.get_handler_id() not in self.spec:
                    raise

                self.pipes.append(pipe)
                print(f"Added {p}")

    def updatePipes(self):
        for p in self.pipes:
            if p.get_type() == PipeType.INPUT:
                self.processPipe(p)

    def getMatchingPipeNames(self, name):
        return [p.get_pipe_name() for p in self.pipes if p.get_pipe_name() == name]

    def getMatchingPipeTypes(self, pipe):
        return [p for p in self.pipes if p.get_handler_id() in self.spec[pipe.get_handler_id()] and p.get_type() == PipeType.OUTPUT]

    def processPipe(self, p: Pipe):
        t = self.getMatchingPipeTypes(p)

        d = p.read()

        if d == None:
            return

        print(f"{p.get_pipe_name()} => ", end="")

        for op in t:
            print(f"{op.get_pipe_name()} ", end="")
            out = op.get_handler().get_output(d)
            op.write(out)

        print("")

def registerDefaultHandlers(m: RcMuxServer):
    m.add_handler(LogHandler())
    m.add_handler(FullLogHandler())
    m.add_handler(CompleteLogHandler("tmplog.txt"))