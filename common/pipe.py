import os

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

        pipe_path = self.__pn.pipe_path

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

    @property
    def type(self):
        if self.__pn == None:
            return None
        return self.__pn.type
    
    @property
    def id(self):
        if self.__pn == None:
            return None
        return self.__pn.id
    
    @property
    def handler_id(self):
        if self.__pn == None:
            return None
        return self.__pn.handler_id
    
    @property
    def pipe_name(self):
        if self.__pn == None:
            return None
        return self.__pn
    
    @property
    def pipe_path(self):
        if self.__pn == None:
            return None
        return self.__pn.pipe_path

    def set_handler(self, handlers):
        if self.__pn == None:
            raise ValueError("Bad pipe name")
        
        try:
            self.__handler = handlers[self.__pn.handler_id]
        except:
            raise

    @property
    def handler(self):
        return self.__handler
