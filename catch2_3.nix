{ lib
, stdenv
, fetchFromGitHub
, cmake
, python3
}:

stdenv.mkDerivation rec {
  pname = "catch2_local";
  version = "3.5.2";

  src = fetchFromGitHub {
    owner = "catchorg";
    repo = "Catch2";
    rev = "v${version}";
    hash = "sha256-xGPfXjk+oOnR7JqTrZd2pKJxalrlS8CMs7HWDClXaS8=";
  };

  nativeBuildInputs = [
    cmake
  ];

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
