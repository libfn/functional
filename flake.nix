{
  description = "Functional programming in C++";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
  };

  outputs = inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux" "aarch64-linux" "aarch64-darwin" "x86_64-darwin"
      ];
      perSystem = { config, self', inputs', pkgs, system, ... }: {
        packages = {
          default = pkgs.callPackage ./nix/package.nix { stdenv = pkgs.gcc13Stdenv; };
          clang = pkgs.callPackage ./nix/package.nix { stdenv = pkgs.llvmPackages_18.libcxxStdenv; };
          gcc = pkgs.callPackage ./nix/package.nix { stdenv = pkgs.gcc13Stdenv; };
        }
        // pkgs.lib.optionalAttrs (system != "x86_64-linux") {
          crossIntel = pkgs.pkgsCross.gnu64.callPackage ./nix/package.nix {
            enableTests = false;
          };
        } // pkgs.lib.optionalAttrs (system != "aarch64-linux") {
          crossAarch64 = pkgs.pkgsCross.aarch64-multiplatform.callPackage ./nix/package.nix {
            enableTests = false;
          };
        };
        checks = config.packages // {
          clang = config.packages.default.override {
            stdenv = pkgs.llvmPackages_18.stdenv;
          };
          gcc = config.packages.default.override {
            stdenv = pkgs.gcc13Stdenv;
          };
        };
      };
    };
}
