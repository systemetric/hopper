{ stdenv, meson, ninja, pkg-config, python313 }:

stdenv.mkDerivation {
  name = "hopper";

  src = ./..;

  nativeBuildInputs = [
    meson
    ninja
    pkg-config
    python313
  ];
}
