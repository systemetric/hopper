import os
import fcntl
import errno

from enum import Enum

class HopperPipeType(str, Enum):
    IN = "in"
    OUT = "out"

class HopperPipe:
    def __init__(self, type: HopperPipeType, name: str, endpoint: str,
                    hopper: str = "", nonblock: bool = False):
        self.type = type
        self.name = name
        self.endpoint = endpoint
        self.hopper = hopper
        self.nonblock = nonblock
        self.fd = -1

    def _get_open_flags(self):
        flags = 0
        if self.type == HopperPipeType.IN:
            flags |= os.O_WRONLY
        elif self.type == HopperPipeType.OUT:
            flags |= os.O_RDONLY
        if self.nonblock:
            flags |= os.O_NONBLOCK
        return flags

    def _get_endpoint_path(self):
        return os.path.join(self.hopper, self.endpoint)

    def _get_path(self):
        return os.path.join(self._get_endpoint_path(), f"{self.name}.{str(self.type.value)}")

    def open(self):
        if self.name == "" or self.endpoint == "":
            raise ValueError("Name and endpoint must be set")

        if self.hopper == "":
            hopper = os.getenv("HOPPER_PATH")
            if hopper:
                self.hopper = hopper
            else:
                raise ValueError("Hopper path not set, or HOPPER_PATH not available")

        endpoint_path = self._get_endpoint_path()
        os.makedirs(endpoint_path, exist_ok=True)

        path = self._get_path()

        try:
            os.mkfifo(path, mode=0o660)
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise e

        open_flags = self._get_open_flags()
        fd = os.open(path, open_flags)

        try:
            fcntl.flock(fd, (fcntl.LOCK_EX if self.type == HopperPipeType.IN else fcntl.LOCK_SH) | fcntl.LOCK_NB);
        except OSError as e:
            if e.errno == errno.EWOULDBLOCK:
                e.errno = EBUSY
            errsv = e.errno
            os.close(fd)
            raise OSError(errno=errsv)

        self.fd = fd

    def close(self):
        fcntl.flock(self.fd, fcntl.LOCK_UN)
        os.close(self.fd)
        self.fd = -1

    def read(self, len: int):
        try:
            buf = os.read(self.fd, len)
            return buf
        except OSError as e:
            if e.errno == EWOULDBLOCK:
                return b''
            else:
                raise e

    def write(self, buf: bytes):
        try:
            res = os.write(self.fd, buf)
            return res
        except OSError as e:
            if e.errno == EWOULDBLOCK:
                return 0;
            else:
                raise e

