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
    __COOLDOWN = 1   # Time between re-scans to reduce CPU usage 

    def __init__(self, pipe_dir):
        self.__pipe_dir = pipe_dir
        self.__pipes = []
        self.__handlers = {}
        self.__spec = get_handler_spec()

    def add_handler(self, h):
        self.__handlers[h.type] = h

    def cycle(self):
        self.scan()
        self.update_pipes()
        sleep(self.__COOLDOWN)

    def scan(self):
        new_pipes = os.listdir(self.__pipe_dir)

        # Remove pipes that no longer exist
        for p in self.__pipes:
            if not p.pipe_name in new_pipes:
                self.__pipes.remove(p)
                print(f"Removed {p.pipe_name}")

        # Add newly discovered pipes
        for p in new_pipes:
            matches = self.get_pipes_by_name(p)
            if not p in matches:
                pipe_name = PipeName(p, self.__pipe_dir)
                pipe = Pipe(pipe_name)

                pipe.set_handler(self.__handlers)

                # If an input pipe is not in the spec, we don't know what to do with its data
                if pipe.type == PipeType.INPUT and pipe.handler_id not in self.__spec:
                    raise

                self.__pipes.append(pipe)
                print(f"Added {p}")

    def update_pipes(self):
        for p in self.__pipes:
            if p.type == PipeType.INPUT:
                self.update_pipe(p)

    # Gets pipes whose name matches `name`
    def get_pipes_by_name(self, name):
        return [p.pipe_name for p in self.__pipes if p.pipe_name == name]

    # Gets output pipes from handlers defined in the spec file
    def get_pipes_by_handler_id(self, pipe):
        return [p for p in self.__pipes if p.handler_id in self.__spec[pipe.handler_id] and p.type == PipeType.OUTPUT]

    def update_pipe(self, p: Pipe):
        t = self.get_pipes_by_handler_id(p)

        d = p.read()
        if d == None:
            return

        print(f"{p.pipe_name} => ", end="")

        for op in t:
            print(f"{op.pipe_name} ", end="")
            out = op.handler.get_output(d)
            op.write(out)

        print("")

def registerDefaultHandlers(m: RcMuxServer):
    m.add_handler(LogHandler())
    m.add_handler(FullLogHandler())
    m.add_handler(CompleteLogHandler("tmplog.txt"))