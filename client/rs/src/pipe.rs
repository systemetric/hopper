use std::{
    ops::Deref,
    os::{fd::OwnedFd, unix::fs::DirBuilderExt},
    path::{Path, PathBuf},
};

use bitflags::bitflags;
use nix::{
    errno::Errno,
    fcntl::{Flock, FlockArg, OFlag, open},
    sys::stat::Mode,
    unistd::{close, mkfifo, read, write},
};

use crate::Error;

bitflags! {
    /// Mode of operation of the Hopper pipe.
    ///
    /// **Must** include either [`PipeMode::IN`] or [`PipeMode::OUT`],
    /// but not both.
    #[derive(Debug, Clone)]
    pub struct PipeMode: u32 {
        /// Pipe should be opened as a server-side input
        const IN = (1 << 0);
        /// Pipe should be opened as a server-side output
        const OUT = (1 << 1);
        /// Pipe should be opened in non-blocking mode
        const NONBLOCK = (1 << 2);
    }
}

/// A pipe for interacting with the Hopper IPC server
pub struct Pipe {
    mode: PipeMode,
    name: String,
    endpoint: String,
    hopper: PathBuf,
    lock: Option<Flock<OwnedFd>>,
}

impl Pipe {
    fn get_open_flags(&self) -> OFlag {
        let mut flags = OFlag::empty();

        if self.mode.contains(PipeMode::IN) {
            flags |= OFlag::O_WRONLY;
        }
        if self.mode.contains(PipeMode::OUT) {
            flags |= OFlag::O_RDONLY;
        }
        if self.mode.contains(PipeMode::NONBLOCK) {
            flags |= OFlag::O_NONBLOCK;
        }

        flags
    }

    fn get_endpoint_path(&self) -> PathBuf {
        self.hopper.join(&self.endpoint)
    }

    fn get_pipe_path(&self) -> PathBuf {
        self.get_endpoint_path().join(format!(
            "{}.{}",
            &self.name,
            if self.mode.contains(PipeMode::IN) {
                "in"
            } else if self.mode.contains(PipeMode::OUT) {
                "out"
            } else {
                "" // how the hell did we get here?
            }
        ))
    }

    /// Create a new [`Pipe`] with a given mode, name, and endpoint.
    ///
    /// A path to the local Hopper instance can be optionally specified. If
    /// not specified it will be read from the `HOPPER_PATH` environment variable
    /// at runtime, returning [`Error::HopperNotFound`] if it cannot be identified.
    pub fn new<S, P>(mode: PipeMode, name: S, endpoint: S, hopper: Option<P>) -> Result<Self, Error>
    where
        S: AsRef<str>,
        P: AsRef<Path>,
    {
        if !(mode.contains(PipeMode::IN) ^ mode.contains(PipeMode::OUT)) {
            return Err(Error::InvalidMode);
        }

        let hopper = if let Some(p) = hopper {
            p.as_ref().to_path_buf()
        } else {
            PathBuf::from(
                std::env::var("HOPPER_PATH")
                    .ok()
                    .ok_or(Error::HopperNotFound)?,
            )
        };

        Ok(Self {
            mode,
            name: name.as_ref().to_string(),
            endpoint: endpoint.as_ref().to_string(),
            hopper,
            lock: None,
        })
    }

    /// Open the pipe described by this object.
    ///
    /// If the pipe is already open, close it first.
    ///
    /// A exclusive or shared lock is acquired on the pipe when opening
    /// depending on which mode is set. If the endpoint directiry does not
    /// exist, it will be created.
    pub fn open(&mut self) -> Result<(), Error> {
        if self.is_open() {
            self.close()?;
        }

        let endpoint = self.get_endpoint_path();
        std::fs::DirBuilder::new()
            .recursive(true)
            .mode(0o755)
            .create(&endpoint)
            .map_err(Error::Io)?;

        let pipe = self.get_pipe_path();
        match mkfifo(&pipe, Mode::from_bits_truncate(0o660)) {
            Ok(_) => {}
            Err(Errno::EEXIST) => {}
            Err(e) => return Err(Error::Other(e)),
        }

        let flags = self.get_open_flags();
        let fd = open(&pipe, flags, Mode::empty()).map_err(Error::Other)?;

        let lock = Flock::lock(
            fd,
            if self.mode.contains(PipeMode::IN) {
                FlockArg::LockExclusiveNonblock
            } else {
                FlockArg::LockSharedNonblock
            },
        );

        let lock = match lock {
            Ok(l) => l,
            Err((fd, e)) => {
                let _ = close(fd);

                return Err(Error::Other(if e == Errno::EWOULDBLOCK {
                    Errno::EBUSY // EBUSY makes more sense for clients
                } else {
                    e
                }));
            }
        };

        self.lock = Some(lock);

        Ok(())
    }

    /// Read a buffer from the pipe
    pub fn read(&self, buf: &mut [u8]) -> Result<usize, Error> {
        let lock = self.lock.as_ref().ok_or(Error::NotOpen)?;
        let res = match read(lock.deref(), buf) {
            Ok(s) => s,
            Err(Errno::EWOULDBLOCK) => 0, // this isn't an error for non-block pipes
            Err(e) => return Err(Error::Other(e)),
        };

        Ok(res)
    }

    /// Write a buffer to the pipe
    pub fn write(&self, buf: &[u8]) -> Result<usize, Error> {
        let lock = self.lock.as_ref().ok_or(Error::NotOpen)?;
        let res = match write(lock.deref(), buf) {
            Ok(s) => s,
            Err(Errno::EWOULDBLOCK) => 0, // this isn't an error for non-block pipes
            Err(e) => return Err(Error::Other(e)),
        };

        Ok(res)
    }

    /// Close the pipe, freeing all locks.
    pub fn close(&mut self) -> Result<(), Error> {
        let lock = self.lock.take();

        if let Some(lock) = lock {
            match lock.unlock() {
                Ok(fd) => close(fd).map_err(Error::Other)?,
                Err((lock, e)) => {
                    self.lock = Some(lock);
                    return Err(Error::Other(e));
                }
            }
        }

        Ok(())
    }

    /// Check if the pipe is currently open
    pub fn is_open(&self) -> bool {
        self.lock.is_some()
    }
}

impl Drop for Pipe {
    fn drop(&mut self) {
        let _ = self.close();
    }
}
