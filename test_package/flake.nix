{
  description = "Consumer-side integration tests for the functional library";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    functional.url = "path:..";
    functional.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, functional }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" "x86_64-darwin" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          libfn = functional.packages.${system}.default;
        in {
          consumer = pkgs.callPackage ./consumer.nix {
            stdenv = pkgs.gcc14Stdenv;
            functional = libfn;
          };
          consumer-no-cxx23 = pkgs.callPackage ./consumer.nix {
            stdenv = pkgs.gcc14Stdenv;
            functional = libfn.override { disableCxx23 = true; };
            disableCxx23 = true;
          };
        });
    };
}
