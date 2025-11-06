# hopper

Pipe multiplexer for internal communication.

Hopper is the core part of the new communication system that ties together
various systems within the (updated) RoboCon brains. Hopper is only relevant
in brains for 2026 (and perhaps later) competitions. Where applicable, Hopper
is a primarily dependency for Shepherd and the robot library, but is also
needed for other smaller services. In the future, Hopper will be a dependency
of Wardog (unfinished as of 2026).

Hopper's primary purpose is to provide a multiplexer for named pipes,
specifically, FIFOs. I (OldUser101, Nathan Gill) may use the terms
"FIFO", "pipe", and "named pipe" interchangeably. If you really want to,
go and research the differences between these terms. In simple terms,
Hopper is a service that acts as a one-way broadcaster between various
data channels. In even simpler terms, one program can send a message, and
any number of other programs which are listening for it, can get the message.
It is functionally similar to MQTT (which was originally discussed prior to
Hopper's design in late 2025, and Hopper's inspiration), but doesn't touch
the network stack. The advantage of this is lower latency compared to the
network-based MQTT. The primary disadvantage is that external clients
(outside the brain) cannot communicate with the system directly, adding
network overhead and complexity for, say, the arena.

Originally, Hopper was written completely in Python, mainly for
interoperability with other services such as Shepherd, which (at the time of
writing) is also written in Python. Hopper is formed of multiple modules.
Each module of Hopper will be explained briefly, and in detail later.

The first part is the `server` module, which is a standalone program
that handles the multiplexing between named pipes in a directory. Due to
potential latency issues, and the sheer overhead of Python, the server module
was rewritten in C, during the development of the (still unfinished) Wardog
hardware server.

The second is the `client` module, which provides Python wrappers around Hopper
communication. In effect, it provides basic functions to allow Python clients
to open, read, write, and close named pipes in such a way that the Hopper
server can use them.

The third module is named `common`. This module provides common **Python**,
functionality, such as named pipe handling. Ironically, since the rewrite of
the Hopper server, the `common` module only provides functionality for the
`client` module, rather than being shared, as the name suggests.

Finally, the `util` module consists purely of programs that can be used to
test the functionality of both the Hopper `server`, and `client` modules.
Like `common`, it is badly named, and should probably be called `tests`,
but I can't be bothered to rename it.

## `server`

The `server` module is the standalone pipe multiplexer server that facilitates
Hopper itself. It is (now) written in C. If you don't know POSIX and/or Linux
APIs in general, this section may seem complicated. This section focuses on the
current C-based Hopper server, which improves upon the Python version (which is
largely undocumented). The core focus of the rewrite was significantly reducing
the latency when sending/reciving data through the server. This is critical for
both system stability, and the planned Wardog hardware server, which requires
near-zero latency for precise hardware timing.

In the Python version,
a single message would have an average latency of around 0.25 seconds, for
data buffers <1 KiB. Above this size, delays of seconds or longer could occur.
This was not only due to the overhead of Python, but also inefficiencies and
filesystem constraints. The primary part of this was that pipes were opened
in non-blocking mode, since the server was single threaded, and blocking is
non-ideal. Because of this, a 0.25 second delay was added to prevent the
process from consuming all system resources, constantly, and crashing Shepherd.

**OUT OF DATE**:The way the current Hopper server achieves this near-zero latency, and low
system resource usage, involves the use of Linux and POSIX APIs that are not
trivially exposed in Python. More specifically, `epoll` is used to block
the server when no data needs to be transferred, while keeping file descriptors
open in non-blocking mode for other purposes; the use of `splice`, `tee`, and
intermediate pipes mean that the transferred data buffers are never copied
into user space, allowing the kernel to effectively manage them. In effect,
the Hopper server doesn't actually copy any data itself, reducing latency
from the copy process, and allowing for the 1 KiB limit to be increased to
1 MiB in a single transfer.

**UPDATED**: Due to complexities and issues with using kernel buffers, a simple
ring buffer is used instead, which is slightly slower, but probably fine
for out use case.

For coordinating which FIFOs need to receive what data, Hopper uses a filename
format system.

`I/O_<HANDLER>_<NAME>`

- `I/O`, input or output pipe. This is relative to the server, and is a little
counterintuitive. An client providing an "input" pipe is actually *sending*
through the Hopper server, not receiving it.

- `<HANDLER>`, the handler ID. This should really be called the channel
identifier, as it controls the group of FIFOs that data is shared between.

- `<NAME>`, a unique name for the client.

For example, a FIFO with name `O_log_helper`, is an output pipe, that will
receive data from the Hopper server. It will receive all data from FIFOs that
have the matching handler `log` (for log messages), e.g. `I_log_robot` (logs
sent from usercode, the robot library). It is given the unique name `helper`.
This name corresponds to the `helper.py` service, provided alongside Shepherd.
As the name suggests, this FIFO will recieve logs, which need to be sent to
Sheep, and the arena.

Neither the handler ID or client name can contain underscores, as it messes
with the format system. However, this may not be checked by the rewritten
server?

The handler ID was originally given its name in the older Python server.
In that system, a handler was a particular class that processed data
sent through a pipe, invoked by the server. These handlers could manipulate
the data in transit, such as appending timestamps, or saving to a separate
file. The handler concept was ditched in the server rewrite, but the name,
and old values, still remain.

Handler IDs, and their respective internal mappings are defined in
`handler.h`, and `handler.c`.

A better way to identify channels of FIFOs could be a directory structure
like this:

```
/home/pi/pipes
|
----log
|   |
|   ----in
|   |   |
|   |   ----robot
|   |   |
|   |   ----runner
|   |   |
|   |   ...
|   |
|   ----out
|       |
|       ----helper
|       |
|       ----shepherd
|       |
|       ...
|
...
```

Rather than using filenames, the standard directory based model could be used.
This is much cleaner, and probably easier to understand, than the current
model.

The new Hopper server also uses `inotify` to detect when FIFOs and directories
change, as well as other mechanisms.

If you couldn't figure out already, the Hopper server takes a single argument:

- `<pipes directory>`, a path pointing to the directory of FIFOs to multiplex.

In practice, this is only ever `/home/pi/pipes`. It you ever get a fault where
the brain boots, the Wi-Fi network is operational, but doesn't get to flashy,
and Shepherd doesn't work, try SSHing and creating this directory, as
Hopper doesn't create it, and the failure of Hopper will prevent anything else
from starting at all.

## `client`

The second module of Hopper is the Python client bindings. These are relatively
easy to use. `hopper` should be installed as a Python module available to
use. If not, a `setup.py` is provided.

The client APIs are relatively easy to use, and basic examples can be found in
the `read.py` and `write.py` files in the `util` module.

The API also provides a `JsonReader` class, which is used in the development
version of Wardog, and simplifies reading JSON from Hopper. If you want
examples of this, check out Wardog.

## `common`

This module provides functionality to the Hopper `client`. This was previously
used by the `server` module, before the rewrite.

The `common` module is almost entirely re-exported by the `client` module,
so you should look at the documentation for that instead.

## `util`

The `util` module is (hopefully) not something you need documentation for, if
you've read everything above.
