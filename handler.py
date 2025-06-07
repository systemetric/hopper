import os

class PipeHandler:
    def __init__(self, type):
        self.type = type

    def get_output(self, input):
        return input

class LogHandler(PipeHandler):
    def __init__(self):
        super().__init__("log")

    def get_output(self, input):
        return input
    
class FullLogHandler(PipeHandler):
    def __init__(self):
        self.buf = b''

        super().__init__("fulllog")

    def get_output(self, input):
        self.buf += input

        return self.buf

