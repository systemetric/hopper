import os

from common import *

class RcMuxClient:
    def __init__(self, pn):
        self.__pn = pn
        self.__pipe = Pipe(pn, create=True)
    
    def read(self):
        return self.__pipe.read()
    
    def write(self, buf):
        return self.__pipe.write(buf)
