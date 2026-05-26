{
  description = "Consumer-side integration tests for the libfn library";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    libfn.url = "github:libfn/functional";
    libfn.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, libfn }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" "x86_64-darwin" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          libfnPkg = libfn.packages.${system}.default;
        in {
          consumer = pkgs.callPackage ./consumer.nix {
            stdenv = pkgs.gcc14Stdenv;
            libfn = libfnPkg;
          };
          consumer-no-cxx23 = pkgs.callPackage ./consumer.nix {
            stdenv = pkgs.gcc14Stdenv;
            libfn = libfnPkg.override { disableCxx23 = true; };
            disableCxx23 = true;
          };
        });
    };
}
