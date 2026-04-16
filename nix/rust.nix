{ lib, rustPlatform }:

let
  cargoToml = builtins.fromTOML (builtins.readFile ../client/rs/Cargo.toml);
  version = cargoToml.package.version;
in
rustPlatform.buildRustPackage {
  pname = "hopper";
  inherit version;

  src = ../.;

  cargoLock = {
    lockFile = ../Cargo.lock;
  };

  meta = {
    description = "Rust client library for Hopper IPC";
    homepage = "https://github.com/systemetric/hopper";
    license = lib.licenses.bsd2;
    platforms = lib.platforms.linux;
  };
}
