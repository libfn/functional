{ lib
, stdenv
, fetchFromGitHub
, cmake
, python3
}:

stdenv.mkDerivation rec {
  pname = "catch2_local";
  version = "3.14.0";

  src = fetchFromGitHub {
    owner = "catchorg";
    repo = "Catch2";
    rev = "v${version}";
    hash = "sha256-tegAa+cNF7pJcW33B+VZ86ZlDG7dwS3o6QnN/XvTI2A=";
  };

  nativeBuildInputs = [
    cmake
  ];

  # On Nix, @lib_dir@ and @include_dir@ already expand to absolute paths under
  # /nix/store/..., so the upstream "${prefix}/@…@" join produces a double slash
  # that fixupPhase rejects. See https://github.com/NixOS/nixpkgs/issues/144170.
  postPatch = ''
    substituteInPlace CMake/catch2.pc.in \
      --replace 'libdir=''${prefix}/@lib_dir@' 'libdir=@lib_dir@' \
      --replace 'includedir=''${prefix}/@include_dir@' 'includedir=@include_dir@'
    substituteInPlace CMake/catch2-with-main.pc.in \
      --replace 'libdir=''${prefix}/@lib_dir@' 'libdir=@lib_dir@'
  '';

  hardeningDisable = [ "trivialautovarinit" ];

  cmakeFlags = [
    "-DCATCH_DEVELOPMENT_BUILD=ON"
    "-DCATCH_BUILD_TESTING=OFF"
  ];

  meta = {
    description = "Modern, C++-native, test framework for unit-tests";
    homepage = "https://github.com/catchorg/Catch2";
    changelog = "https://github.com/catchorg/Catch2/blob/${src.rev}/docs/release-notes.md";
    license = lib.licenses.boost;
    maintainers = with lib.maintainers; [ dotlambda ];
    platforms = with lib.platforms; unix ++ windows;
  };
}
