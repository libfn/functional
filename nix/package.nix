{ lib
, pkgs
, stdenv
, cmake
, ccache
, llvmPackages_21
, ninja
, enableTests ? true
}:

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
  # Rebuild catch2_3 with the consumer's stdenv so its stdlib ABI matches libfn's.
  buildInputs = [ (pkgs.catch2_3.override { inherit stdenv; }) ];
  checkInputs = [ ];

  doCheck = enableTests;
  cmakeFlags = [ "-DDISABLE_CCACHE_DETECTION=On" "-DDISABLE_FETCH_CONTENT=On" ]
    ++ lib.optional (!enableTests) "-DLIBFN_TESTS=OFF";
}
