import enum, os

class PipeType:
    OUTPUT = enum.auto()
    INPUT = enum.auto()

class Pipe:
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