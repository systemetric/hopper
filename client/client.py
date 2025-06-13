import os

from common import *

class RcMuxClient:
    def __init__(self):
        self.__pipes = []

    def open_pipe(self, pn):
        pipe = Pipe(pn, create=True)
        self.__pipes.append(pipe)

    def close_pipe(self, pn):
        p = self.get_pipe_by_pipe_name(pn)
        if p == None:
            raise

        p.close()
        self.__pipes.remove(p)
    
    def get_pipe_by_pipe_name(self, pn):
        for p in self.__pipes:
            if p.pipe_name == pn:
                return p
        return None

    def read(self, pn):
        p = self.get_pipe_by_pipe_name(pn)
        if p == None:
            raise

        return p.read()
    
    def write(self, pn, buf):
        p = self.get_pipe_by_pipe_name(pn)
        if p == None:
            raise

        return p.write(buf)
