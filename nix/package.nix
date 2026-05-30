{ lib
, pkgs
, stdenv
, cmake
, ccache
, llvmPackages_21
, ninja
, enableTests ? true
, disableCxx23 ? false
}:

let
  catch2_local = pkgs.callPackage ./catch2_3.nix { inherit stdenv; };
in
stdenv.mkDerivation {
  name = "libfn";

  src = lib.sourceByRegex ./.. [
    "^include.*"
    "^tests.*"
    "^examples.*"
    "CMakeLists.txt"
    "^cmake.*"
    "README.md"
    "LICENSE.md"
    "VERSION"
  ];

  nativeBuildInputs = [ cmake ninja ccache llvmPackages_21.clang-tools ];
  buildInputs = [ catch2_local ];
  checkInputs = [ ];

  doCheck = enableTests;
  cmakeFlags = [ "-DDISABLE_CCACHE_DETECTION=On" "-DDISABLE_FETCH_CONTENT=On" ]
    ++ lib.optional (!enableTests) "-DLIBFN_TESTS=OFF"
    ++ lib.optional disableCxx23 "-DDISABLE_CXX23=ON";
}
