{
  lib,
  stdenv,
  meson,
  ninja,
  pkg-config,
  python313,
}:

stdenv.mkDerivation {
  name = "hopper";

  src = ./..;

  nativeBuildInputs = [
    meson
    ninja
    pkg-config
    python313
  ];

  meta = {
    description = "Hopper IPC system";
    homepage = "https://github.com/systemetric/hopper";
    license = lib.licenses.bsd2;
    platforms = lib.platforms.linux;
  };
}
