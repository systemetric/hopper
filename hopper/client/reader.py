import json


class PipeReader:
    def __init__(self, client, pipe_name):
        self._HOPPER_CLIENT = client
        self._PIPE_NAME = pipe_name

    def read(self):
        return self._HOPPER_CLIENT.read(self._PIPE_NAME)


class JsonReader(PipeReader):
    def __init__(self, client, pipe_name):
        super().__init__(client, pipe_name)

        if not self._HOPPER_CLIENT.get_pipe_by_pipe_name(pipe_name).blocking:
            print("WARN: Non-blocking reads may crash the brain!")

    def read(self):
        s = ""

        j = json.decoder.JSONDecoder()

        while (1):
            try:
                b = self._HOPPER_CLIENT.read(self._PIPE_NAME, _buf_size=1)
                s += b.decode("utf-8")

                j.decode(s)
            except json.decoder.JSONDecodeError:
                continue
            except BlockingIOError:
                continue

            break

        return j.decode(s)
