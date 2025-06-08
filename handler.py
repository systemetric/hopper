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

class CompleteLogHandler(PipeHandler):
    def __init__(self, file):
        self.f = open(file, "ab+")

        super().__init__("complog")

    def get_output(self, input):
        buf = self.f.read()
        buf += input
        self.f.write(input)

        return buf

    def __del__(self):
        self.f.close()