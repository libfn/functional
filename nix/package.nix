{ lib
, pkgs
, stdenv
, cmake
, ccache
, clang-tools_18
, enableTests ? true
}:

let 
  catch2_local = pkgs.callPackage ./catch2_3.nix { inherit stdenv; };
in
stdenv.mkDerivation {
  name = "functional";

  src = lib.sourceByRegex ./.. [
    "^include.*"
    "^tests.*"
    "CMakeLists.txt"
    "^cmake.*"
    "README.md"
    "LICENSE.md"
  ];

  nativeBuildInputs = [ cmake ccache clang-tools_18 ];
  buildInputs = [ catch2_local ];
  checkInputs = [ ];

  doCheck = enableTests;
  cmakeFlags = [ "-DDISABLE_CCACHE_DETECTION=On" "-DUSE_NIX=On" ] 
    ++ lib.optional (!enableTests) "-DTESTING=off";
}
