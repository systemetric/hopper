import os

from .pipe_type import PipeType

class PipeName:
    __type = None
    __handler_id = None
    __name = None
    __root = ""

    def __init__(self, x, root = ""):
        self.__root = root

        if type(x) is str:
            self.__from_str(x)
        elif type(x) is tuple:
            self.__from_tuple(x)
        else:
            raise ValueError("Invalid pipe name object.")

    def __from_str(self, s):
        p = s.split("_")
        self.__type = (PipeType.INPUT if p[0] == 'I' else PipeType.OUTPUT)
        self.__handler_id = p[1]
        self.__name = p[2]

    def __from_tuple(self, t):
        self.__type = t[0]
        self.__handler_id = t[1]
        self.__name = t[2]

    def __str__(self):
        return f"{'I' if self.__type == PipeType.INPUT else 'O'}_{self.__handler_id}_{self.__name}"

    def get_pipe_path(self):
        return os.path.join(self.__root, str(self))
    
    def get_type(self):
        return self.__type
    
    def get_handler_id(self):
        return self.__handler_id
    
    def get_name(self):
        return self.__name
    
    def get_root_path(self):
        return self.__root

