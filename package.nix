{ lib
, pkgs
, stdenv
, cmake
, ccache
, enableTests ? true
}:

let 
  catch2_local = pkgs.callPackage ./catch2_3.nix { inherit stdenv; };
in
stdenv.mkDerivation {
  name = "libfn";

  # good source filtering is important for caching of builds.
  # It's easier when subprojects have their own distinct subfolders.
  src = lib.sourceByRegex ./. [
    "^include.*"
    "^tests.*"
    "CMakeLists.txt"
    "^cmake.*"
  ];

  # Distinguishing between native build inputs (runnable on the host
  # at compile time) and normal build inputs (runnable on target
  # platform at run time) is important for cross compilation.
  nativeBuildInputs = [ cmake ccache ];
  buildInputs = [ catch2_local ];
  checkInputs = [ ];

  doCheck = enableTests;
  cmakeFlags = [ "-DDISABLE_CCACHE_DETECTION=On" "-DUSE_NIX=On" ] 
    ++ lib.optional (!enableTests) "-DTESTING=off";
}
