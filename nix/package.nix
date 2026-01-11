{ stdenv, meson, ninja, pkg-config }:

stdenv.mkDerivation {
  name = "hopper";

  src = ./..;

  nativeBuildInputs = [
    meson
    ninja
    pkg-config
  ];
}
