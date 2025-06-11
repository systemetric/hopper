import os

from .pipe_type import PipeType
from .pipe_name import PipeName

class Pipe:
    __BUF_SIZE = 64
    __fd = 0
    __pn = None
    __handler = None

    def __init__(self, pn: PipeName, create = False):
        self.__pn = pn
        self.__open(create=create)

    def __open(self, create = False):
        if self.__pn == None:
            return

        pipe_path = self.__pn.get_pipe_path()

        if not os.path.exists(pipe_path) and create:
            os.mkfifo(pipe_path)
        elif not os.path.exists(pipe_path):
            raise FileNotFoundError(f"Cannot find pipe at '{pipe_path}'.")

        self.__fd = os.open(pipe_path, os.O_NONBLOCK | os.O_RDWR)

    def read(self):
        try:
            buf = os.read(self.__fd, self.__BUF_SIZE)
        except:
            return None
        return buf
    
    def write(self, buf):
        try:
            os.write(self.__fd, buf)
        except:
            raise

    def close(self):
        os.close(self.__fd)

    def __del__(self):
        self.close()

    def get_type(self):
        if self.__pn == None:
            return None
        return self.__pn.get_type()
    
    def get_name(self):
        if self.__pn == None:
            return None
        return self.__pn.get_name()
    
    def get_handler_id(self):
        if self.__pn == None:
            return None
        return self.__pn.get_handler_id()
    
    def get_pipe_name(self):
        if self.__pn == None:
            return None
        return str(self.__pn)
    
    def get_pipe_path(self):
        if self.__pn == None:
            return None
        return self.__pn.get_pipe_path()

    def set_handler(self, handlers):
        if self.__pn == None:
            raise ValueError("Bad pipe name")
        
        try:
            self.__handler = handlers[self.__pn.get_handler_id()]
        except:
            raise

    def get_handler(self):
        return self.__handler


class PipeOld:
    BUF_SIZE = 64
    fd = 0
    handler = None

    def __init__(self, name, path):
        self.name = name
        self.path = path

        self.parse(name)

        if os.path.exists(path):
            self.fd = os.open(path, os.O_NONBLOCK | os.O_RDWR)
        else:
            raise FileNotFoundError(path)
        
    def assign_handler(self, handler):
        self.handler = handler
        
    def parse(self, name):
        p = name.split("_")
        self.pipe_type = PipeType.INPUT if p[0] == "I" else PipeType.OUTPUT
        self.type = p[1]
        self.id = p[2]
        
    def read(self):
        if self.pipe_type != PipeType.INPUT:
            raise
        
        try:
            buf = os.read(self.fd, self.BUF_SIZE)
        except:
            return None

        return buf

    def write(self, data):
        try:
            os.write(self.fd, data)
        except:
            raise

    def __del__(self):
        os.close(self.fd)
