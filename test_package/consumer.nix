{ lib
, stdenv
, cmake
, ninja
, libfn
}:

stdenv.mkDerivation {
  name = "libfn-test-consumer";

  src = lib.sourceByRegex ./. [
    "CMakeLists.txt"
    "^src.*"
  ];

  nativeBuildInputs = [ cmake ninja ];
  buildInputs = [ libfn ];

  doCheck = true;
  # main.cpp is a quine: the binary's stdout must reproduce the source exactly.
  checkPhase = ''
    runHook preCheck
    ./main > main.out
    diff main.out ../src/main.cpp
    runHook postCheck
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    cp main $out/bin/
    runHook postInstall
  '';
}
