import json

class PipeReader:
    def __init__(self, pipe):
        self._PIPE = pipe

    def read(self, len = 1024):
        return self._PIPE.read(len)

class JsonReader(PipeReader):
    def __init__(self, pipe, read_validator=None):
        super().__init__(pipe)

        self.read_validator = read_validator if read_validator else self.default_read_validator
        self.tail = ""

    @staticmethod
    def default_read_validator(_):
        return True

    def _try_decode_json(self, s):
        """ decode and validate the first json object from a string """
        decoder = json.JSONDecoder()
        idx = 0
        while idx < len(s):
            slice = s[idx:].lstrip()
            if not slice:
                break
            offset = len(s[idx:]) - len(slice)
            try:
                obj, end = decoder.raw_decode(slice)
                if self.read_validator(obj):
                    idx += offset + end
                    return obj, s[idx:].rstrip()
                else:
                    idx += 1
            except json.JSONDecodeError as e:
                idx += 1
        return None, s

    def read(self):
        buffer = self.tail
        chunk_size = 1024
        max_buffer_size = 1024 * 1024

        while len(buffer) < max_buffer_size:
            try:
                chunk = self._PIPE.read(chunk_size)
                if not chunk:
                    break

                buffer += chunk.decode("utf-8")

                obj, tail = self._try_decode_json(buffer)

                if obj:
                    self.tail = tail
                    return obj

            except BlockingIOError:
                continue

        return None
