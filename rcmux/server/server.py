#!/usr/bin/python3

import os
import logging

from rcmux.common import *
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

    def __init__(self, pipe_dir: str):
        """
        Initialize a new RcMuxServer.
        """
        self.__pipe_dir = pipe_dir
        self.__pipes = []
        self.__handlers = {}
        self.__spec = get_handler_spec()
        logging.info("RcMuxServer initialized.")

    def add_handler(self, h: PipeHandler):
        """
        Add handler `h` as a pipe handler for the server.
        """
        self.__handlers[h.type] = h
        logging.info(f"Added handler for '{h.type}'")

    def cycle(self):
        """
        Run a single scan-update-cooldown cycle.
        """
        self.scan()
        self.update_pipes()
        sleep(self.__COOLDOWN)

    def scan(self):
        """
        Scan for new pipes and remove old pipes from the server's pipe list.
        """
        new_pipes = os.listdir(self.__pipe_dir)

        # Remove pipes that no longer exist
        for p in self.__pipes:
            if not str(p.pipe_name) in new_pipes:
                self.__pipes.remove(p)
                logging.info(f"Removed '{p.pipe_path}'")
            elif p.inode_number != os.stat(p.pipe_path).st_ino:
                self.__pipes.remove(p)
                logging.info(f"Removed '{p.pipe_path}'")

        pipe_names = [str(p.pipe_name) for p in self.__pipes]

        # Add newly discovered pipes
        for p in new_pipes:
            if not p in pipe_names:
                pipe_name = PipeName(p, self.__pipe_dir)
                pipe = Pipe(pipe_name)

                pipe.set_handler(self.__handlers)

                # If an input pipe is not in the spec, we don't know what to do with its data
                if pipe.type == PipeType.INPUT and pipe.handler_id not in self.__spec:
                    logging.warning(f"No valid spec found for '{pipe.handler_id}'.")

                self.__pipes.append(pipe)
                logging.info(f"Added pipe '{pipe_name.pipe_path}'")

    def update_pipes(self):
        for p in self.__pipes:
            if p.type == PipeType.INPUT:
                self.update_pipe(p)

    # Gets output pipes from handlers defined in the spec file
    def get_pipes_by_handler_id(self, pipe):
        if pipe.handler_id in self.__spec.keys():   # The handler ID is in the spec
            return [p for p in self.__pipes if p.handler_id in self.__spec[pipe.handler_id] and p.type == PipeType.OUTPUT]
        else:
            return [p for p in self.__pipes if p.handler_id == pipe.handler_id and p.type == PipeType.OUTPUT]

    def update_pipe(self, p: Pipe):
        """
        Read content from pipe `p` and send it to corresponding output handlers.
        """
        t = self.get_pipes_by_handler_id(p)

        d = p.read()
        if d == None:
            return

        s = f"{p.pipe_name} => "

        for op in t:
            s += str(op.pipe_name) + " "
            out = op.handler.get_output(d)
            op.write(out)

        logging.info(s)

def registerDefaultHandlers(m: RcMuxServer):
    """
    Register the default pipe handlers with the server `m`:
     - LogHandler
     - FullLogHandler
     - CompleteLogHandler
    """
    m.add_handler(LogHandler())
    m.add_handler(FullLogHandler())
    m.add_handler(CompleteLogHandler("tmplog.txt"))
    m.add_handler(StartButtonHandler())