import os

from .spec import HandlerTypes

"""
Handlers produce the output given to pipes.
They all inherit from `PipeHandler`. When generating a message,
`get_output` is called, with the data that was read from the
input pipe as a byte string. `get_output` returns a byte string
of data to write to corresponding output pipes.
"""

class PipeHandler:
    def __init__(self, type):
        self.type = type

    def get_output(self, input):
        return input
    
class GenericHandler(PipeHandler):
    def __init__(self):
        super().__init__(HandlerTypes.GENERIC)

class LogHandler(PipeHandler):
    def __init__(self):
        super().__init__(HandlerTypes.LOG)

    def get_output(self, input):
        return input
    
class FullLogHandler(PipeHandler):
    def __init__(self):
        self.buf = b''

        super().__init__(HandlerTypes.FULL_LOG)

    def get_output(self, input):
        self.buf += input

        return self.buf

class CompleteLogHandler(PipeHandler):
    def __init__(self, file):
        self.f = open(file, "ab+")

        super().__init__(HandlerTypes.COMPLETE_LOG)

    def get_output(self, input):
        self.f.seek(0, os.SEEK_END)
        self.f.write(input)
        self.f.flush()
        self.f.seek(0)

        buf = self.f.read()

        return buf

    def __del__(self):
        self.f.close()