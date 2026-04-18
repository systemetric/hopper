{
  lib,
  python3Packages,
}:
python3Packages.buildPythonPackage {
  pname = "hopper";
  version = "0.1.1";
  pyproject = true;

  src = ./..;

  build-system = with python3Packages; [ setuptools ];

  meta = {
    description = "Python client library for Hopper IPC";
    homepage = "https://github.com/systemetric/hopper";
    license = lib.licenses.bsd2;
    platforms = lib.platforms.linux;
  };
}
