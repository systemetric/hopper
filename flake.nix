{
  description = "";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      utils,
    }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
    in
    utils.lib.eachSystem systems (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        packages = {
          hopper = {
            default = pkgs.callPackage ./nix/package.nix { };

            cross = {
              x86_64 = pkgs.pkgsCross.gnu64.callPackage ./nix/package.nix { };
              x86_64-static = pkgs.pkgsCross.gnu64.pkgsStatic.callPackage ./nix/package.nix { };
              aarch64 = pkgs.pkgsCross.aarch64-multiplatform.callPackage ./nix/package.nix { };
              aarch64-static = pkgs.pkgsCross.aarch64-multiplatform.pkgsStatic.callPackage ./nix/package.nix { };
            };
          };
        };

        devShell = pkgs.mkShell {
          packages = with pkgs; [
            clang-tools
            meson
            ninja
            pkg-config

            # one probably wants these too
            gdb
            valgrind
          ];
        };
      }
    );
}
