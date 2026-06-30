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
  checkPhase = ''
    runHook preCheck
    ./main
    runHook postCheck
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    cp main $out/bin/
    runHook postInstall
  '';
}
