from rcmux.common import *

class RcMuxClient:
    def __init__(self):
        """
        Initialize a new RcMuxClient.
        """
        self.__pipes = []

    def open_pipe(self, pn, create = True, delete = False):
        """
        Open a pipe specified by a PipeName `pn`.
        """
        pipe = Pipe(pn, create=create, delete=delete)
        self.__pipes.append(pipe)

    def close_pipe(self, pn):
        """
        Close a pipe specified by a PipeName `pn`.
        """
        p = self.get_pipe_by_pipe_name(pn)
        if p == None:
            raise

        p.close()
        self.__pipes.remove(p)
    
    def get_pipe_by_pipe_name(self, pn):
        """
        Return the Pipe object specified by PipeName `pn`.
        """
        for p in self.__pipes:
            if p.pipe_name == pn:
                return p
        return None

    def read(self, pn):
        """
        Read content from the pipe specified by `pn`.
        """
        p = self.get_pipe_by_pipe_name(pn)
        if p == None:
            raise

        return p.read()
    
    def write(self, pn, buf):
        """
        Write `buf` to the PipeName specified by `pn`.
        """

        print(f"writing to pipe: {buf.decode('utf-8')}")
        p = self.get_pipe_by_pipe_name(pn)
        if p == None:
            raise

        return p.write(buf)
